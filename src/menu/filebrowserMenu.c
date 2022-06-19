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

#include <crypto.h>
#include <file.h>
#include <input.h>
#include <renderer.h>
#include <state.h>
#include <usb.h>
#include <menu/filebrowser.h>

#include <coreinit/memdefaultheap.h>

#include <dirent.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FILEBROWSER_LINES (MAX_LINES - 5)

static void drawFBMenuFrame(char **folders, size_t foldersSize, const size_t pos, const size_t cursor, const NUSDEV activeDevice, bool usbMounted)
{
	startNewFrame();
	textToFrame(0, 6, "Select a folder:");

	boxToFrame(1, MAX_LINES - 3);

	char *toWrite = getToFrameBuffer();
	strcpy(toWrite, "Press " BUTTON_A " to select || " BUTTON_B " to return || " BUTTON_X " to switch to ");
	strcat(toWrite, activeDevice == NUSDEV_USB ? "SD" : activeDevice == NUSDEV_SD ? "NAND" : usbMounted ? "USB" : "SD");
	strcat(toWrite, " || " BUTTON_Y " to refresh");
	textToFrame(MAX_LINES - 2, ALIGNED_CENTER, toWrite);

	strcpy(toWrite, "Searching on => ");
	strcpy(toWrite + 16, activeDevice == NUSDEV_USB ? "USB" : activeDevice == NUSDEV_SD ? "SD" : "NAND");
	strcat(toWrite, ":/install/");
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, toWrite);

	foldersSize -= pos;
	if(foldersSize > MAX_FILEBROWSER_LINES)
		foldersSize = MAX_FILEBROWSER_LINES;

	for(size_t i = 0; i < foldersSize; ++i)
	{
		textToFrame(i + 2, 5, folders[i + pos]);
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
	NUSDEV usbMounted = getUSB();
	NUSDEV activeDevice = usbMounted ? NUSDEV_USB : NUSDEV_SD;
	bool mov;
	DIR *dir;
	char *ret = NULL;
	
refreshDirList:
    OSTime t = OSGetTime();
	for(int i = 1; i < foldersSize; ++i)
		MEMFreeToDefaultHeap(folders[i]);
	foldersSize = 0;
	cursor = pos = 0;

	if(ret != NULL)
	{
		MEMFreeToDefaultHeap(ret);
		ret = NULL;
	}

	dir = opendir((activeDevice & NUSDEV_USB) ? (usbMounted == NUSDEV_USB01 ? INSTALL_DIR_USB1 : INSTALL_DIR_USB2) : (activeDevice == NUSDEV_SD ? INSTALL_DIR_SD : INSTALL_DIR_MLC));
	if(dir != NULL)
	{
		size_t len;
		for(struct dirent *entry = readdir(dir); entry != NULL && foldersSize < 1024; entry = readdir(dir))
			if(entry->d_type & DT_DIR) //Check if it's a directory
			{
				len = strlen(entry->d_name);
				folders[++foldersSize] = MEMAllocFromDefaultHeap(len + 2);
				
				strcpy(folders[foldersSize], entry->d_name);
				folders[foldersSize][len] = '/';
				folders[foldersSize][++len] = '\0';
			}
		closedir(dir);
	}
	t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));
	
	drawFBMenuFrame(folders, ++foldersSize, pos, cursor, activeDevice, usbMounted);
	
	mov = foldersSize >= MAX_FILEBROWSER_LINES;
	bool redraw = false;
	uint32_t oldHold = 0;
	size_t frameCount = 0;
	bool dpadAction;
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawFBMenuFrame(folders, foldersSize, pos, cursor, activeDevice, usbMounted);
		
		showFrame();
		if(vpad.trigger & VPAD_BUTTON_B)
			goto exitFileBrowserMenu;
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			if(dir != NULL)
			{
				size_t len = strlen((activeDevice & NUSDEV_USB) ? (usbMounted == NUSDEV_USB01 ? INSTALL_DIR_USB1 : INSTALL_DIR_USB2) : (activeDevice == NUSDEV_SD ? INSTALL_DIR_SD : INSTALL_DIR_MLC)) + strlen(folders[cursor + pos]) + 1;
				ret = MEMAllocFromDefaultHeap(len);
				if(ret != NULL)
				{
					strcpy(ret, (activeDevice & NUSDEV_USB) ? (usbMounted == NUSDEV_USB01 ? INSTALL_DIR_USB1 : INSTALL_DIR_USB2) : (activeDevice == NUSDEV_SD ? INSTALL_DIR_SD : INSTALL_DIR_MLC));
					strcat(ret, folders[cursor + pos]);
				}
				goto exitFileBrowserMenu;
			}
		}
		
		if(vpad.hold & VPAD_BUTTON_UP)
		{
			if(oldHold != VPAD_BUTTON_UP)
			{
				oldHold = VPAD_BUTTON_UP;
				frameCount = 30;
				dpadAction = true;
			}
			else if(frameCount == 0)
				dpadAction = true;
			else
			{
				--frameCount;
				dpadAction = false;
			}

			if(dpadAction)
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
		}
		else if(vpad.hold & VPAD_BUTTON_DOWN)
		{
			if(oldHold != VPAD_BUTTON_DOWN)
			{
				oldHold = VPAD_BUTTON_DOWN;
				frameCount = 30;
				dpadAction = true;
			}
			else if(frameCount == 0)
				dpadAction = true;
			else
			{
				--frameCount;
				dpadAction = false;
			}

			if(dpadAction)
			{
				if(cursor + pos >= foldersSize -1 || cursor >= MAX_FILEBROWSER_LINES - 1)
				{
					if(!mov || ++pos + cursor >= foldersSize)
						cursor = pos = 0;
				}
				else
					++cursor;

				redraw = true;
			}
		}
		else if(mov)
		{
			if(vpad.hold & VPAD_BUTTON_RIGHT)
			{
				if(oldHold != VPAD_BUTTON_RIGHT)
				{
					oldHold = VPAD_BUTTON_RIGHT;
					frameCount = 30;
					dpadAction = true;
				}
				else if(frameCount == 0)
					dpadAction = true;
				else
				{
					--frameCount;
					dpadAction = false;
				}

				if(dpadAction)
				{
					pos += MAX_FILEBROWSER_LINES;
					if(pos >= foldersSize)
						pos = 0;
					cursor = 0;
					redraw = true;
				}
			}
			else if(vpad.hold & VPAD_BUTTON_LEFT)
			{
				if(oldHold != VPAD_BUTTON_LEFT)
				{
					oldHold = VPAD_BUTTON_LEFT;
					frameCount = 30;
					dpadAction = true;
				}
				else if(frameCount == 0)
					dpadAction = true;
				else
				{
					--frameCount;
					dpadAction = false;
				}

				if(dpadAction)
				{
					if(pos >= MAX_FILEBROWSER_LINES)
						pos -= MAX_FILEBROWSER_LINES;
					else
						pos = foldersSize - MAX_FILEBROWSER_LINES;
					cursor = 0;
					redraw = true;
				}
			}
		}
		
		if(vpad.trigger & VPAD_BUTTON_X)
		{
			switch((int)activeDevice)
			{
				case NUSDEV_USB:
					activeDevice = NUSDEV_SD;
					break;
				case NUSDEV_SD:
					activeDevice = NUSDEV_MLC;
					break;
				case NUSDEV_MLC:
					activeDevice = usbMounted ? NUSDEV_USB : NUSDEV_SD;
			}
			goto refreshDirList;
		}
		if(vpad.trigger & VPAD_BUTTON_Y)
			goto refreshDirList;
		
		if(oldHold && !(vpad.hold & (VPAD_BUTTON_UP | VPAD_BUTTON_DOWN | VPAD_BUTTON_LEFT | VPAD_BUTTON_RIGHT)))
			oldHold = 0;

		if(redraw)
		{
			drawFBMenuFrame(folders, foldersSize, pos, cursor, activeDevice, usbMounted);
			redraw = false;
		}
	}
	
exitFileBrowserMenu:
	for(int i = 0; i < foldersSize; ++i)
		MEMFreeToDefaultHeap(folders[i]);
	return ret;
}
