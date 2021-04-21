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

#include <file.h>
#include <input.h>
#include <renderer.h>
#include <status.h>
#include <usb.h>
#include <menu/filebrowser.h>

#include <coreinit/memdefaultheap.h>

#include <dirent.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FILEBROWSER_LINES (MAX_LINES - 5)

void drawFBMenuFrame(char **folders, size_t foldersSize, const size_t pos, const size_t cursor, const bool onUSB)
{
	startNewFrame();
	textToFrame(0, 6, "Select a folder:");
	
	boxToFrame(1, MAX_LINES - 3);
	
	char toWrite[512];
	strcpy(toWrite, "Press \uE000 to select || \uE001 to return || \uE002 to switch to ");
	strcat(toWrite, onUSB ? "SD" : "USB");
	strcat(toWrite, " || \uE003 to refresh");
	textToFrame(MAX_LINES - 2, ALIGNED_CENTER, toWrite);
	
	strcpy(toWrite, "Searching on => ");
	strcat(toWrite, onUSB ? "USB" : "SD");
	strcat(toWrite, ":/install/");
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, toWrite);
	
	foldersSize -= pos;
	size_t max = MAX_FILEBROWSER_LINES < foldersSize ? MAX_FILEBROWSER_LINES : foldersSize;
	for(size_t i = 0; i < max; i++)
	{
		textToFrame(i + 2, 8, folders[i + pos]);
		if(cursor == i)
			arrowToFrame(i + 2, 1);
	}
	drawFrame();
}

char *fileBrowserMenu()
{
	char *folders[1024];
	folders[0] = MEMAllocFromDefaultHeap(3);
	if(folders[0] == NULL)
		return NULL;
	
	strcpy(folders[0], "./");
	
	size_t foldersSize = 1;
	size_t cursor, pos;
	bool usbMounted = mountUSB();
	bool onUSB = usbMounted;
	bool mov;
	DIR *dir;
	
refreshDirList:
	usbMounted = mountUSB();
	if(!usbMounted)
		onUSB = false;
	
	for(int i = 1; i < foldersSize; i++)
		MEMFreeToDefaultHeap(folders[i]);
	foldersSize = 1;
	cursor = pos = 0;
	
	dir = opendir(onUSB ? INSTALL_DIR_USB : INSTALL_DIR_SD);
	if(dir != NULL)
	{
		size_t len;
		for(struct dirent *entry = readdir(dir); entry != NULL && foldersSize < 1024; entry = readdir(dir))
			if(entry->d_type & DT_DIR) //Check if it's a directory
			{
				len = strlen(entry->d_name);
				folders[foldersSize] = MEMAllocFromDefaultHeap(len + 2);
				
				strcpy(folders[foldersSize], entry->d_name);
				folders[foldersSize][len++] = '/';
				folders[foldersSize++][len] = '\0';
			}
		closedir(dir);
	}
	
	drawFBMenuFrame(folders, foldersSize, pos, cursor, onUSB);
	
	mov = foldersSize >= MAX_FILEBROWSER_LINES;
	char *ret = NULL;
	bool redraw = false;
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawFBMenuFrame(folders, foldersSize, pos, cursor, onUSB);
		
		showFrame();
		if(vpad.trigger & VPAD_BUTTON_B)
			goto exitFileBrowserMenu;
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			if(dir != NULL)
			{
				size_t len = strlen(onUSB ? INSTALL_DIR_USB : INSTALL_DIR_SD) + strlen(folders[cursor + pos]) + 1;
				ret = MEMAllocFromDefaultHeap(len); //TODO: Free
				if(ret != NULL)
				{
					strcpy(ret, onUSB ? INSTALL_DIR_USB : INSTALL_DIR_SD);
					strcat(ret, folders[cursor + pos]);
				}
				goto exitFileBrowserMenu;
			}
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
						cursor = MAX_FILEBROWSER_LINES - 1;
						pos = foldersSize - MAX_FILEBROWSER_LINES;
					}
				}
				else
					cursor = foldersSize - 1;
			}
			
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			if(cursor >= foldersSize - 1 || cursor >= MAX_FILEBROWSER_LINES - 1)
			{
				if(mov && pos < foldersSize - MAX_FILEBROWSER_LINES)
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
				pos += MAX_FILEBROWSER_LINES;
				if(pos > foldersSize - MAX_FILEBROWSER_LINES)
					pos = 0;
				redraw = true;
			}
			else if(vpad.trigger & VPAD_BUTTON_LEFT)
			{
				pos -= MAX_FILEBROWSER_LINES;
				if(pos > foldersSize - MAX_FILEBROWSER_LINES) //TODO
					pos = foldersSize - MAX_FILEBROWSER_LINES;
				redraw = true;
			}
		}
		
		if(vpad.trigger & VPAD_BUTTON_X)
		{
			onUSB = !onUSB;
			goto refreshDirList;
		}
		if(vpad.trigger & VPAD_BUTTON_Y)
			goto refreshDirList;
		
		if(redraw)
		{
			drawFBMenuFrame(folders, foldersSize, pos, cursor, onUSB);
			redraw = false;
		}
	}
	
exitFileBrowserMenu:
	for(int i = 0; i < foldersSize; i++)
		MEMFreeToDefaultHeap(folders[i]);
	return ret;
}
