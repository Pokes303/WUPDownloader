/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
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

#include <coreinit/core.h>
#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <renderer.h>
#include <crypto.h>
#include <file.h>
#include <ioQueue.h>
#include <thread.h>
#include <utils.h>

#define IOT_STACK_SIZE		0x1000
#define MAX_IO_QUEUE_ENTRIES	(512 * (IO_MAX_FILE_BUFFER / (1024 * 1024))) // 512 MB
#define IO_MAX_FILE_BUFFER	(1024 * 1024) // 1 MB

typedef struct WUT_PACKED
{
	volatile bool ready;
	volatile FSFileHandle *file;
	volatile size_t size;
	volatile uint8_t *buf;
} WriteQueueEntry;

static OSThread *ioThread;
static volatile bool ioRunning = false;
static spinlock ioWriteLock;

static WriteQueueEntry *queueEntries;
static volatile uint32_t activeReadBuffer;
static volatile uint32_t activeWriteBuffer;

static volatile FSStatus fwriteErrno = FS_STATUS_OK;
static volatile int fwriteOverlay = -1;

extern FSClient *__wut_devoptab_fs_client;

static int ioThreadMain(int argc, const char **argv)
{
	uint32_t asl;
	WriteQueueEntry *entry;
	FSCmdBlock cmdBlk;
	FSStatus err;

	FSInitCmdBlock(&cmdBlk);
	FSSetCmdPriority(&cmdBlk, 1);

	while(ioRunning && fwriteErrno == FS_STATUS_OK)
	{
		asl = activeWriteBuffer;
		entry = queueEntries + asl;
		if(!entry->ready)
		{
			OSSleepTicks(256);
			continue;
		}

		if(entry->size) // WRITE command
		{
			err = FSWriteFile(__wut_devoptab_fs_client, &cmdBlk, (uint8_t *)entry->buf, entry->size, 1, *entry->file, 0, FS_ERROR_FLAG_ALL);
			if(err != 1)
				fwriteErrno = err;

			entry->size = 0;
		}
		else // Close command
		{
			OSTime t = OSGetTime();
			err = FSCloseFile(__wut_devoptab_fs_client, &cmdBlk, *entry->file, FS_ERROR_FLAG_ALL);
			if(err != FS_STATUS_OK)
				fwriteErrno = err;

			t = OSGetTime() - t;
			addEntropy(&t, sizeof(OSTime));
		}

		if(++asl == MAX_IO_QUEUE_ENTRIES)
			asl = 0;

		activeWriteBuffer = asl;
		entry->file = NULL;
		entry->ready = false;
	}
	
	return 0;
}

bool initIOThread()
{
	queueEntries = MEMAllocFromDefaultHeap(MAX_IO_QUEUE_ENTRIES * sizeof(WriteQueueEntry));
	if(queueEntries != NULL)
	{
		for(int i = 0; i < MAX_IO_QUEUE_ENTRIES; ++i)
		{
			queueEntries[i].buf = MEMAllocFromDefaultHeapEx(IO_MAX_FILE_BUFFER, 0x40);
			if(queueEntries[i].buf == NULL)
			{
				for(--i; i > -1; --i)
					MEMFreeToDefaultHeap((void *)queueEntries[i].buf);

				MEMFreeToDefaultHeap(queueEntries);
				return false;
			}

			queueEntries[i].ready = false;
			queueEntries[i].file = NULL;
			queueEntries[i].size = 0;
		}

		spinCreateLock(ioWriteLock, SPINLOCK_FREE);
		activeReadBuffer = activeWriteBuffer = 0;
		ioRunning = true;

		ioThread = startThread("NUSspli I/O", THREAD_PRIORITY_HIGH, IOT_STACK_SIZE, ioThreadMain, 0, NULL, OS_THREAD_ATTRIB_AFFINITY_CPU0); // We move this to core 0 for maximum performance. Later on move it back to core 1 as we want download threads on core 0 and 2.
		if(ioThread != NULL)
			return true;

		for(int i = 0; i < MAX_IO_QUEUE_ENTRIES; ++i)
			MEMFreeToDefaultHeap((void *)queueEntries[i].buf);

		MEMFreeToDefaultHeap(queueEntries);
	}
	return false;
}

bool checkForQueueErrors()
{
	if(fwriteErrno != FS_STATUS_OK)
	{
		if(fwriteOverlay == -1 && OSIsMainCore())
		{
			OSSleepTicks(OSMillisecondsToTicks(20)); // Lazy race condition prevention
			char errMsg[1024];
			sprintf(errMsg, "Write error:\n%s\n\nThis is an unrecoverable error!", translateFSErr(fwriteErrno));
			fwriteOverlay = addErrorOverlay(errMsg);
		}
		return true;
	}
	return false;
}

void shutdownIOThread()
{
	if(!ioRunning)
		return;

	if(!checkForQueueErrors())
	{
		spinLock(ioWriteLock);

		while(queueEntries[activeWriteBuffer].ready)
			;
	}
	
	ioRunning = false;
#ifdef NUSSPLI_DEBUG
	int ret;
	stopThread(ioThread, &ret);
	debugPrintf("I/O thread returned: %d", ret);
#else
	stopThread(ioThread, NULL);
#endif
	for(int i = 0; i < MAX_IO_QUEUE_ENTRIES; ++i)
		MEMFreeToDefaultHeap((void *)queueEntries[i].buf);

	MEMFreeToDefaultHeap(queueEntries);
}

#ifdef NUSSPLI_DEBUG
bool queueStalled = false;
#endif
size_t addToIOQueue(const void *buf, size_t size, size_t n, FSFileHandle *file)
{
	if(checkForQueueErrors())
		return 0;

    WriteQueueEntry *entry;
		
retryAddingToQueue:
	
	while(!spinTryLock(ioWriteLock))
		if(!ioRunning)
			return 0;
	
    entry = queueEntries + activeReadBuffer;
	if(entry->ready)
	{
        spinReleaseLock(ioWriteLock);
#ifdef NUSSPLI_DEBUG
		if(!queueStalled)
		{
			debugPrintf("Waiting for free slot...");
			queueStalled = true;
		}
#endif
		goto retryAddingToQueue; // We use goto here instead of just calling addToIOQueue again to not overgrow the stack.
	}

#ifdef NUSSPLI_DEBUG
	if(queueStalled)
	{
		debugPrintf("Slot free!");
		queueStalled = false;
	}
#endif

	if(buf != NULL)
	{
		size *= n;
		if(size == 0)
		{
			n = 0;
			goto queueExit;
		}

		size_t ns = entry->size + size;
		if(ns > IO_MAX_FILE_BUFFER)
		{
			spinReleaseLock(ioWriteLock);
			ns = IO_MAX_FILE_BUFFER - entry->size;
			n = addToIOQueue(buf, 1, ns, file);
			size -= ns;
			if(size != 0)
			{
				const uint8_t *newPtr = buf;
				newPtr += ns;
				n += addToIOQueue((const void *)newPtr, 1, size, file);
			}

			return n;
		}

		OSBlockMove((void *)(entry->buf + entry->size), buf, size, false);
		entry->size = ns;
		if(ns != IO_MAX_FILE_BUFFER) // ns < IO_MAX_FILE_BUFFER
			goto queueExit;
	}
	else
	{
		if(entry->size != 0)
		{
			// TODO: Deduplicate code
			entry->file = file;
			entry->ready = true;

			if(++activeReadBuffer == MAX_IO_QUEUE_ENTRIES)
				activeReadBuffer = 0;

			spinReleaseLock(ioWriteLock);
			return addToIOQueue(NULL, 0, 0, file);
		}

		entry->size = 0;
	}

	entry->file = file;
	entry->ready = true;
	
	if(++activeReadBuffer == MAX_IO_QUEUE_ENTRIES)
		activeReadBuffer = 0;

queueExit:
	spinReleaseLock(ioWriteLock);
	return n;
}

void flushIOQueue()
{
	if(checkForQueueErrors())
		return;

	int ovl = addErrorOverlay("Flushing queue, please wait...");
	debugPrintf("Flushing...");
	spinLock(ioWriteLock);

	while(queueEntries[activeWriteBuffer].ready)
	{
		OSSleepTicks(1024);
		if(checkForQueueErrors())
			break;
	}

	spinReleaseLock(ioWriteLock);
	removeErrorOverlay(ovl);

	checkForQueueErrors();
}

FSFileHandle *openFile(const char *path, const char *mode)
{
	if(checkForQueueErrors())
		return NULL;
	
	OSTime t = OSGetTime();
	FSFileHandle *ret = MEMAllocFromDefaultHeap(sizeof(FSFileHandle));
	if(ret == NULL)
		return NULL;

	FSStatus s = FSOpenFile(__wut_devoptab_fs_client, getCmdBlk(), path, mode, ret, FS_ERROR_FLAG_ALL);
	if(s == FS_STATUS_OK)
		return ret;

	t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));

	MEMFreeToDefaultHeap(ret);
	debugPrintf("Error opening %s: %s!", path, translateFSErr(s));
	return NULL;
}
