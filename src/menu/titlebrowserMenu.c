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

#include <deinstaller.h>
#include <downloader.h>
#include <input.h>
#include <renderer.h>
#include <status.h>
#include <titles.h>
#include <usb.h>
#include <utils.h>
#include <menu/download.h>
#include <menu/titlebrowser.h>
#include <menu/utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

#define MAX_TITLEBROWSER_LINES (MAX_LINES - 4)

bool vibrateWhenFinished2; // TODO
TitleEntry *titleEntrys;
TitleEntry *filteredTitleEntries;
size_t filteredTitleEntrySize;

void drawTBMenuFrame2(const TitleEntry *entry, bool installed, const char *folderName, bool usbMounted, bool dlToUSB, bool keepFiles)
{
	startNewFrame();
	textToFrame(0, 0, "Name:");
	
	char *toFrame = getToFrameBuffer();
	strcpy(toFrame, entry->name);
	char *tid = hex(entry->tid, 16);
	if(tid != NULL)
	{
		strcat(toFrame, " [");
		strcat(toFrame, tid);
		strcat(toFrame, "]");
		MEMFreeToDefaultHeap(tid);
	}
	textToFrame(1, 3, toFrame);
	
	textToFrame(2, 0, "Custom folder name [Only text and numbers]:");
	textToFrame(3, 3, folderName);
	
	strcpy(toFrame, "Press \uE045 to ");
	strcat(toFrame, vibrateWhenFinished2 ? "deactivate" : "activate");
	strcat(toFrame, " the vibration after installing");
	textToFrame(MAX_LINES - 1, 0, toFrame); //Thinking to change this to activate HOME Button led
	
	int line = MAX_LINES - 2;
	if(usbMounted)
	{
		strcpy(toFrame, "Press \uE046 to download to ");
		strcat(toFrame, dlToUSB ? "SD" : "USB");
		textToFrame(line--, 0, toFrame);
	}
	if(dlToUSB)
		textToFrame(line--, 0, "WARNING: Files on USB will always be deleted after installing!");
	else
	{
		strcpy(toFrame, "Press \uE07B to ");
		strcat(toFrame, keepFiles ? "delete" : "keep");
		strcat(toFrame, " downloaded files after the installation");
		textToFrame(line--, 0, toFrame);
	}
	
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	textToFrame(line--, 0, "Press \uE07A to set a custom name to the download folder");
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	textToFrame(line--, 0, "Press \uE001 to return");
	
	strcpy(toFrame, "Press \uE003 to download to ");
	strcat(toFrame, dlToUSB ? "USB" : "SD");
	strcat(toFrame, " only");
	textToFrame(line--, 0, toFrame);
	
	textToFrame(line--, 0, "Press \uE002 to install to NAND");
	if(usbMounted)
		textToFrame(line--, 0, "Press \uE000 to install to USB");
	
	if(installed)
		textToFrame(line--, 0, "Press \uE079 to uninstall");
	
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	if(!dlToUSB)
		textToFrame(--line, 0, "WARNING: Downloading to SD is slow!");
	
	drawFrame();
}

void drawTBMenuFrame(const size_t pos, const size_t cursor, const char* search)
{
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
		for(size_t i = 0 ; i < filteredTitleEntrySize; i++)
		{
			ss = strlen(titleEntrys[i].name);
			char tmpName[ss + 1];
			for(size_t j = 0; j < ss; j++)
				tmpName[j] = tolower(titleEntrys[i].name[j]);
			tmpName[ss] = '\0';
			
			if(strstr(tmpName, lowerSearch) == NULL)
				continue;
			
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
	for(size_t i = 0; i < max; i++)
	{
		l = i + 2;
		if(cursor == i)
			arrowToFrame(l, 1);
		
		j = i + pos;
		if(MCP_GetTitleInfo(mcpHandle, filteredTitleEntries[j].tid, &titleList) == 0)
			checkmarkToFrame(l, 5);
		
		flagToFrame(l, 9, filteredTitleEntries[j].region);
		textToFrame(l, 13, filteredTitleEntries[j].name);
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
	
	MCPTitleListType titleList;
	bool installed = MCP_GetTitleInfo(mcpHandle, entry->tid, &titleList) == 0;
	if(!installed)
		titleList.titleId = entry->tid;
	
	bool usbMounted = mountUSB();
	bool dlToUSB = usbMounted;
	bool keepFiles = true;
	char folderName[FILENAME_MAX - 11];
	folderName[0] = '\0';
	
	drawTBMenuFrame2(entry, installed, folderName, usbMounted, dlToUSB, keepFiles);
	
	bool loop = true;
	bool inst, toUSB;
	bool uninstall = false;
	redraw = false;
	while(loop && AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawTBMenuFrame2(entry, installed, folderName, usbMounted, dlToUSB, keepFiles);
		
		showFrame();
		
		if(vpad.trigger & VPAD_BUTTON_B)
		{
			MEMFreeToDefaultHeap(filteredTitleEntries);
			return;
		}
		
		if(usbMounted && vpad.trigger & VPAD_BUTTON_A)
		{
			inst = toUSB = true;
			loop = false;
		}
		else if(vpad.trigger & VPAD_BUTTON_Y)
			inst = toUSB = loop = false;
		else if(vpad.trigger & VPAD_BUTTON_X)
		{
			inst = true;
			toUSB = loop = false;
		}
		
		if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			if(!showKeyboard(KEYBOARD_TYPE_NORMAL, folderName, CHECK_ALPHANUMERICAL, FILENAME_MAX - 11, false, folderName, NULL))
				folderName[0] = '\0';
			redraw = true;
		}
		
		if(usbMounted && vpad.trigger & VPAD_BUTTON_MINUS)
		{
			dlToUSB = !dlToUSB;
			keepFiles = !dlToUSB;
			redraw = true;
		}
		if(vpad.trigger & VPAD_BUTTON_PLUS)
		{
			vibrateWhenFinished2 = !vibrateWhenFinished2;
			redraw = true;
		}
		if(!dlToUSB && vpad.trigger & VPAD_BUTTON_LEFT)
		{
			keepFiles = !keepFiles;
			redraw = true;
		}
		if(installed && vpad.trigger & VPAD_BUTTON_UP)
		{
			uninstall = true;
			redraw = loop = false;
		}
		
		if(redraw)
		{
			drawTBMenuFrame2(entry, installed, folderName, usbMounted, dlToUSB, keepFiles);
			redraw = false;
		}
	}
	
	MEMFreeToDefaultHeap(filteredTitleEntries);
	if(!AppRunning())
		return;
	
	if(uninstall)
	{
		deinstall(titleList, true);
		return;
	}
	
	char *tid = hex(titleList.titleId, 16);
	if(tid != NULL)
	{
		downloadTitle(tid, "\0", folderName, inst, dlToUSB, toUSB, keepFiles);
		MEMFreeToDefaultHeap(tid);
	}
	// TODO: Else
}
