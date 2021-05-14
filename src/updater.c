/***************************************************************************
 * This file is part of NUSspli.                                           *
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

#include <config.h>
#include <deinstaller.h>
#include <downloader.h>
#include <file.h>
#include <input.h>
#include <installer.h>
#include <ioThread.h>
#include <osdefs.h>
#include <renderer.h>
#include <rumbleThread.h>
#include <status.h>
#include <utils.h>
#include <cJSON.h>
#include <menu/update.h>
#include <menu/utils.h>

#include <coreinit/filesystem.h>
#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>

#include <unzip.h>

#include <errno.h>
#include <file.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define UPDATE_CHECK_URL NAPI_URL "s?v=" NUSSPLI_VERSION "&t="
#define UPDATE_DL_URL NAPI_URL "d?t="
#define UPDATE_TEMP_FOLDER "fs:/vol/external01/NUSspli_temp/"
#define UPDATE_AROMA_FOLDER "fs:/vol/external01/wiiu/apps/"
#define UPDATE_AROMA_FILE "NUSspli.wuhb"
#define UPDATE_HBL_FOLDER "fs:/vol/external01/wiiu/apps/NUSspli"

void showUpdateError(const char* msg)
{
	enableShutdown();
	drawErrorFrame(msg, B_RETURN);
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawErrorFrame(msg, B_RETURN);
		
		showFrame();
		
		if(vpad.trigger & VPAD_BUTTON_B)
			return;
	}
}

void showUpdateErrorf(const char *msg, ...)
{
	va_list va;
	va_start(va, msg);
	char newMsg[2048];
	vsnprintf(newMsg, 2048, msg, va);
	va_end(va);
	showUpdateError(newMsg);
}

bool updateCheck()
{
	if(!updateCheckEnabled())
		return false;
	
	startNewFrame();
	textToFrame(0, 0, "Prepairing download");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	char *updateChkUrl = isAroma() ? UPDATE_CHECK_URL "a" : isChannel() ? UPDATE_CHECK_URL "c" : UPDATE_CHECK_URL "h";
	
	if(downloadFile(updateChkUrl, "JSON", NULL, FILE_TYPE_JSON | FILE_TYPE_TORAM, false) != 0)
	{
		clearRamBuf();
		debugPrintf("Error downloading %s", updateChkUrl);
		return false;
	}
	
	startNewFrame();
	textToFrame(0, 0, "Parsing JSON");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	cJSON *json = cJSON_ParseWithLength(ramBuf, ramBufSize);
	if(json == NULL)
	{
		clearRamBuf();
		debugPrintf("Invalid JSON data");
		return false;
	}
	
	cJSON *jsonObj = cJSON_GetObjectItemCaseSensitive(json, "s");
	if(jsonObj == NULL || !cJSON_IsNumber(jsonObj))
	{
		cJSON_Delete(json);
		clearRamBuf();
		debugPrintf("Invalid JSON data");
		return false;
	}
	
	char *newVer;
	switch(jsonObj->valueint)
	{
		case 0:
			debugPrintf("Newest version!");
			cJSON_Delete(json);
			clearRamBuf();
			return false;
		case 1:
			newVer = cJSON_GetObjectItemCaseSensitive(json, "v")->valuestring;
			break;
		case 2: //TODO
			showUpdateErrorf("The %s version of NUSspli is deprecated!", isAroma() ? "Aroma" : isChannel() ? "Channel" : "HBL");
			cJSON_Delete(json);
			clearRamBuf();
			return false;
		case 3: // TODO
		case 4:
			showUpdateError("Internal server error!");
			cJSON_Delete(json);
			clearRamBuf();
			return false;
		default: // TODO
			showUpdateErrorf("Invalid state value: %d", jsonObj->valueint);
			cJSON_Delete(json);
			clearRamBuf();
			return false;
	}
	
	char versionString[strlen(newVer) + 1];
	strcpy(versionString, newVer);
	cJSON_Delete(json);
	clearRamBuf();
	
	return updateMenu(versionString);
}

//TODO: Quick & dirty, supports absolute paths on fs:/vol/external01 only.
bool createDirRecursive(char *dir)
{
	if(strlen(dir) < 20 || dir[18] != '/')
		return false;
	
	char *ptr = dir + 19;
	char *needle;
	do
	{
		needle = strchr(ptr, '/');
		if(needle == NULL)
		{
			if(!dirExists(dir))
				return mkdir(dir, 777) == 0;
			
			return true;
		}
		
		needle[0] = '\0';
		if(!dirExists(dir) && mkdir(dir, 777) != 0)
			return false;
		
		needle[0] = '/';
		ptr = needle + 1;
	}
	while(ptr[0] != '\0');
	
	return true;
}

void update(char *newVersion)
{
	disableShutdown();
	removeDirectory(UPDATE_TEMP_FOLDER);
	if(mkdir(UPDATE_TEMP_FOLDER, 777) == -1)
	{
		int ie = errno;
		char *toScreen = getToFrameBuffer();
		strcpy(toScreen, "Error creating temporary directory!\n\n");
		switch(ie)
		{
			case EROFS:
				strcat(toScreen, "SD card write locked!");
				break;
			case FS_ERROR_MAX_FILES:
			case FS_ERROR_MAX_DIRS:
			case ENOSPC:
				strcat(toScreen, "Filesystem limits reached!");
				break;
			default:
				sprintf(toScreen + strlen(toScreen), "%d %s", ie, strerror(ie));
		}
		showUpdateError(toScreen);
		return;
	}
	
	char *url = isAroma() ? UPDATE_DL_URL"a" : (isChannel() ? UPDATE_DL_URL"c" : UPDATE_DL_URL"h");
	
	if(downloadFile(url, "NUSspli.zip", NULL, FILE_TYPE_JSON | FILE_TYPE_TORAM, false) != 0)
	{
		clearRamBuf();
		showUpdateErrorf("Error downloading %s", url);
		return;
	}
	
	startNewFrame();
	textToFrame(0, 0, "Updating, please wait...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	unzFile zip = unzOpenBuffer(ramBuf, ramBufSize);
	unz_global_info zipInfo;
	if(unzGetGlobalInfo(zip, &zipInfo) != UNZ_OK)
	{
		clearRamBuf();
		showUpdateError("Error getting zip info");
		return;
	}
	
	uint8_t *buf = MEMAllocFromDefaultHeap(IO_BUFSIZE);
	if(buf == NULL)
	{
		clearRamBuf();
		showUpdateError("Error allocating memory");
		return;
	}
	
	char zipFileName[256];
	unz_file_info zipFileInfo;
	char *needle;
	char path[256];
	char fileName[1024];
	strcpy(fileName, UPDATE_TEMP_FOLDER);
	char *fnp = fileName + strlen(UPDATE_TEMP_FOLDER);
	NUSFILE *file;
	size_t extracted;
	char *lastSlash;
	char *lspp;
	
	do
	{
		if(unzGetCurrentFileInfo(zip, &zipFileInfo, zipFileName, 256, NULL, 0, NULL, 0) != UNZ_OK)
		{
			MEMFreeToDefaultHeap(buf);
			clearRamBuf();
			showUpdateError("Error extracting zip");
			return;
		}
		
		if(unzOpenCurrentFile(zip) != UNZ_OK)
		{
			MEMFreeToDefaultHeap(buf);
			clearRamBuf();
			showUpdateError("Error opening zip file");
			return;
		}
		
		needle = strchr(zipFileName, '/');
		if(needle != NULL)
		{
			lastSlash = needle;
			lspp = needle + 1;
			needle = strchr(lspp, '/');
			while(needle != NULL)
			{
				lastSlash = needle;
				lspp = needle + 1;
				needle = strchr(lspp, '/');
			}
			
			if(lastSlash[1] == '\0')
			{
				unzCloseCurrentFile(zip);
				continue;
			}
			
			lastSlash[0] = '\0';
			strcpy(path, zipFileName);
			strcat(path, "/");
			strcpy(fnp, path);
			strcpy(zipFileName, lastSlash + 1);
			
			if(!createDirRecursive(fileName))
			{
				unzCloseCurrentFile(zip);
				MEMFreeToDefaultHeap(buf);
				clearRamBuf();
				showUpdateErrorf("Error creating directory: %s", fileName);
				return;
			}
		}
		else
			path[0] = '\0';
		
		sprintf(fnp, "%s%s", path, zipFileName);
		file = openFile(fileName, "wb");
		if(file == NULL)
		{
			unzCloseCurrentFile(zip);
			MEMFreeToDefaultHeap(buf);
			clearRamBuf();
			showUpdateErrorf("Error opening file: %s", fileName);
			return;
		}
		
		while(true)
		{
			extracted = unzReadCurrentFile(zip, buf, IO_BUFSIZE);
			if(extracted < 0)
			{
				addToIOQueue(NULL, 0, 0, file);
				unzCloseCurrentFile(zip);
				MEMFreeToDefaultHeap(buf);
				clearRamBuf();
				showUpdateErrorf("Error extracting file: %s", fileName);
				return;
			}
			
			if(extracted != 0)
			{
				if(addToIOQueue(buf, 1, extracted, file) != extracted)
				{
					addToIOQueue(NULL, 0, 0, file);
					unzCloseCurrentFile(zip);
					MEMFreeToDefaultHeap(buf);
					clearRamBuf();
					showUpdateErrorf("Error writing file: %s", fileName);
					return;
				}
			}
			else
				break;
		}
		
		unzCloseCurrentFile(zip);
		addToIOQueue(NULL, 0, 0, file);
	}
	while(unzGoToNextFile(zip) == UNZ_OK);
	
	MEMFreeToDefaultHeap(buf);
	clearRamBuf();
	flushIOQueue();
	
	if(isChannel())
	{
		MCPTitleListType ownInfo;
		MCPError err = MCP_GetOwnTitleInfo(mcpHandle, &ownInfo);
		if(err != 0)
		{
			showUpdateErrorf("Error getting own title info: %#010x", err);
			return;
		}
		
		deinstall(ownInfo, true);
		OSSleepTicks(OSSecondsToTicks(10)); // channelHaxx...
		
		if(isAroma())
			goto aromaInstallation;
		
		char installPath[strlen(UPDATE_TEMP_FOLDER) + 8];
		strcpy(installPath, UPDATE_TEMP_FOLDER);
		strcat(installPath, "NUSspli");
		
		install("Update", false, false, installPath, ownInfo.indexedDevice[0] == 'u', true);
		removeDirectory(UPDATE_TEMP_FOLDER);
		enableShutdown();
	}
	else
	{
		if(isAroma())
		{
			if(dirExists(UPDATE_HBL_FOLDER))
				removeDirectory(UPDATE_HBL_FOLDER);
			
			char *aromaFile = UPDATE_AROMA_FOLDER UPDATE_AROMA_FILE;
			if(fileExists(aromaFile))
				remove(aromaFile);
			
aromaInstallation:
			rename(UPDATE_TEMP_FOLDER UPDATE_AROMA_FILE, aromaFile);
		}
		else
		{
			if(!dirExists(UPDATE_HBL_FOLDER))
			{
				showUpdateError("Couldn't find NUSspli folder on the SD card");
				return;
			}
			
			removeDirectory(UPDATE_HBL_FOLDER);
			char installPath[strlen(UPDATE_TEMP_FOLDER) + 8];
			strcpy(installPath, UPDATE_TEMP_FOLDER);
			strcat(installPath, "NUSspli");
			moveDirectory(installPath, UPDATE_HBL_FOLDER);
		}
		
		removeDirectory(UPDATE_TEMP_FOLDER);
		enableShutdown();
		startRumble();
		colorStartNewFrame(SCREEN_COLOR_D_GREEN);
		textToFrame(0, 0, "Update");
		textToFrame(1, 0, "Installed successfully!");
		writeScreenLog();
		drawFrame();
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
			{
				colorStartNewFrame(SCREEN_COLOR_D_GREEN);
				textToFrame(0, 0, "Update");
				textToFrame(1, 0, "Installed successfully!");
				writeScreenLog();
				drawFrame();
			}
			
			showFrame();
			
			if(vpad.trigger)
				return;
		}
	}
}
