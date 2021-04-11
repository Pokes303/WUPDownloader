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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_TITLEBROWSER_LINES (MAX_LINES - 4)

bool vibrateWhenFinished2; // TODO

void drawTBMenuFrame2(const TitleEntry *entry, const char *folderName, bool usbMounted, bool dlToUSB, bool keepFiles)
{
	startNewFrame();
	textToFrame(0, 0, "Name:");
	
	char *toFrame = getToFrameBuffer();
	strcpy(toFrame, entry->name);
	strcat(toFrame, " [");
	strcat(toFrame, entry->tid);
	strcat(toFrame, "]");
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
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	drawFrame();
}

void drawTBMenuFrame(const size_t pos, const size_t cursor)
{
	startNewFrame();
	textToFrame(0, 6, "Select a title:");
	
	boxToFrame(1, MAX_LINES - 2);
	
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, "Press \uE000 to select || \uE001 to return || \uE002 to enter a title ID");
	
	TitleEntry *titleEntries = getTitleEntries();
	size_t titleEntriesSize = getTitleEntriesSize() - pos;
	size_t max = MAX_TITLEBROWSER_LINES < titleEntriesSize ? MAX_TITLEBROWSER_LINES : titleEntriesSize;
	char *toFrame = getToFrameBuffer();
	size_t j;
	for(size_t i = 0; i < max; i++)
	{
		j = i + pos;
		if(titleEntries[j].region != NULL)
		{
			toFrame[0] = '[';
			strcpy(toFrame + 1, titleEntries[j].region);
			strcat(toFrame, "] ");
			strcat(toFrame, titleEntries[j].name);
		}
		else
			strcpy(toFrame, titleEntries[j].name);
		
		textToFrame(i + 2, 8, toFrame);
		if(cursor == i)
			arrowToFrame(i + 2, 1);
	}
	drawFrame();
}

void titleBrowserMenu()
{
	size_t cursor = 0;
	size_t pos = 0;
	
	drawTBMenuFrame(pos, cursor);
	
	debugPrintf("getTitleEntriesSize(): %i", getTitleEntriesSize());
	bool mov = getTitleEntriesSize() >= MAX_TITLEBROWSER_LINES;
	bool redraw = false;
	TitleEntry *entry;
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawTBMenuFrame(pos, cursor);
		
		showFrame();
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			entry = getTitleEntries() + cursor + pos;
			break;
		}
		
		if(vpad.trigger & VPAD_BUTTON_B)
			return;
		
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
						pos = getTitleEntriesSize() - MAX_TITLEBROWSER_LINES;
					}
				}
				else
					cursor = getTitleEntriesSize() - 1;
			}
			
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			if(cursor >= getTitleEntriesSize() - 1 || cursor >= MAX_TITLEBROWSER_LINES - 1)
			{
				if(mov && pos < getTitleEntriesSize() - MAX_TITLEBROWSER_LINES)
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
				if(pos > getTitleEntriesSize() - MAX_TITLEBROWSER_LINES)
					pos = 0;
				redraw = true;
			}
			else if(vpad.trigger & VPAD_BUTTON_LEFT)
			{
				pos -= MAX_TITLEBROWSER_LINES;
				if(pos > getTitleEntriesSize() - MAX_TITLEBROWSER_LINES) //TODO
					pos = getTitleEntriesSize() - MAX_TITLEBROWSER_LINES;
				redraw = true;
			}
		}
		
		if(vpad.trigger & VPAD_BUTTON_X)
		{
			downloadMenu();
			return;
		}
		
		if(redraw)
		{
			drawTBMenuFrame(pos, cursor);
			redraw = false;
		}
	}
	if(!AppRunning())
		return;
	
	bool usbMounted = mountUSB();
	bool dlToUSB = false;
	bool keepFiles = true;
	char folderName[FILENAME_MAX - 11];
	folderName[0] = '\0';
	
	drawTBMenuFrame2(entry, folderName, usbMounted, dlToUSB, keepFiles);
	
	bool loop = true;
	bool inst, toUSB;
	redraw = false;
	while(loop && AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawTBMenuFrame2(entry, folderName, usbMounted, dlToUSB, keepFiles);
		
		showFrame();
		
		if(vpad.trigger & VPAD_BUTTON_B)
			return;
		
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
		if(vpad.trigger & VPAD_BUTTON_LEFT)
		{
			if(!dlToUSB)
			{
				keepFiles = !keepFiles;
				redraw = true;
			}
		}
		
		if(redraw)
		{
			drawTBMenuFrame2(entry, folderName, usbMounted, dlToUSB, keepFiles);
			redraw = false;
		}
	}
	if(!AppRunning())
		return;
	
	downloadTitle(entry->tid, "\0", folderName, inst, dlToUSB, toUSB, keepFiles);
}
