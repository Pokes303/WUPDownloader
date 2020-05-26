/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020 V10lator <v10lator@myway.de>                         *
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

#include <coreinit/fastmutex.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <gui/memory.h>

#include <ioThread.h>
#include <memdebug.h>
#include <utils.h>

#define IOT_STACK_SIZE		0x2000
#define MAX_IO_QUEUE_SIZE	64 * 1024 * 1024

struct WriteQueueEntry;
typedef struct WriteQueueEntry WriteQueueEntry;
struct WriteQueueEntry
{
	FILE *file;
	void *buf;
	size_t size;
	bool fast;
	WriteQueueEntry *next;
};

static OSThread ioThread;
static uint8_t ioThreadStack[IOT_STACK_SIZE];
static bool ioRunning = false;
static OSFastMutex *writeQueueMutex;
static WriteQueueEntry *writeQueue = NULL;
static WriteQueueEntry *lastWriteQueueEntry;
static size_t writeQueueSize = 0;

static inline void lockIOQueueAgressive()
{
	while(!OSFastMutex_TryLock(writeQueueMutex))
		;
}

void executeIOQueue(bool unlock)
{
	if(writeQueue != NULL)
	{
		WriteQueueEntry *entry = writeQueue;
		writeQueue = entry->next;
		writeQueueSize -= entry->size;
		if(unlock)
			OSFastMutex_Unlock(writeQueueMutex);
		
		if(entry->buf != NULL)
		{
			fwrite(entry->buf, entry->size, 1, entry->file);
			if(entry->fast)
				MEM1_free(entry->buf);
			else
				MEMFreeToDefaultHeap(entry->buf);
		}
		else
		{
			fflush(entry->file);
			fclose(entry->file);
		}
		
		if(entry->fast)
			MEM1_free(entry);
		else
			MEMFreeToDefaultHeap(entry);
		
		return;
	}
	if(unlock)
		OSFastMutex_Unlock(writeQueueMutex);
}

int ioThreadMain(int argc, const char **argv)
{
	debugPrintf("I/O queue running!");
	while(ioRunning)
	{
		lockIOQueueAgressive();
		executeIOQueue(true);
	}
	return 0;
}

bool initIOThread()
{
	writeQueueMutex = MEM1_alloc(sizeof(OSFastMutex), 4);
	if(writeQueueMutex == NULL)
		return false;
	
	if(!OSCreateThread(&ioThread, ioThreadMain, 0, NULL, ioThreadStack + IOT_STACK_SIZE, IOT_STACK_SIZE, 0, OS_THREAD_ATTRIB_AFFINITY_CPU0)) // We move this to core 0 for maximum performance. Later on move it back to core 1 as we want download threads on core 0 and 2.
	{
		MEMFreeToDefaultHeap(&ioThread);
		MEM1_free(writeQueueMutex);
		return false;
	}
	OSSetThreadName(&ioThread, "NUSspli I/O");
	
	OSFastMutex_Init(writeQueueMutex, "NUSspli I/O Queue");
	OSFastMutex_Unlock(writeQueueMutex);
	
	ioRunning = true;
	OSResumeThread(&ioThread);
	return true;
}

void shutdownIOThread()
{
	if(!ioRunning)
		return;
	
	ioRunning = false;
	int ret;
	OSJoinThread(&ioThread, &ret);
	debugPrintf("I/O thread returned: %d", ret);
	
	flushIOQueue();
	
	MEM1_free(writeQueueMutex);
}

#ifdef NUSSPLI_DEBUG
size_t maxWriteQueueSize = 0;
#endif

size_t addToIOQueue(const void *buf, size_t size, size_t n, FILE *file)
{
	WriteQueueEntry *toAdd;
	void *newBuf;
	bool fast;
	
	if(buf != NULL)
	{
		size *= n;
		newBuf = MEM1_alloc(size, 4);
		fast = newBuf != NULL;
		if(fast)
		{
			toAdd = MEM1_alloc(sizeof(WriteQueueEntry), 4);
			if(toAdd == NULL)
			{
				MEM1_free(newBuf);
				fast = false;
			}
		}
		if(!fast)
		{
			newBuf = MEMAllocFromDefaultHeap(size);
			if(newBuf == NULL)
				return 0;
			
			toAdd = MEMAllocFromDefaultHeap(sizeof(WriteQueueEntry));
			if(toAdd == NULL)
			{
				MEMFreeToDefaultHeap(newBuf);
				return 0;
			}
#ifdef NUSSPLI_DEBUG
			if((writeQueueSize + size) >> 20 > maxWriteQueueSize)
			{
				maxWriteQueueSize = (writeQueueSize + size) >> 20;
				debugPrintf("I/O queue size: %d MB", maxWriteQueueSize);
			}
#endif
		}
		
		OSBlockMove(newBuf, buf, size, false);
	}
	else
	{
		toAdd = MEM1_alloc(sizeof(WriteQueueEntry), 4);
		fast = toAdd != NULL;
		if(!fast)
		{
			toAdd = MEMAllocFromDefaultHeap(sizeof(WriteQueueEntry));
			if(toAdd == NULL)
				return 0;
		}
		
		newBuf = NULL;
	}
	
	toAdd->buf = newBuf;
	toAdd->file = file;
	toAdd->size = size;
	toAdd->fast = fast;
	toAdd->next = NULL;
	
	lockIOQueueAgressive();
	if(newBuf != NULL)
	{
		while(writeQueueSize > MAX_IO_QUEUE_SIZE)
		{
			OSFastMutex_Unlock(writeQueueMutex);
			debugPrintf("Waiting for free slot...");
			OSSleepTicks(256);
			lockIOQueueAgressive();
		}
		
		writeQueueSize += size;
	}
	
	if(writeQueue == NULL)
		writeQueue = lastWriteQueueEntry = toAdd;
	else
		lastWriteQueueEntry = lastWriteQueueEntry->next = toAdd;
	
	OSFastMutex_Unlock(writeQueueMutex);
	return size;
}

void flushIOQueue()
{
	debugPrintf("Flushing...");
	lockIOQueueAgressive();
	while(writeQueue != NULL)
		executeIOQueue(false);
	
	OSFastMutex_Unlock(writeQueueMutex);
}
