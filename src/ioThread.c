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

#include <crypto.h>
#include <file.h>
#include <ioThread.h>
#include <utils.h>

#define IOT_STACK_SIZE		0x2000
#define MAX_IO_QUEUE_ENTRIES	((256 * 1024 * 1024) / IO_BUFSIZE) // 256 MB
#define IO_MAX_FILE_BUFFER	(512 * 1024) // 512 KB

typedef struct
{
	volatile NUSFILE *file;
	void *buf;
	volatile size_t size;
	volatile bool inUse;
} WriteQueueEntry;

static OSThread ioThread;
static void *ioThreadStack;
static volatile bool ioRunning = false;
#ifdef NUSSPLI_DEBUG
static volatile uint32_t ioWriteLock = true;
static volatile uint32_t *ioWriteLockPtr = &ioWriteLock;
#endif

static WriteQueueEntry *queueEntries;
static volatile uint32_t activeReadBuffer;
static volatile uint32_t activeWriteBuffer;

static int ioThreadMain(int argc, const char **argv)
{
#ifdef NUSSPLI_DEBUG
	debugPrintf("I/O queue running!");
	ioWriteLock = false;
#endif
	uint32_t asl;
	WriteQueueEntry *entry;
	while(ioRunning)
	{
		asl = activeWriteBuffer;
		entry = queueEntries + asl;
		if(!entry->inUse)
		{
			OSSleepTicks(256);
			continue;
		}

		if(entry->size != 0) // WRITE command
			fwrite(entry->buf, entry->size, 1, entry->file->fd);
		else // Close command
		{
			fflush(entry->file->fd);
			fclose(entry->file->fd);
			MEMFreeToDefaultHeap(entry->file->buffer);
			MEMFreeToDefaultHeap((void *)entry->file);
		}

		if(++asl == MAX_IO_QUEUE_ENTRIES)
			asl = 0;

		activeWriteBuffer = asl;
		entry->inUse = false;
	}
	
	return 0;
}

bool initIOThread()
{
	queueEntries = MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * sizeof(WriteQueueEntry));
	if(queueEntries != NULL)
	{
		uint8_t *ptr = (uint8_t *)MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * IO_BUFSIZE);
		if(ptr != NULL)
		{
			for(int i = 0; i < MAX_IO_QUEUE_ENTRIES; i++, ptr += IO_BUFSIZE)
			{
				queueEntries[i].buf = (void *)ptr;
				queueEntries[i].inUse = false;
			}

			activeReadBuffer = activeWriteBuffer = 0;
			ioRunning = true;

			if(startThread(&ioThread, "NUSspli I/O", THREAD_PRIORITY_HIGH, &ioThreadStack, IOT_STACK_SIZE, ioThreadMain, OS_THREAD_ATTRIB_AFFINITY_CPU0)) // We move this to core 0 for maximum performance. Later on move it back to core 1 as we want download threads on core 0 and 2.
				return true;

			MEMFreeToDefaultHeap(queueEntries[0].buf);
		}
		MEMFreeToDefaultHeap(queueEntries);
	}
	return false;
}

void shutdownIOThread()
{
	if(!ioRunning)
		return;
	
#ifdef NUSSPLI_DEBUG
	while(!OSCompareAndSwapAtomic(ioWriteLockPtr, false, true))
		;
#endif
	while(queueEntries[activeWriteBuffer].inUse)
		;
	
	ioRunning = false;
	int ret;
	OSJoinThread(&ioThread, &ret);
	MEMFreeToDefaultHeap(ioThreadStack);
	MEMFreeToDefaultHeap(queueEntries[0].buf);
	MEMFreeToDefaultHeap(queueEntries);
	debugPrintf("I/O thread returned: %d", ret);
}

size_t addToIOQueue(const void *buf, size_t size, size_t n, NUSFILE *file)
{
    WriteQueueEntry *entry;
		
retryAddingToQueue:
	
#ifdef NUSSPLI_DEBUG
	while(!OSCompareAndSwapAtomic(ioWriteLockPtr, false, true))
		if(!ioRunning)
			return 0;
#endif
	
    entry = queueEntries + activeReadBuffer;
	if(entry->inUse)
	{
#ifdef NUSSPLI_DEBUG
		ioWriteLock = false;
#endif
		debugPrintf("Waiting for free slot...");
		OSSleepTicks(256);
		goto retryAddingToQueue; // We use goto here instead of just calling addToIOQueue again to not overgrow the stack.
	}
	
	if(buf != NULL)
	{
		size *= n;
		if(size == 0)
		{
			n = 0;
			goto queueExit;
		}
		
		if(size > IO_BUFSIZE)
		{
			debugPrintf("size > %i (%i)", IO_BUFSIZE, size);
#ifdef NUSSPLI_DEBUG
			ioWriteLock = false;
#endif
			addToIOQueue(buf, 1, IO_BUFSIZE, file);
			const uint8_t *newPtr = buf;
			newPtr += IO_BUFSIZE;
			addToIOQueue((const void *)newPtr, 1, size - IO_BUFSIZE, file);
			return n;
		}
		
		OSBlockMove(entry->buf, buf, size, false);
		entry->size = size;
	}
	else
		entry->size = 0;
	
	entry->file = file;
	entry->inUse = true;
	
	if(++activeReadBuffer == MAX_IO_QUEUE_ENTRIES)
		activeReadBuffer = 0;
	
queueExit:
#ifdef NUSSPLI_DEBUG
	ioWriteLock = false;
#endif
	return n;
}

void flushIOQueue()
{
#ifdef NUSSPLI_DEBUG
	debugPrintf("Flushing...");
	while(!OSCompareAndSwapAtomic(ioWriteLockPtr, false, true))
		;
#endif
	while(queueEntries[activeWriteBuffer].inUse)
		OSSleepTicks(1024);
	
#ifdef NUSSPLI_DEBUG
	ioWriteLock = false;
#endif
}

NUSFILE *openFile(const char *path, const char *mode)
{
	NUSFILE *ret = MEMAllocFromDefaultHeap(sizeof(NUSFILE));
	if(ret == NULL)
		return NULL;
	
	OSTime t = OSGetSystemTime();
	ret->fd = fopen(path, mode);
	if(ret->fd != NULL)
	{
		t = OSGetSystemTime() - t;
		addEntropy(&t, sizeof(OSTime));
		ret->buffer = MEMAllocFromDefaultHeapEx(IO_MAX_FILE_BUFFER, 0x40);
		if(ret->buffer != NULL)
		{
			if(setvbuf(ret->fd, ret->buffer, _IOFBF, IO_MAX_FILE_BUFFER) != 0)
				debugPrintf("Error setting buffer!");

			return ret;
		}
		fclose(ret->fd);
	}
	MEMFreeToDefaultHeap(ret);
	return NULL;
}
