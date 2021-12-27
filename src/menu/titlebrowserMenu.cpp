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

#include <filesystem.h>
#include <input.h>
#include <renderer.h>
#include <status.h>
#include <titles.h>
#include <utils.h>
#include <menu/download.h>
#include <menu/predownload.h>
#include <menu/titlebrowser.h>
#include <menu/utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <wctype.h>
#include <string>
#include <locale>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

#define MAX_TITLEBROWSER_LINES (MAX_LINES - 4)

TitleEntry *filteredTitleEntries;
size_t filteredTitleEntrySize;

void drawTBMenuFrame(const TITLE_CATEGORY tab, const size_t pos, const size_t cursor, char16_t *search)
{
	if(!isAroma())
		unmountAll();
	
	startNewFrame();
	
	// Games, Updates, DLC, Demos, All
	const char *tabLabels[5] = { "Games", "Updates", "DLC", "Demos", "All" };
	for(int i = 0; i < 5; ++i)
		tabToFrame(0, i, (char *)tabLabels[i], i == tab);
	
	boxToFrame(1, MAX_LINES - 2);
	
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, "Press " BUTTON_A " to select || " BUTTON_B " to return || " BUTTON_X " to enter a title ID || " BUTTON_Y " to search");
	
	filteredTitleEntrySize = getTitleEntriesSize(tab);
	TitleEntry *titleEntrys = getTitleEntries(tab);
	
	if(search[0] != u'\0')
	{
		size_t ts = strlen16(search);
		char16_t lowerSearch[ts + 1];
		ts = 0;
		do
			lowerSearch[ts] = tolower16(search[ts]);
		while(search[ts++]);
		
		ts = 0;
		size_t ss;
		std::u16string u16s;
		char16_t *ptr[2];
		bool found;
		std::wstring_convert<codecvt<char16_t, char, std::mbstate_t>, char16_t> conv;
		for(size_t i = 0 ; i < filteredTitleEntrySize; ++i)
		{
			ss = strlen(titleEntrys[i].name);
			char tmpName[ss + 1];
			for(size_t j = 0; j < ss; ++j)
				tmpName[j] = tolower(titleEntrys[i].name[j]);
			
			u16s = conv.from_bytes(tmpName, tmpName+ss);
			char16_t *tmp16Name = (char16_t *)u16s.c_str();
			
			ptr[0] = lowerSearch;
			ptr[1] = str16str(ptr[0], (char16_t *)u" ");
			do
			{
				if(ptr[1] != NULL)
					ptr[1][0] = u'\0';
				
				found = str16str(tmp16Name, ptr[0]) != NULL;
				
				if(ptr[1] != NULL)
				{
					ptr[1][0] = u' ';
					if(found)
					{
						ptr[0] = ptr[1] + 1;
						ptr[1] = str16str(ptr[0], (char16_t *)u" ");
					}
				}
				else
					break;
			}
			while(found);
			
			if(found)
				filteredTitleEntries[ts++] = titleEntrys[i];
		}
		
		filteredTitleEntrySize = ts;
	}
	else
		for(size_t i = 0; i < filteredTitleEntrySize; ++i)
			filteredTitleEntries[i] = titleEntrys[i];
	
	size_t max = MAX_TITLEBROWSER_LINES < filteredTitleEntrySize ? MAX_TITLEBROWSER_LINES : filteredTitleEntrySize;
	size_t j, l;
	MCPTitleListType titleList;
	char *toFrame = getToFrameBuffer();
	for(size_t i = 0; i < max; ++i)
	{
		l = i + 2;
		if(cursor == i)
			arrowToFrame(l, 1);
		
		j = i + pos;
		if(MCP_GetTitleInfo(mcpHandle, filteredTitleEntries[j].tid, &titleList) == 0)
			checkmarkToFrame(l, 4);
		
		flagToFrame(l, 7, filteredTitleEntries[j].region);
		
		if(tab == TITLE_CATEGORY_ALL)
		{
			if(filteredTitleEntries[j].isDLC)
				strcpy(toFrame, "[DLC] ");
			else if(filteredTitleEntries[j].isUpdate)
				strcpy(toFrame, "[UPD] ");
			else
			{
				textToFrame(l, 10, filteredTitleEntries[j].name);
				continue;
			}
			
			strcat(toFrame, filteredTitleEntries[j].name);
			textToFrameCut(l, 10, toFrame, (1280 - (FONT_SIZE << 1)) - (getSpaceWidth() * 11));
		}
		else
			textToFrameCut(l, 10, filteredTitleEntries[j].name, (1280 - (FONT_SIZE << 1)) - (getSpaceWidth() * 11));
	}
	drawFrame();
}

void titleBrowserMenu()
{
	filteredTitleEntrySize = getTitleEntriesSize(TITLE_CATEGORY_ALL);
	filteredTitleEntries = (TitleEntry *)MEMAllocFromDefaultHeap(filteredTitleEntrySize * sizeof(TitleEntry));
	if(filteredTitleEntries == NULL)
	{
		debugPrintf("Titlebrowser: OUT OF MEMORY!");
		return;
	}
	
	TITLE_CATEGORY tab = TITLE_CATEGORY_GAME;
	size_t cursor = 0;
	size_t pos = 0;
	char16_t search[129];
	search[0] = u'\0';
	
	drawTBMenuFrame(tab, pos, cursor, search);
	
	bool mov = filteredTitleEntrySize >= MAX_TITLEBROWSER_LINES;
	bool redraw = false;
	TitleEntry *entry;
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawTBMenuFrame(tab, pos, cursor, search);
		
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
			if(!downloadMenu())
				titleBrowserMenu();
			return;
		}
		if(vpad.trigger & VPAD_BUTTON_Y)
		{
			char16_t oldSearch[sizeof(search)];
			str16cpy(oldSearch, search);
			showKeyboard(KEYBOARD_LAYOUT_NORMAL, KEYBOARD_TYPE_NORMAL, (char *)search, CHECK_NONE, 128, false, (const char *)search, "Search");
			if(str16cmp(oldSearch, search) != 0)
			{
				cursor = pos = 0;
				redraw = true;
			}
		}
		
		if(vpad.trigger & VPAD_BUTTON_R || vpad.trigger & VPAD_BUTTON_ZR || vpad.trigger & VPAD_BUTTON_PLUS)
		{
			size_t tt = (size_t)tab;
			if(++tt > TITLE_CATEGORY_ALL)
				tt = (size_t)TITLE_CATEGORY_GAME;
			
			tab = (TITLE_CATEGORY)tt;
			cursor = pos = 0;
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_L || vpad.trigger & VPAD_BUTTON_ZL || vpad.trigger & VPAD_BUTTON_MINUS)
		{
			if(tab == TITLE_CATEGORY_GAME)
				tab = TITLE_CATEGORY_ALL;
			else
			{
				size_t tt = (size_t)tab;
				tt--;
				tab = (TITLE_CATEGORY)tt;
			}
			
			cursor = pos = 0;
			redraw = true;
		}
		
		
		if(redraw)
		{
			drawTBMenuFrame(tab, pos, cursor, search);
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
