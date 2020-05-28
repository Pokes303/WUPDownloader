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

GameInfo *gameInfo = NULL;
size_t gameInfoSize;
bool vibrateWhenFinished = true;

void drawDownloadFrame1()
{
	startNewFrame();
	textToFrame(0, 0, "Input a title ID to download its content [Ex: 000500001234abcd]");
	textToFrame(0, 2, "If a fake ticket.tik is needed, you will need to write its encrypted tile key (Check it on any 'WiiU title key");
	textToFrame(1, 3, "site')");
	textToFrame(0, 5, "[USE A VALID ENCRYPTED TITLE KEY FOR EACH TITLE, OTHERWISE, IT WILL THROW A TITLE.TIK");
	textToFrame(1, 6, "ERROR WHILE INSTALLING IT]");
	lineToFrame(MAX_LINES - 3, SCREEN_COLOR_BROWN);
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
	const char *tfn = folderName[0] == '\0' ? titleID : folderName;
	char toFrame[13 + strlen(tfn) > 59 ? strlen(tfn) : 59];
	strcpy(toFrame, "sd:/install/");
	strcat(toFrame, tfn);
	textToFrame(3, 5, toFrame);
	
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
	
	lineToFrame(line--, SCREEN_COLOR_BROWN);
	
	textToFrame(0, line--, "Press \uE041 DOWN to set a custom name to the download folder");
	textToFrame(0, line--, "Press \uE041 RIGHT to set the title version");
	textToFrame(0, line--, "Press \uE041 UP to set the title ID");
	lineToFrame(line--, SCREEN_COLOR_BROWN);
	
	textToFrame(0, line--, "Press \uE001 to return");
	
	strcpy(toFrame, "Press \uE003 to download to ");
	strcat(toFrame, dlToUSB ? "USB" : "SD");
	strcat(toFrame, " only");
	textToFrame(0, line--, toFrame);
	
	textToFrame(0, line--, "Press \uE002 to install to NAND");
	textToFrame(0, line--, "Press \uE000 to install to USB");
	lineToFrame(line--, SCREEN_COLOR_BROWN);
	
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
		showFrame();
		
		if(app == 2)
			continue;
		
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
	
	GameInfo gameToInstall;
	
	gameToInstall.tid = titleID;
	gameToInstall.key = gameToInstall.name = NULL;
	
	if(gameInfo != NULL)
	{
		for(int i = 0; i < gameInfoSize; i++)
			if(gameInfo[i].tid != NULL && strcmp(gameInfo[i].tid, titleID) == 0)
			{
				gameToInstall.key = gameInfo[i].key;
				gameToInstall.name = gameInfo[i].name;
				break;
			}
	}
	
	bool usbMounted = mountUSB();
	bool dlToUSB = false;
	bool keepFiles = true;
	drawDownloadFrame2(titleID, titleVer, folderName, usbMounted, dlToUSB, keepFiles);
	
	bool loop = true;
	bool inst, toUSB;
	while(loop && AppRunning())
	{
		showFrame();
		
		if(app == 2)
			continue;
		
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
	
	downloadTitle(gameToInstall, titleVer, folderName, inst, dlToUSB, toUSB, keepFiles);
}

void drawDownloadJSONFrame()
{
	colorStartNewFrame(SCREEN_COLOR_RED);
	textToFrame(0, 0, "Could not download from");
	textToFrame(0, 1, getTitleKeySite());
	lineToFrame(MAX_LINES - 4, SCREEN_COLOR_BROWN);
	textToFrame(0, MAX_LINES - 3, "Press \uE000 to enter a new URL");
	textToFrame(0, MAX_LINES - 2, "Press \uE003 to continue");
	textToFrame(0, MAX_LINES - 1, "Press \uE001 to exit");
	drawFrame();
}

bool enterKeySite()
{
	drawDownloadJSONFrame();
	
	while(true)
	{
		showFrame();
		
		switch(vpad.trigger)
		{
			case VPAD_BUTTON_A:
				;
				char newUrl[1024];
				if(showKeyboard(KEYBOARD_TYPE_NORMAL, newUrl, CHECK_URL, 1024, false, getTitleKeySite(), "SAVE"))
					setTitleKeySite(newUrl);
				
				return downloadJSON();
			case VPAD_BUTTON_B:
				return false;
			case VPAD_BUTTON_Y:
				return true;
		}
	}
}

bool downloadJSON()
{
	char jsonUrl[2048];
	strcpy(jsonUrl, getTitleKeySite());
	strcat(jsonUrl, "/json");
	contents = 0xFFFF;
	if(downloadFile(jsonUrl, "keys.json", FILE_TYPE_JSON | FILE_TYPE_TORAM) != 0)
	{
		flushIOQueue();
		if(ramBuf != NULL)
		{
			MEMFreeToDefaultHeap(ramBuf);
			ramBuf = NULL;
			ramBufSize = 0;
		}
		
		return enterKeySite();
	}
	
	startNewFrame();
	textToFrame(0, 0, "Parsing keys.json");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	flushIOQueue();
	cJSON *json = cJSON_ParseWithLength(ramBuf, ramBufSize);
	if(json == NULL)
	{
		debugPrintf("json == NULL");
		return enterKeySite();
	}
	
	gameInfoSize = cJSON_GetArraySize(json);
	gameInfo = MEMAllocFromDefaultHeap(sizeof(GameInfo) * gameInfoSize);
	if(gameInfo == NULL)
	{
		cJSON_Delete(json);
		return false;
	}
	
	char *tid;
	char *name;
	char *key;
	cJSON *start = json;
	cJSON *tmpObj;
	for(int i = 0; i < gameInfoSize; i++)
	{
		json = cJSON_GetArrayItem(start, i);
		tid = name = key = NULL;
		tmpObj = cJSON_GetObjectItemCaseSensitive(json, "titleID");
		if(tmpObj != NULL && cJSON_IsString(tmpObj) && tmpObj->valuestring != NULL)
		{
			tid = tmpObj->valuestring;
			
			tmpObj = cJSON_GetObjectItemCaseSensitive(json, "name");
			if(tmpObj != NULL && cJSON_IsString(tmpObj) && tmpObj->valuestring != NULL)
			{
				name = tmpObj->valuestring;
				for(int j = 0; j < strlen(name); j++)
					if(name[j] == '\n')
						name[j] = ' ';
			}
			
			tmpObj = cJSON_GetObjectItemCaseSensitive(json, "titleKey");
			if(tmpObj != NULL && cJSON_IsString(tmpObj) && tmpObj->valuestring != NULL)
				key = tmpObj->valuestring;
			else
			{
				tmpObj = cJSON_GetObjectItemCaseSensitive(json, "ticket");
				if(tmpObj != NULL && cJSON_IsNumber(tmpObj) && tmpObj->valueint == 1)
					key = "x";
			}
			
			if(name != NULL || key != NULL)
			{
				gameInfo[i].tid = MEMAllocFromDefaultHeap(17);
				gameInfo[i].name = name == NULL ? NULL : MEMAllocFromDefaultHeap(sizeof(char) * strlen(name));
				gameInfo[i].key = key == NULL ? NULL : MEMAllocFromDefaultHeap(33);
				
				strcpy(gameInfo[i].tid, tid);
				if(gameInfo[i].name != NULL)
					strcpy(gameInfo[i].name, name);
				if(gameInfo[i].key != NULL)
					strcpy(gameInfo[i].key, key);
				continue;
			}
		}
		gameInfo[i].tid = gameInfo[i].name = gameInfo[i].key = NULL;
	}
	
	cJSON_Delete(start);
	MEMFreeToDefaultHeap(ramBuf);
	ramBuf = NULL;
	ramBufSize = 0;
	addToScreenLog("keys.json parsed!");
	return true;
}

void freeJSON()
{
	if(gameInfo != NULL)
	{
		for(int i = 0; i < gameInfoSize; i++)
		{
			if(gameInfo[i].tid != NULL)
				MEMFreeToDefaultHeap(gameInfo[i].tid);
			if(gameInfo[i].name != NULL)
				MEMFreeToDefaultHeap(gameInfo[i].name);
			if(gameInfo[i].key != NULL)
				MEMFreeToDefaultHeap(gameInfo[i].key);
		}
		MEMFreeToDefaultHeap(gameInfo);
	}
}
