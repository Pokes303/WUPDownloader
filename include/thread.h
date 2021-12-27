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

#ifndef NUSSPLI_THREAD_H
#define NUSSPLI_THREAD_H

#include <wut-fixups.h>

#include <stdbool.h>
#include <stdint.h>

#include <coreinit/atomic.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>

#define DEFAULT_STACKSIZE	0x800000	// 8 MB
#define MIN_STACKSIZE		0x40000		// 256 KB

#ifdef __cplusplus
	extern "C" {
#endif

/*
 * Default Wii U thread priority is 16.
 * SDL changes its audio thread priority one lower, so to 15.
 * We want the SDL audio thread to be at THREAD_PRIORITY_LOW,
 * so that has to be 15, too
 */
typedef enum
{
    THREAD_PRIORITY_HIGH = 13,
    THREAD_PRIORITY_MEDIUM = 14,
    THREAD_PRIORITY_LOW = 15,
} THREAD_PRIORITY;

#define spinlock uint32_t

#define SPINLOCK_FREE	false
#define SPINLOCK_LOCKED	true

#define spinIsLocked(lock)		lock == SPINLOCK_LOCKED
#define spinTryLock(lock)		OSCompareAndSwapAtomic(&lock, SPINLOCK_FREE, SPINLOCK_LOCKED)
#define spinLock(lock)			while(!spinTryLock(lock)) {}
#define spinReleaseLock(lock)	lock = SPINLOCK_FREE

OSThread *startThread(const char *name, THREAD_PRIORITY priority, size_t stacksize, OSThreadEntryPointFn mainfunc, int argc, char *argv, OSThreadAttributes attribs);

#define stopThread(thread, ret)																	\
{																								\
	OSJoinThread(thread, ret);																	\
/*																								\
#ifdef NUSSPLI_DEBUG																			\
	debugPrintf("%s used %d bytes of stack", thread->name, OSCheckThreadStackUsage(thread));	\
#endif																							\
*/																								\
	OSDetachThread(thread);																		\
	MEMFreeToDefaultHeap(thread);																\
}

#ifdef __cplusplus
    }
#endif

#endif // NUSSPLI_THREAD_H
