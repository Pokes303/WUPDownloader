/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
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

#include <deinstaller.h>
#include <input.h>
#include <renderer.h>
#include <status.h>
#include <utils.h>
#include <menu/insttitlebrowser.h>
#include <menu/utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

#define MAX_ITITLEBROWSER_LINES (MAX_LINES - 4)

static MCPTitleListType *ititleEntries;
static size_t ititleEntrySize;

static void drawITBMenuFrame(const size_t pos, const size_t cursor)
{
	startNewFrame();
	boxToFrame(1, MAX_LINES - 2);
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, "Press " BUTTON_A " to delete || " BUTTON_B " to return");
	
	size_t max = MAX_ITITLEBROWSER_LINES < ititleEntrySize ? MAX_ITITLEBROWSER_LINES : ititleEntrySize;
	size_t j, l;
	char *toFrame = getToFrameBuffer();
	TitleEntry *e;
	char tid[17];
	char *name;
	for(size_t i = 0; i < max; ++i)
	{
		l = i + 2;
		if(cursor == i)
			arrowToFrame(l, 1);
		
		j = i + pos;
		e = getTitleEntryByTid(ititleEntries[j].titleId);
		flagToFrame(l, 7, e == NULL ? TITLE_REGION_UNKNOWN : e->region);

		if(ititleEntries[j].indexedDevice[0] == 'u')
			deviceToFrame(l, 4, DEVICE_TYPE_USB);
		else if(ititleEntries[j].indexedDevice[0] == 'm')
			deviceToFrame(l, 4, DEVICE_TYPE_NAND);
		else
			deviceToFrame(l, 4, DEVICE_TYPE_UNKNOWN); // TODO: bt. drh, slc
		
		if(e == NULL)
		{
			hex(ititleEntries[j].titleId, 16, tid);
			name = tid;
			toFrame[0] = '\0';
		}
		else
		{
			name = e->name;
			if(e->isDLC)
				strcpy(toFrame, "[DLC] ");
			else if(e->isUpdate)
				strcpy(toFrame, "[UPD] ");
			else
				toFrame[0] = '\0';
		}

		strcat(toFrame, name);
		textToFrameCut(l, 10, toFrame, (1280 - (FONT_SIZE << 1)) - (getSpaceWidth() * 11));
	}
	drawFrame();
}

void ititleBrowserMenu()
{
	int32_t r = MCP_TitleCount(mcpHandle);
	if(r < 0)
	{
		// TODO
		return;
	}

	uint32_t s = sizeof(MCPTitleListType) * (uint32_t)r;
	ititleEntries = (MCPTitleListType *)MEMAllocFromDefaultHeap(s);
	if(ititleEntries == NULL)
	{
		debugPrintf("Insttitlebrowser: OUT OF MEMORY!");
		return;
	}

	r = MCP_TitleList(mcpHandle, &s, ititleEntries, s);
	if(r < 0)
	{
		// TODO
		MEMFreeToDefaultHeap(ititleEntries);
		return;
	}

	ititleEntrySize = s;
	size_t cursor = 0;
	size_t pos = 0;
	
	drawITBMenuFrame(pos, cursor);
	
	bool mov = ititleEntrySize >= MAX_ITITLEBROWSER_LINES;
	bool redraw = false;
	MCPTitleListType *entry;
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawITBMenuFrame(pos, cursor);
		
		showFrame();
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			entry = ititleEntries + cursor + pos;
			break;
		}
		
		if(vpad.trigger & VPAD_BUTTON_B)
		{
			MEMFreeToDefaultHeap(ititleEntries);
			return;
		}
		
		if(vpad.trigger & VPAD_BUTTON_UP)
		{
			if(cursor)
				cursor--;
			else
			{
				if(mov)
				{
					if(pos)
						pos--;
					else
					{
						cursor = MAX_ITITLEBROWSER_LINES - 1;
						pos = ititleEntrySize - MAX_ITITLEBROWSER_LINES;
					}
				}
				else
					cursor = ititleEntrySize - 1;
			}
			
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			if(cursor >= ititleEntrySize - 1 || cursor >= MAX_ITITLEBROWSER_LINES - 1)
			{
				if(mov && pos < ititleEntrySize - MAX_ITITLEBROWSER_LINES)
					++pos;
				else
					cursor = pos = 0;
			}
			else
				++cursor;
			
			redraw = true;
		}
		else if(mov)
		{
			if(vpad.trigger & VPAD_BUTTON_RIGHT)
			{
				pos += MAX_ITITLEBROWSER_LINES;
				if(pos >= ititleEntrySize - MAX_ITITLEBROWSER_LINES)
					pos = 0;
				cursor = 0;
				redraw = true;
			}
			else if(vpad.trigger & VPAD_BUTTON_LEFT)
			{
				if(pos >= MAX_ITITLEBROWSER_LINES)
					pos -= MAX_ITITLEBROWSER_LINES;
				else
					pos = ititleEntrySize - MAX_ITITLEBROWSER_LINES;
				cursor = 0;
				redraw = true;
			}
		}
		
		if(redraw)
		{
			drawITBMenuFrame(pos, cursor);
			mov = ititleEntrySize > MAX_ITITLEBROWSER_LINES;
			redraw = false;
		}
	}
	if(!AppRunning())
	{
		MEMFreeToDefaultHeap(ititleEntries);
		return;
	}
	
	deinstall(*entry, false);
	MEMFreeToDefaultHeap(ititleEntries);
}
