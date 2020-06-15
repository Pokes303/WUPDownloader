/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020 V10lator <v10lator@myway.de>                         *
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

#include <config.h>
#include <downloader.h>
#include <input.h>
#include <ioThread.h>
#include <renderer.h>
#include <status.h>
#include <usb.h>
#include <utils.h>
#include <cJSON.h>
#include <menu/download.h>
#include <menu/utils.h>

#include <stdio.h>
#include <string.h>

#include <coreinit/memdefaultheap.h>
#include <vpad/input.h>

bool vibrateWhenFinished = true;

void drawDownloadFrame1()
{
	startNewFrame();
	textToFrame(0, 0, "Input a title ID to download its content [Ex: 000500001234abcd]");
	lineToFrame(MAX_LINES - 3, SCREEN_COLOR_WHITE);
	textToFrame(0, MAX_LINES - 2, "Press \uE000 to show the keyboard [Only input hexadecimal numbers]");
	textToFrame(0, MAX_LINES - 1, "Press \uE001 to return");
	drawFrame();
}

void drawDownloadFrame2(const char *titleID, const char *titleVer, const char *folderName, bool usbMounted, bool dlToUSB, bool keepFiles)
{
	startNewFrame();
	textToFrame(0, 0, "Provided title ID [Only 16 digit hexadecimal]:");
	textToFrame(3, 1, titleID);
	
	textToFrame(0, 2, "Provided title version [Only numbers]:");
	textToFrame(3, 3, titleVer[0] == '\0' ? "<LATEST>" : titleVer);
	
	textToFrame(0, 4, "Custom folder name [Only text and numbers]:");
	textToFrame(3, 5, folderName);
	
	char toFrame[128];
	strcpy(toFrame, "Press \uE045 to ");
	strcat(toFrame, vibrateWhenFinished ? "deactivate" : "activate");
	strcat(toFrame, " the vibration after installing");
	textToFrame(0, MAX_LINES - 1, toFrame); //Thinking to change this to activate HOME Button led
	
	int line = MAX_LINES - 2;
	if(usbMounted)
	{
		strcpy(toFrame, "Press \uE046 to download to ");
		strcat(toFrame, dlToUSB ? "SD" : "USB");
		textToFrame(0, line--, toFrame);
	}
	if(dlToUSB)
		textToFrame(0, line--, "WARNING: Files on USB will always be deleted after installing!");
	else
	{
		strcpy(toFrame, "Press \uE041 LEFT to ");
		strcat(toFrame, keepFiles ? "delete" : "keep");
		strcat(toFrame, " downloaded files after the installation");
		textToFrame(0, line--, toFrame);
	}
	
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	textToFrame(0, line--, "Press \uE041 DOWN to set a custom name to the download folder");
	textToFrame(0, line--, "Press \uE041 RIGHT to set the title version");
	textToFrame(0, line--, "Press \uE041 UP to set the title ID");
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	textToFrame(0, line--, "Press \uE001 to return");
	
	strcpy(toFrame, "Press \uE003 to download to ");
	strcat(toFrame, dlToUSB ? "USB" : "SD");
	strcat(toFrame, " only");
	textToFrame(0, line--, toFrame);
	
	textToFrame(0, line--, "Press \uE002 to install to NAND");
	textToFrame(0, line--, "Press \uE000 to install to USB");
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	drawFrame();
}

void downloadMenu()
{
	char titleID[17];
	char titleVer[33];
	char folderName[FILENAME_MAX - 11];
	titleID[0] = titleVer[0] = folderName[0] = '\0';
	
	drawDownloadFrame1();
	
	while(AppRunning())
	{
		if(app == 2)
			continue;
		if(app == 9)
			drawDownloadFrame1();
		
		showFrame();
		
		if (vpad.trigger == VPAD_BUTTON_A) {
			if(showKeyboard(KEYBOARD_TYPE_RESTRICTED, titleID, CHECK_HEXADECIMAL, 16, true, "00050000101", NULL))
				 break;
			 drawDownloadFrame1();
		}
		else if (vpad.trigger == VPAD_BUTTON_B)
			return;
	}
	if(!AppRunning())
		return;
	
	toLowercase(titleID);
	
	bool usbMounted = mountUSB();
	bool dlToUSB = false;
	bool keepFiles = true;
	drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
	
	bool loop = true;
	bool inst, toUSB;
	while(loop && AppRunning())
	{
		if(app == 2)
			continue;
		if(app == 9)
			drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
		
		showFrame();
		
		switch(vpad.trigger)
		{
			case VPAD_BUTTON_A:
				inst = toUSB = true;
				loop = false;
				break;
			case VPAD_BUTTON_Y:
				inst = toUSB = loop = false;
				break;
			case VPAD_BUTTON_X:
				inst = true;
				toUSB = loop = false;
				break;
			case VPAD_BUTTON_B:
				return;
			case VPAD_BUTTON_UP:
				;
				char tmpTitleID[17];
				if(showKeyboard(KEYBOARD_TYPE_RESTRICTED, tmpTitleID, CHECK_HEXADECIMAL, 16, true, titleID, NULL))
				{
					toLowercase(tmpTitleID);
					strcpy(titleID, tmpTitleID);
					drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
				}
				break;
			case VPAD_BUTTON_RIGHT:
				if(!showKeyboard(KEYBOARD_TYPE_RESTRICTED, titleVer, CHECK_NUMERICAL, 5, false, titleVer, NULL))
					titleVer[0] = '\0';
				drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
				break;
			case VPAD_BUTTON_DOWN:
				if(!showKeyboard(KEYBOARD_TYPE_RESTRICTED, folderName, CHECK_NOSPECIAL, FILENAME_MAX - 11, false, folderName, NULL))
					folderName[0] = '\0';
				drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
				break;
			case VPAD_BUTTON_MINUS:
				if(usbMounted)
				{
					dlToUSB = !dlToUSB;
					keepFiles = !dlToUSB;
					drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
				}
				break;
			case VPAD_BUTTON_PLUS:
				vibrateWhenFinished = !vibrateWhenFinished;
				drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
				break;
			case VPAD_BUTTON_LEFT:
				if(!dlToUSB)
				{
					keepFiles = !keepFiles;
					drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
				}
				break;
		}
	}
	
	if(!AppRunning())
		return;
	
	downloadTitle(titleID, titleVer, folderName, inst, dlToUSB, toUSB, keepFiles);
}
