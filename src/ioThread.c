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
	FILE *file;
	uint8_t *buf;
	size_t size;
	volatile bool inUse;
} WriteQueueEntry;

typedef struct
{
	FILE *file;
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

void executeIOQueue()
{
	uint32_t asl = activeWriteBuffer;
	if(!queueEntries[asl].inUse)
		return;
	
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
	
	if(queueEntries[asl].size != 0)
	{
		size_t r = 0;
		if(files[openFile].i + queueEntries[asl].size >= IO_MAX_FILE_BUFFER)
		{
			r = IO_MAX_FILE_BUFFER - files[openFile].i;
			if(r != 0)
				OSBlockMove(files[openFile].buf + files[openFile].i, queueEntries[asl].buf, r, false);
			
			fwrite(files[openFile].buf, IO_MAX_FILE_BUFFER, 1, files[openFile].file);
			
			queueEntries[asl].size -= r;
			files[openFile].i = 0;
			
			if(queueEntries[asl].size == 0)
				goto ioQueueExitPoint;
		}
		
		OSBlockMove(files[openFile].buf + files[openFile].i, queueEntries[asl].buf + r, queueEntries[asl].size, false);
		files[openFile].i += queueEntries[asl].size;
	}
	else
	{
		if(files[openFile].i != 0)
		{
			fwrite(files[openFile].buf, files[openFile].i, 1, files[openFile].file);
			files[openFile].i = 0;
		}
		fclose(files[openFile].file);
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
		debugPrintf("OUT OF MEMORY (queueEntries)!");
		return false;
	}
	
	uint8_t *ptr = MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * MAX_IO_BUFFER_SIZE);
	if(ptr == NULL)
	{
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
		MEMFreeToDefaultHeap(queueEntries[0].buf);
		MEMFreeToDefaultHeap(queueEntries);
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
	
	MEMFreeToDefaultHeap(queueEntries[0].buf);
	MEMFreeToDefaultHeap(queueEntries);
	MEMFreeToDefaultHeap(files[0].buf);
	
	ioRunning = false;
	int ret;
	OSJoinThread(&ioThread, &ret);
	MEMFreeToDefaultHeap(ioThreadStack);
	debugPrintf("I/O thread returned: %d", ret);
}

size_t addToIOQueue(const void *buf, size_t size, size_t n, FILE *file)
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
			size_t newSize = size - MAX_IO_BUFFER_SIZE;
			const uint8_t *newPtr = buf;
			newPtr += newSize;
			addToIOQueue(newPtr, 1, newSize, file);
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
