/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2022 V10lator <v10lator@myway.de>                         *
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

#include <limits.h>
#include <stdint.h>

#include <coreinit/memdefaultheap.h>

#include <renderer.h>
#include <staticMem.h>

static char *staticPointer;

bool initStaticMem()
{
	staticPointer = MEMAllocFromDefaultHeap((TO_FRAME_BUFFER_SIZE * 2) + (PATH_MAX * 3));
	return staticPointer != NULL;
}

void shutdownStaticMem()
{
	MEMFreeToDefaultHeap(staticPointer);
}

char *getStaticScreenBuffer()
{
	return staticPointer;
}

char *getStaticLineBuffer()
{
	return staticPointer + TO_FRAME_BUFFER_SIZE;
}

char *getStaticPathBuffer(uint32_t i)
{
	char *ptr = staticPointer + (TO_FRAME_BUFFER_SIZE * 2);
	for(uint32_t j = 0; j < i; ++j)
		ptr += PATH_MAX;

	return ptr;
}
