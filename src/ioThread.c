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

#include <ioThread.h>
#include <utils.h>

#define IOT_STACK_SIZE			0x2000
#define MAX_IO_BUFFER_SIZE	(8 * 1024) // 8 KB
#define MAX_IO_QUEUE_ENTRIES	((512 * 1024 * 1024) / MAX_IO_BUFFER_SIZE) // 512 MB

struct WriteQueueEntry;
typedef struct WriteQueueEntry WriteQueueEntry;
struct WriteQueueEntry
{
	FILE *file;
	void *buf;
	size_t size;
	volatile bool inUse;
};

static OSThread ioThread;
static uint8_t ioThreadStack[IOT_STACK_SIZE];
static volatile bool ioRunning = false;
static volatile uint32_t ioWriteLock = true;
static volatile uint32_t *ioWriteLockPtr = &ioWriteLock;

static volatile WriteQueueEntry *sliceEntries;
static volatile uint32_t activeReadBuffer;
static volatile uint32_t activeWriteBuffer;

void executeIOQueue()
{
	uint32_t asl = activeWriteBuffer;
	if(!sliceEntries[asl].inUse)
		return;
	
	if(sliceEntries[asl].size != 0)
	{
		fwrite(sliceEntries[asl].buf, sliceEntries[asl].size, 1, sliceEntries[asl].file);
	}
	else
	{
		fflush(sliceEntries[asl].file);
		fclose(sliceEntries[asl].file);
	}
	
	uint32_t osl = asl;
	if(++asl == MAX_IO_QUEUE_ENTRIES)
		asl = 0;
	
	activeWriteBuffer = asl;
	sliceEntries[osl].inUse = false;
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
	if(!OSCreateThread(&ioThread, ioThreadMain, 0, NULL, ioThreadStack + IOT_STACK_SIZE, IOT_STACK_SIZE, 0, OS_THREAD_ATTRIB_AFFINITY_CPU0)) // We move this to core 0 for maximum performance. Later on move it back to core 1 as we want download threads on core 0 and 2.
		return false;
	
	OSSetThreadName(&ioThread, "NUSspli I/O");
	
	sliceEntries = MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * sizeof(WriteQueueEntry));
	if(sliceEntries == NULL)
	{
		debugPrintf("OUT OF MEMORY (sliceEntries)!");
		return false;
	}
	
	uint8_t *ptr = MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * MAX_IO_BUFFER_SIZE);
	if(ptr == NULL)
	{
		MEMFreeToDefaultHeap(sliceEntries);
		debugPrintf("OUT OF MEMORY (ptr)!");
		return false;
	}
	for(int i = 0; i < MAX_IO_QUEUE_ENTRIES; i++, ptr += MAX_IO_BUFFER_SIZE)
	{
		sliceEntries[i].buf = ptr;
		sliceEntries[i].inUse = false;
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
	while(sliceEntries[activeWriteBuffer].inUse)
		;
	
	MEMFreeToDefaultHeap(sliceEntries[0].buf);
	MEMFreeToDefaultHeap(sliceEntries);
	
	ioRunning = false;
	int ret;
	OSJoinThread(&ioThread, &ret);
	debugPrintf("I/O thread returned: %d", ret);
}

size_t addToIOQueue(const void *buf, size_t size, size_t n, FILE *file)
{
	while(!OSCompareAndSwapAtomic(ioWriteLockPtr, false, true))
		if(!ioRunning)
			return 0;
	
	uint32_t asl = activeReadBuffer;
	if(sliceEntries[asl].inUse)
	{
		ioWriteLock = false;
		debugPrintf("Waiting for free slot...");
		OSSleepTicks(256);
		return addToIOQueue(buf, size, n, file);
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
		if(size < 1)
		{
			debugPrintf("size > %i (%i)", MAX_IO_BUFFER_SIZE, size);
			ioWriteLock = false;
			return 0;
		}
		
		OSBlockMove(sliceEntries[asl].buf, buf, size, false);
		sliceEntries[asl].size = size;
	}
	else
		sliceEntries[asl].size = 0;
	
	sliceEntries[asl].file = file;
	sliceEntries[asl].inUse = true;
	
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
	
	while(sliceEntries[activeWriteBuffer].inUse)
		OSSleepTicks(1024);
	
	ioWriteLock = false;
}
