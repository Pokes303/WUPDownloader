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
#include <menu/predownload.h>
#include <menu/utils.h>

#include <stdbool.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

bool vibrateWhenFinished = true;

void drawPDMenuFrame(const TitleEntry *entry, bool installed, const char *folderName, bool usbMounted, bool dlToUSB, bool keepFiles)
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
	
	textToFrame(2, 0, "Custom folder name [ASCII only]:");
	textToFrame(3, 3, folderName);
	
	strcpy(toFrame, "Press \uE045 to ");
	strcat(toFrame, vibrateWhenFinished ? "deactivate" : "activate");
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
	
	lineToFrame(line, SCREEN_COLOR_WHITE);
	
	drawFrame();
}

void predownloadMenu(const TitleEntry *entry)
{
	MCPTitleListType titleList;
	bool installed = MCP_GetTitleInfo(mcpHandle, entry->tid, &titleList) == 0;
	
	bool usbMounted = mountUSB();
	bool dlToUSB = usbMounted;
	bool keepFiles = true;
	char folderName[FILENAME_MAX - 11];
	folderName[0] = '\0';
	
	drawPDMenuFrame(entry, installed, folderName, usbMounted, dlToUSB, keepFiles);
	
	bool loop = true;
	bool inst, toUSB;
	bool uninstall = false;
	bool redraw = false;
	while(loop && AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawPDMenuFrame(entry, installed, folderName, usbMounted, dlToUSB, keepFiles);
		
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
			vibrateWhenFinished = !vibrateWhenFinished;
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
			drawPDMenuFrame(entry, installed, folderName, usbMounted, dlToUSB, keepFiles);
			redraw = false;
		}
	}
	
	if(!AppRunning())
		return;
	
	if(uninstall)
	{
		deinstall(titleList, false);
		return;
	}
	
	char *tid = hex(entry->tid, 16);
	if(tid != NULL)
	{
		downloadTitle(tid, "\0", folderName, inst, dlToUSB, toUSB, keepFiles);
		MEMFreeToDefaultHeap(tid);
	}
	// TODO: Else
}
