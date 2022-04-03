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

#include <crypto.h>
#include <thread.h>
#include <utils.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

// Our current implementation glues the threads stack to the OSThread, returning something 100% OSThread compatible
OSThread *startThread(const char *name, THREAD_PRIORITY priority, size_t stacksize, OSThreadEntryPointFn mainfunc, int argc, char *argv, OSThreadAttributes attribs)
{
	if(name == NULL || stacksize < MIN_STACKSIZE)
		return NULL;

	OSTime t;
	addEntropy(&t, sizeof(OSTime));
	t = OSGetSystemTime();
	uint8_t *thread = MEMAllocFromDefaultHeapEx(sizeof(OSThread) + stacksize, 8);
	if(thread != NULL)
	{
		OSThread *ost = (OSThread *)thread;
		if(OSCreateThread(ost, mainfunc, argc, argv, thread + stacksize + sizeof(OSThread), stacksize, priority, attribs))
		{
			OSSetThreadName(ost, name);
#ifdef NUSSPLI_DEBUG
			if(!OSSetThreadStackUsage(ost))
				debugPrintf("Tracking stack usage failed for %s", name);
#endif
			OSResumeThread(ost);
			t = OSGetSystemTime() - t;
			addEntropy(&t, sizeof(OSTime));
			addEntropy(&(ost->id), sizeof(uint16_t));

			return ost;
		}
		MEMFreeToDefaultHeap(thread);
	}
	return NULL;
}
