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
#include <nn/acp/title.h>

#include <staticMem.h>
#include <renderer.h>
#include <titles.h>

static char *staticMemToFrameBuffer;
static char *staticMemLineBuffer;
static char *staticMemPathBuffer;
static ACPMetaXml *staticMeta;

bool initStaticMem()
{
	staticMemToFrameBuffer = MEMAllocFromDefaultHeap(TO_FRAME_BUFFER_SIZE);
	if(staticMemToFrameBuffer != NULL)
	{
		staticMemLineBuffer = MEMAllocFromDefaultHeap(TO_FRAME_BUFFER_SIZE);
		if(staticMemLineBuffer != NULL)
		{
			staticMemPathBuffer = MEMAllocFromDefaultHeap(PATH_MAX * 3);
			if(staticMemPathBuffer != NULL)
			{
				staticMeta = MEMAllocFromDefaultHeapEx(sizeof(ACPMetaXml), 0x40);
				if(staticMeta != NULL)
					return true;

				MEMFreeToDefaultHeap(staticMemPathBuffer);
			}

			MEMFreeToDefaultHeap(staticMemLineBuffer);
		}

		MEMFreeToDefaultHeap(staticMemToFrameBuffer);
	}

	return false;
}

void shutdownStaticMem()
{
	MEMFreeToDefaultHeap(staticMemToFrameBuffer);
	MEMFreeToDefaultHeap(staticMemLineBuffer);
	MEMFreeToDefaultHeap(staticMemPathBuffer);
	MEMFreeToDefaultHeap(staticMeta);
}

char *getStaticScreenBuffer()
{
	return staticMemToFrameBuffer;
}

char *getStaticLineBuffer()
{
	return staticMemLineBuffer;
}

char *getStaticPathBuffer(uint32_t i)
{
	return staticMemPathBuffer + i;
}

ACPMetaXml *getStaticMetaXmlBuffer()
{
	return staticMeta;
}
