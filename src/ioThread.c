/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2021 V10lator <v10lator@myway.de>                    *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *
 ***************************************************************************/

#include <wut-fixups.h>

#include <stdbool.h>
#include <stdio.h>

#include <coreinit/atomic.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <downloader.h>
#include <ioThread.h>
#include <utils.h>

#define IOT_STACK_SIZE			0x2000
#define MAX_IO_BUFFER_SIZE	SOCKET_BUFSIZE
#define MAX_IO_QUEUE_ENTRIES	((64 * 1024 * 1024) / MAX_IO_BUFFER_SIZE) // 64 MB
#define IO_MAX_OPEN_FILES	8
#define IO_MAX_FILE_BUFFER	(1 * 1024 * 1024) // 1 MB

typedef struct
{
	volatile NUSFILE *file;
	volatile uint8_t *buf;
	volatile size_t size;
	volatile bool inUse;
} WriteQueueEntry;

typedef struct
{
	NUSFILE *file;
	uint8_t *buf;
	size_t i;
} OpenFile;

static OSThread ioThread;
static uint8_t *ioThreadStack;
static volatile bool ioRunning = false;
static volatile uint32_t ioWriteLock = true;
static volatile uint32_t *ioWriteLockPtr = &ioWriteLock;

static WriteQueueEntry *queueEntries;
static volatile uint32_t activeReadBuffer;
static volatile uint32_t activeWriteBuffer;

static OpenFile files[IO_MAX_OPEN_FILES];

static void executeIOQueue()
{
	uint32_t asl = activeWriteBuffer;
	if(!queueEntries[asl].inUse)
	{
		OSSleepTicks(256);
		return;
	}
	
	int openFile = 0;
	while(true)
	{
		if(files[openFile].file == queueEntries[asl].file)
			break;
		
		if(++openFile == IO_MAX_OPEN_FILES)
		{
			openFile = 0;
			while(true)
			{
				if(files[openFile].file == NULL)
				{
					files[openFile].file = queueEntries[asl].file;
					break;
				}
				
				if(++openFile == IO_MAX_OPEN_FILES)
				{
					debugPrintf("TOO MANY OPEN FILES!");
					return;
				}
			}
		}
	}
	
	if(queueEntries[asl].size != 0) // WRITE command
	{
		size_t r = 0;
		if(files[openFile].i + queueEntries[asl].size >= IO_MAX_FILE_BUFFER) // small buffer don't fit into the large buffer anymore
		{
			r = IO_MAX_FILE_BUFFER - files[openFile].i; // get free space of large buffer
			if(r != 0)
			{
				OSBlockMove(files[openFile].buf + files[openFile].i, queueEntries[asl].buf, r, false); // make the large buffer full
				queueEntries[asl].size -= r; // adjust small buffers size
			}
			
			fwrite(files[openFile].buf, 1, IO_MAX_FILE_BUFFER, files[openFile].file->fd); // Write large buffer to disc
			files[openFile].i = 0; // set large buffer as empty
			
			if(queueEntries[asl].size == 0)
				goto ioQueueExitPoint;
		}
		
		// Copy small buffer into large buffer but do not copy what might have been written to disc already (that's the + r)
		OSBlockMove(files[openFile].buf + files[openFile].i, queueEntries[asl].buf + r, queueEntries[asl].size, false);
		files[openFile].i += queueEntries[asl].size;
	}
	else // Close command
	{
		// In case there's something which needs to go to disc: Write it
		if(files[openFile].i != 0)
		{
			fwrite(files[openFile].buf, 1, files[openFile].i, files[openFile].file->fd);
			files[openFile].i = 0;
		}
		
		// Close the file
		fflush(files[openFile].file->fd);
		fclose(files[openFile].file->fd);
		MEMFreeToDefaultHeap(files[openFile].file->buffer);
		MEMFreeToDefaultHeap(files[openFile].file);
		files[openFile].file = NULL;
	}
	
ioQueueExitPoint:
	;
	uint32_t osl = asl;
	if(++asl == MAX_IO_QUEUE_ENTRIES)
		asl = 0;
	
	activeWriteBuffer = asl;
	queueEntries[osl].inUse = false;
}

int ioThreadMain(int argc, const char **argv)
{
	debugPrintf("I/O queue running!");
	ioWriteLock = false;
	while(ioRunning)
		executeIOQueue();
	
	return 0;
}

bool initIOThread()
{
	ioThreadStack = MEMAllocFromDefaultHeapEx(IOT_STACK_SIZE, 8);
	if(ioThreadStack == NULL || !OSCreateThread(&ioThread, ioThreadMain, 0, NULL, ioThreadStack + IOT_STACK_SIZE, IOT_STACK_SIZE, 0, OS_THREAD_ATTRIB_AFFINITY_CPU0)) // We move this to core 0 for maximum performance. Later on move it back to core 1 as we want download threads on core 0 and 2.
		return false;
	
	OSSetThreadName(&ioThread, "NUSspli I/O");
	
	queueEntries = MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * sizeof(WriteQueueEntry));
	if(queueEntries == NULL)
	{
		MEMFreeToDefaultHeap(ioThreadStack);
		debugPrintf("OUT OF MEMORY (queueEntries)!");
		return false;
	}
	
	uint8_t *ptr = (uint8_t *)MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * MAX_IO_BUFFER_SIZE);
	if(ptr == NULL)
	{
		MEMFreeToDefaultHeap(ioThreadStack);
		MEMFreeToDefaultHeap(queueEntries);
		debugPrintf("OUT OF MEMORY (ptr1)!");
		return false;
	}
	
	for(int i = 0; i < MAX_IO_QUEUE_ENTRIES; i++, ptr += MAX_IO_BUFFER_SIZE)
	{
		queueEntries[i].buf = ptr;
		queueEntries[i].inUse = false;
	}
	
	ptr = MEMAllocFromDefaultHeap(IO_MAX_OPEN_FILES * IO_MAX_FILE_BUFFER);
	if(ptr == NULL)
	{
		MEMFreeToDefaultHeap(ioThreadStack);
		MEMFreeToDefaultHeap((void *)queueEntries[0].buf);
		MEMFreeToDefaultHeap((void *)queueEntries);
		debugPrintf("OUT OF MEMORY (ptr2)!");
		return false;
	}
	
	for(int i = 0; i < IO_MAX_OPEN_FILES; i++, ptr += IO_MAX_FILE_BUFFER)
	{
		files[i].file = NULL;
		files[i].buf = ptr;
		files[i].i = 0;
	}
	
	activeReadBuffer = activeWriteBuffer = 0;
	
	ioRunning = true;
	OSResumeThread(&ioThread);
	return true;
}

void shutdownIOThread()
{
	if(!ioRunning)
		return;
	
	while(!OSCompareAndSwapAtomic(ioWriteLockPtr, false, true))
		;
	while(queueEntries[activeWriteBuffer].inUse)
		;
	
	ioRunning = false;
	int ret;
	OSJoinThread(&ioThread, &ret);
	MEMFreeToDefaultHeap(ioThreadStack);
	MEMFreeToDefaultHeap((void *)queueEntries[0].buf);
	MEMFreeToDefaultHeap((void *)queueEntries);
	MEMFreeToDefaultHeap(files[0].buf);
	debugPrintf("I/O thread returned: %d", ret);
}

size_t addToIOQueue(const void *buf, size_t size, size_t n, NUSFILE *file)
{
	uint32_t asl;
		
retryAddingToQueue:
	
	while(!OSCompareAndSwapAtomic(ioWriteLockPtr, false, true))
		if(!ioRunning)
			return 0;
	
	asl = activeReadBuffer;
	if(queueEntries[asl].inUse)
	{
		ioWriteLock = false;
		debugPrintf("Waiting for free slot...");
		OSSleepTicks(256);
		goto retryAddingToQueue; // We use goto here instead of just calling addToIOQueue again to not overgrow the stack.
	}
	
	if(buf != NULL)
	{
		size *= n;
		if(size < 1)
		{
			debugPrintf("size < 1 (%i)", size);
			ioWriteLock = false;
			return 0;
		}
		
		if(size > MAX_IO_BUFFER_SIZE)
		{
			debugPrintf("size > %i (%i)", MAX_IO_BUFFER_SIZE, size);
			ioWriteLock = false;
			addToIOQueue(buf, 1, MAX_IO_BUFFER_SIZE, file);
			const uint8_t *newPtr = buf;
			newPtr += MAX_IO_BUFFER_SIZE;
			addToIOQueue((const void *)newPtr, 1, size - MAX_IO_BUFFER_SIZE, file);
			return n;
		}
		
		OSBlockMove(queueEntries[asl].buf, buf, size, false);
		queueEntries[asl].size = size;
	}
	else
		queueEntries[asl].size = 0;
	
	queueEntries[asl].file = file;
	queueEntries[asl].inUse = true;
	
	if(++asl == MAX_IO_QUEUE_ENTRIES)
		asl = 0;
	
	activeReadBuffer = asl;
	ioWriteLock = false;
	return n;
}

void flushIOQueue()
{
	debugPrintf("Flushing...");
	
	while(!OSCompareAndSwapAtomic(ioWriteLockPtr, false, true))
		;
	
	while(queueEntries[activeWriteBuffer].inUse)
		OSSleepTicks(1024);
	
	ioWriteLock = false;
}

NUSFILE *openFile(const char *path, const char *mode)
{
	NUSFILE *ret = MEMAllocFromDefaultHeap(sizeof(NUSFILE));
	if(ret == NULL)
		return NULL;
	
	ret->fd = fopen(path, mode);
	if(ret->fd == NULL)
	{
		MEMFreeToDefaultHeap(ret);
		return NULL;
	}
	
	ret->buffer = MEMAllocFromDefaultHeapEx(IO_MAX_FILE_BUFFER, 0x40);
	if(ret->buffer == NULL)
	{
		fclose(ret->fd);
		MEMFreeToDefaultHeap(ret);
		return NULL;
	}
	
	if(setvbuf(ret->fd, ret->buffer, _IOFBF, IO_MAX_FILE_BUFFER) != 0)
		debugPrintf("Error setting buffer!");
	
	return ret;
}
