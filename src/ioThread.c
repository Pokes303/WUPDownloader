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

#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <ioThread.h>
#include <utils.h>

#define IOT_STACK_SIZE			0x2000
#define SLICE_SIZE				512 * 1024 // 512 KB
#define MAX_IO_QUEUE_ENTRIES	(128 * 1024 * 1024) / (SLICE_SIZE) // 128 MB

struct WriteQueueEntry;
typedef struct WriteQueueEntry WriteQueueEntry;
struct WriteQueueEntry
{
	FILE *file;
	uint8_t *buf;
	size_t size;
	bool close;
	volatile bool inUse;
};

static OSThread ioThread;
static uint8_t ioThreadStack[IOT_STACK_SIZE];
static volatile bool ioRunning = false;
static volatile bool ioWriteLock = true;

static volatile WriteQueueEntry sliceEntries[MAX_IO_QUEUE_ENTRIES];
static uint8_t sliceBuffer[MAX_IO_QUEUE_ENTRIES][SLICE_SIZE];
static uint8_t *sliceBufferPointer = &sliceBuffer[0][0];
static uint8_t *sliceBufferEnd = &sliceBuffer[0][0] + SLICE_SIZE;
static volatile uint32_t activeReadBuffer = 0;
static volatile uint32_t activeWriteBuffer = 0;

void executeIOQueue()
{
	uint32_t asl = activeWriteBuffer;
	if(!sliceEntries[asl].inUse)
		return;
	
	if(sliceEntries[asl].buf != NULL)
		fwrite(sliceEntries[asl].buf, sliceEntries[asl].size, 1, sliceEntries[asl].file);
	
	if(sliceEntries[asl].close)
	{
		fflush(sliceEntries[asl].file);
		fclose(sliceEntries[asl].file);
	}
	
	sliceEntries[asl].inUse = false;
	
	if(++asl == MAX_IO_QUEUE_ENTRIES)
		asl = 0;
	
	activeWriteBuffer = asl;
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
	
	for(int i = 0; i < MAX_IO_QUEUE_ENTRIES; i++)
		sliceEntries[i].inUse = false;
	
	ioRunning = true;
	OSResumeThread(&ioThread);
	return true;
}

void shutdownIOThread()
{
	if(!ioRunning)
		return;
	
	ioWriteLock = true;
	while(sliceEntries[activeWriteBuffer].inUse)
		;
	
	ioRunning = false;
	int ret;
	OSJoinThread(&ioThread, &ret);
	debugPrintf("I/O thread returned: %d", ret);
}

size_t addToIOQueue(const void *buf, size_t size, size_t n, FILE *file)
{
	while(ioWriteLock)
		if(!ioRunning)
			return 0;
	
	uint32_t asl = activeReadBuffer;
	while(sliceEntries[asl].inUse)
	{
		debugPrintf("Waiting for free slot...");
		OSSleepTicks(256);
	}
	
	size_t written, rest;
	if(buf != NULL)
	{
		size *= n;
		uint8_t *end = sliceBufferPointer + size;
		if(end < sliceBufferEnd)
		{
			OSBlockMove(sliceBufferPointer, buf, size, false);
			sliceBufferPointer = end;
			return size;
		}
		
		written = sliceBufferEnd - sliceBufferPointer;
		OSBlockMove(sliceBufferPointer, buf, written, false);
		sliceBufferPointer = sliceBufferEnd;
		rest = size - written;
		
		sliceEntries[asl].close = false;
	}
	else
		sliceEntries[asl].close = true;
	
	sliceEntries[asl].buf = &sliceBuffer[activeReadBuffer][0];
	sliceEntries[asl].size = sliceBufferPointer - sliceEntries[asl].buf;
	sliceEntries[asl].file = file;
	sliceEntries[asl].inUse = true;
	
	if(++asl == MAX_IO_QUEUE_ENTRIES)
		asl = 0;
	
	activeReadBuffer = asl;
	
	sliceBufferPointer = &sliceBuffer[asl][0];
	sliceBufferEnd = sliceBufferPointer + SLICE_SIZE;
	
	if(buf != NULL && rest > 0)
		addToIOQueue(((const uint8_t *)buf) + written, rest, 1, file);
	
	return size;
}

void flushIOQueue()
{
	debugPrintf("Flushing...");
	ioWriteLock = true;
	while(sliceEntries[activeWriteBuffer].inUse)
		OSSleepTicks(1024);
	
	ioWriteLock = false;
}
