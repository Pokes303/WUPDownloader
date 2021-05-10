/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
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

#include <input.h>
#include <renderer.h>
#include <status.h>
#include <titles.h>
#include <usb.h>
#include <utils.h>
#include <menu/download.h>
#include <menu/predownload.h>
#include <menu/titlebrowser.h>
#include <menu/utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

#define MAX_TITLEBROWSER_LINES (MAX_LINES - 4)

TitleEntry *titleEntrys;
TitleEntry *filteredTitleEntries;

size_t filteredTitleEntrySize;

void drawTBMenuFrame(const size_t pos, const size_t cursor, const char* search)
{
	if(!isAroma())
		unmountUSB();
	
	startNewFrame();
	textToFrame(0, 6, "Select a title:");
	
	boxToFrame(1, MAX_LINES - 2);
	
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, "Press \uE000 to select || \uE001 to return || \uE002 to enter a title ID || \uE003 to search");
	
	filteredTitleEntrySize = getTitleEntriesSize();
	
	if(search[0] != '\0')
	{
		size_t ts = strlen(search);
		char lowerSearch[ts + 1];
		for(size_t i = 0; i < ts; i++)
			lowerSearch[i] = tolower(search[i]);
		lowerSearch[ts] = '\0';
		
		ts = 0;
		size_t ss;
		char *ptr[2];
		bool found;
		for(size_t i = 0 ; i < filteredTitleEntrySize; i++)
		{
			ss = strlen(titleEntrys[i].name);
			char tmpName[ss + 1];
			for(size_t j = 0; j < ss; j++)
				tmpName[j] = tolower(titleEntrys[i].name[j]);
			tmpName[ss] = '\0';
			
			ptr[0] = lowerSearch;
			ptr[1] = strstr(ptr[0], " ");
			found = true;
			while(found)
			{
				if(ptr[1] != NULL)
					ptr[1][0] = '\0';
				
				if(strstr(tmpName, ptr[0]) == NULL)
					found = false;
				
				if(ptr[1] != NULL)
				{
					ptr[1][0] = ' ';
					if(found)
					{
						ptr[0] = ptr[1] + 1;
						ptr[1] = strstr(ptr[0], " ");
					}
				}
				else
					break;
			};
			
			if(found)
				filteredTitleEntries[ts++] = titleEntrys[i];
		}
		
		filteredTitleEntrySize = ts;
	}
	else
		for(size_t i = 0; i < filteredTitleEntrySize; i++)
			filteredTitleEntries[i] = titleEntrys[i];
	
	size_t max = MAX_TITLEBROWSER_LINES < filteredTitleEntrySize ? MAX_TITLEBROWSER_LINES : filteredTitleEntrySize;
	size_t j, l;
	MCPTitleListType titleList;
	char *toFrame = getToFrameBuffer();
	for(size_t i = 0; i < max; i++)
	{
		l = i + 2;
		if(cursor == i)
			arrowToFrame(l, 1);
		
		j = i + pos;
		if(MCP_GetTitleInfo(mcpHandle, filteredTitleEntries[j].tid, &titleList) == 0)
			checkmarkToFrame(l, 5);
		
		flagToFrame(l, 9, filteredTitleEntries[j].region);
		
		if(filteredTitleEntries[j].isDLC)
			strcpy(toFrame, "[DLC] ");
		else if(filteredTitleEntries[j].isUpdate)
			strcpy(toFrame, "[UPD] ");
		else
		{
			textToFrame(l, 13, filteredTitleEntries[j].name);
			continue;
		}
		
		strcat(toFrame, filteredTitleEntries[j].name);
		textToFrame(l, 13, toFrame);
	}
	drawFrame();
}

void titleBrowserMenu()
{
	size_t cursor = 0;
	size_t pos = 0;
	char search[129];
	search[0] = '\0';
	titleEntrys = getTitleEntries();
	filteredTitleEntrySize = getTitleEntriesSize();
	filteredTitleEntries = MEMAllocFromDefaultHeap(filteredTitleEntrySize * sizeof(TitleEntry));
	if(filteredTitleEntries == NULL)
	{
		debugPrintf("Titlebrowser: OUT OF MEMORY!");
		return;
	}
	
	drawTBMenuFrame(pos, cursor, search);
	
	bool mov = filteredTitleEntrySize >= MAX_TITLEBROWSER_LINES;
	bool redraw = false;
	TitleEntry *entry;
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawTBMenuFrame(pos, cursor, search);
		
		showFrame();
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			entry = filteredTitleEntries + cursor + pos;
			break;
		}
		
		if(vpad.trigger & VPAD_BUTTON_B)
		{
			MEMFreeToDefaultHeap(filteredTitleEntries);
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
						cursor = MAX_TITLEBROWSER_LINES - 1;
						pos = filteredTitleEntrySize - MAX_TITLEBROWSER_LINES;
					}
				}
				else
					cursor = filteredTitleEntrySize - 1;
			}
			
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			if(cursor >= filteredTitleEntrySize - 1 || cursor >= MAX_TITLEBROWSER_LINES - 1)
			{
				if(mov && pos < filteredTitleEntrySize - MAX_TITLEBROWSER_LINES)
					pos++;
				else
					cursor = pos = 0;
			}
			else
				cursor++;
			
			redraw = true;
		}
		else if(mov)
		{
			if(vpad.trigger & VPAD_BUTTON_RIGHT)
			{
				pos += MAX_TITLEBROWSER_LINES;
				if(pos >= filteredTitleEntrySize - MAX_TITLEBROWSER_LINES)
					pos = 0;
				cursor = 0;
				redraw = true;
			}
			else if(vpad.trigger & VPAD_BUTTON_LEFT)
			{
				if(pos >= MAX_TITLEBROWSER_LINES)
					pos -= MAX_TITLEBROWSER_LINES;
				else
					pos = filteredTitleEntrySize - MAX_TITLEBROWSER_LINES;
				cursor = 0;
				redraw = true;
			}
		}
		
		if(vpad.trigger & VPAD_BUTTON_X)
		{
			MEMFreeToDefaultHeap(filteredTitleEntries);
			if(downloadMenu())
				return;
			titleBrowserMenu();
		}
		if(vpad.trigger & VPAD_BUTTON_Y)
		{
			showKeyboard(KEYBOARD_TYPE_NORMAL, search, CHECK_NONE, 128, false, search, "Search");
			cursor = pos = 0;
			redraw = true;
		}
		
		
		if(redraw)
		{
			drawTBMenuFrame(pos, cursor, search);
			mov = filteredTitleEntrySize > MAX_TITLEBROWSER_LINES;
			redraw = false;
		}
	}
	if(!AppRunning())
	{
		MEMFreeToDefaultHeap(filteredTitleEntries);
		return;
	}
	
	predownloadMenu(entry);
	MEMFreeToDefaultHeap(filteredTitleEntries);
}
