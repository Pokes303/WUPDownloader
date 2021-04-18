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

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>

#include <unzip.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define UPDATE_CHECK_URL NAPI_URL "versioncheck.php"
#define UPDATE_DL_URL NAPI_URL "dl.php?t="
#define UPDATE_TEMP_FOLDER "/vol/external01/NUSspli_temp/"
#define UPDATE_AROMA_FOLDER "/vol/external01/wiiu/apps/"
#define UPDATE_AROMA_FILE "NUSspli.wuhb"
#define UPDATE_HBL_FOLDER "/vol/external01/wiiu/apps/NUSspli"
#define UPDATE_ZIP_BUF (2048 << 10) // 2 MB

bool updateCheck()
{
	if(!updateCheckEnabled())
		return false;
	
	startNewFrame();
	textToFrame(0, 0, "Prepairing download");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	if(downloadFile(UPDATE_CHECK_URL, "JSON", FILE_TYPE_JSON | FILE_TYPE_TORAM, false) != 0)
	{
		clearRamBuf();
		debugPrintf("Error downloading %s", UPDATE_CHECK_URL);
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
	
	cJSON *jsonObj = cJSON_GetObjectItemCaseSensitive(json, "state");
	if(jsonObj == NULL || !cJSON_IsNumber(jsonObj))
	{
		cJSON_Delete(json);
		clearRamBuf();
		debugPrintf("Invalid JSON data");
		return false;
	}
	
	if(jsonObj->valueint != 0)
	{
		jsonObj = cJSON_GetObjectItemCaseSensitive(json, "error");
		if(jsonObj != NULL && cJSON_IsString(jsonObj) && jsonObj->valuestring != NULL)
			debugPrintf("Server error: %s", jsonObj->valuestring);
		else
			debugPrintf("Server error");
		
		cJSON_Delete(json);
		clearRamBuf();
		debugPrintf("Invalid JSON data");
		return false;
	}
	
	jsonObj = cJSON_GetObjectItemCaseSensitive(json, "latest");
	if(jsonObj == NULL || !cJSON_IsString(jsonObj) || jsonObj->valuestring == NULL)
	{
		cJSON_Delete(json);
		clearRamBuf();
		debugPrintf("Invalid JSON data");
		return false;
	}
	
	char versionString[strlen(jsonObj->valuestring) + 1];
	strcpy(versionString, jsonObj->valuestring);
	cJSON_Delete(json);
	clearRamBuf();
	
	char cv[8];
	char *curVer = cv;
	strcpy(curVer, NUSSPLI_VERSION);
	char *needle = strchr(curVer, '.');
	needle[0] = '\0';
	int curMajor = atoi(curVer);
	curVer = needle + 1;
	needle = strchr(curVer, '-');
	if(needle != NULL) // BETA version
		needle[0] = '\0';
	
	int curMinor = atoi(curVer);
	if(needle != NULL)
		curMinor--;
	
	needle = strchr(versionString, '.');
	if(needle == NULL)
	{
		debugPrintf("Invalid version string");
		return false;
	}
	
	needle[0] = '\0';
	int major = atoi(versionString);
	if(major == 0)
	{
		debugPrintf("Invalid major version");
		return false;
	}
	
	bool updateAvailable;
	if(major > curMajor)
		updateAvailable = true;
	else if(major == curMajor)
		updateAvailable = atoi(needle + 1) > curMinor;
	else // Should never happen
		updateAvailable = false;
	
	needle[0] = '.';
	return updateAvailable && updateMenu(versionString);
}

//TODO: Quick & dirty, supports absolute paths on /vol/external01 only.
bool createDirRecursive(char *dir)
{
	if(strlen(dir) < 17 || dir[15] != '/')
		return false;
	
	char *ptr = dir + 16;
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

void update(char *newVersion)
{
	disableShutdown();
	removeDirectory(UPDATE_TEMP_FOLDER);
	if(mkdir(UPDATE_TEMP_FOLDER, 777) != 0)
	{
		showUpdateError("Error creating temporary directory!");
		return;
	}
	
	char *url = isAroma() ? UPDATE_DL_URL"a" : (isChannel() ? UPDATE_DL_URL"c" : UPDATE_DL_URL"h");
	
	if(downloadFile(url, "NUSspli.zip", FILE_TYPE_JSON | FILE_TYPE_TORAM, false) != 0)
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
	
	uint8_t *buf = MEMAllocFromDefaultHeap(UPDATE_ZIP_BUF);
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
	FILE *file;
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
				if(unzGoToNextFile(zip) != UNZ_OK)
					break;
				
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
				showUpdateErrorf("Error creating directory: %s", path);
				return;
			}
		}
		else
			path[0] = '\0';
		
		sprintf(fnp, "%s%s", path, zipFileName);
		file = fopen(fileName, "wb");
		if(file == NULL)
		{
			unzCloseCurrentFile(zip);
			MEMFreeToDefaultHeap(buf);
			clearRamBuf();
			showUpdateErrorf("Error opening file: %s", fileName);
			return;
		}
		
		do
		{
			extracted = unzReadCurrentFile(zip, buf, UPDATE_ZIP_BUF);
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
				if(addToIOQueue(buf, extracted, 1, file) != extracted)
				{
					addToIOQueue(NULL, 0, 0, file);
					unzCloseCurrentFile(zip);
					MEMFreeToDefaultHeap(buf);
					clearRamBuf();
					showUpdateErrorf("Error writing file: %s", fileName);
					return;
				}
			}
		}
		while(extracted != 0);
		
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
		
		MCPInstallTitleInfo info;
		//err = MCP_UninstallTitleAsync(mcpHandle, ownInfo.path, &info);
		// The above crashes MCP, so let's leave WUT:
		err = MCP_DeleteTitleAsync(mcpHandle, ownInfo.path, &info);
		if(err != 0)
		{
			showUpdateErrorf("Error uninstalling: %#010x", err);
			return;
		}
		
		if(isAroma())
			goto aromaInstallation;
		
		OSSleepTicks(OSSecondsToTicks(10)); // TODO: Check if MCP finished in a loop
		
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
