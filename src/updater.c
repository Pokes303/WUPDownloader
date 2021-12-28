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
#include <filesystem.h>
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

#include <coreinit/dynload.h>
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

#define UPDATE_CHECK_URL NAPI_URL "s?t="
#define UPDATE_DL_URL NAPI_URL "d?t="
#define UPDATE_TEMP_FOLDER "fs:/vol/external01/NUSspli_temp/"
#define UPDATE_AROMA_FOLDER "fs:/vol/external01/wiiu/apps/"
#define UPDATE_AROMA_FILE "NUSspli.wuhb"
#define UPDATE_HBL_FOLDER "fs:/vol/external01/wiiu/apps/NUSspli"

#ifdef NUSSPLI_DEBUG
	#define NUSSPLI_DLVER "-DEBUG"
#else
	#define NUSSPLI_DLVER ""
#endif

void showUpdateError(const char* msg)
{
	enableShutdown();
	drawErrorFrame(msg, ANY_RETURN);
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawErrorFrame(msg, ANY_RETURN);
		
		showFrame();
		
		if(vpad.trigger)
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
	textToFrame(0, 0, "Preparing download");
	writeScreenLog();
	drawFrame();
	showFrame();
	
#ifdef NUSSPLI_HBL
	char *updateChkUrl = UPDATE_CHECK_URL "h";
#else
	char *updateChkUrl = isAroma() ? UPDATE_CHECK_URL "a" : UPDATE_CHECK_URL "c";
#endif
	
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
	
	cJSON *json = cJSON_ParseWithLength(getRamBuf(), getRamBufSize());
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
	OSDynLoad_Module mod;
	int(*RL_UnmountCurrentRunningBundle)();
	if(isAroma())
	{
		OSDynLoad_Error err = OSDynLoad_Acquire("homebrew_rpx_loader", &mod);
		if(err == OS_DYNLOAD_OK)
		{
			err = OSDynLoad_FindExport(mod, false, "RL_UnmountCurrentRunningBundle", (void **)&RL_UnmountCurrentRunningBundle);
			if(err != OS_DYNLOAD_OK)
				OSDynLoad_Release(mod);
		}
		if(err != OS_DYNLOAD_OK)
		{
			showUpdateError("Aroma version too old to allow auto-updates");
			return;
		}
	}

	disableShutdown();
	removeDirectory(UPDATE_TEMP_FOLDER);
	NUSFS_ERR err = createDirectory(UPDATE_TEMP_FOLDER, 777);
	if(err != NUSFS_ERR_NOERR)
	{
		char *toScreen = getToFrameBuffer();
		strcpy(toScreen, "Error creating temporary directory!\n\n");
		const char *errStr = translateNusfsErr(err);
		if(errStr == NULL)
			sprintf(toScreen + strlen(toScreen), "%d", err);
		else
			strcat(toScreen, errStr);

		showUpdateError(toScreen);
		goto preUpdateError;
	}

	char url[55 + (15 * 2) + 9 + 11 + 1];
	strcpy(url, "https://github.com/V10lator/NUSspli/releases/download/v");
	strcat(url, newVersion);
	strcat(url, "/NUSspli-");
	strcat(url, newVersion);
#ifdef NUSSPLI_HBL
	strcat(url, "-HBL" NUSSPLI_DLVER ".zip");
#else
	strcat(url, isAroma() ? "-Aroma" NUSSPLI_DLVER ".zip" : "-Channel" NUSSPLI_DLVER ".zip");
#endif
	
	if(downloadFile(url, "NUSspli.zip", NULL, FILE_TYPE_JSON | FILE_TYPE_TORAM, false) != 0)
	{
		clearRamBuf();
		showUpdateErrorf("Error downloading %s", url);
		goto preUpdateError;
	}
	
	startNewFrame();
	textToFrame(0, 0, "Updating, please wait...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	unzFile zip = unzOpenBuffer(getRamBuf(), getRamBufSize());
	unz_global_info zipInfo;
	if(unzGetGlobalInfo(zip, &zipInfo) != UNZ_OK)
	{
		showUpdateError("Error getting zip info");
		goto zipError1;
	}
	
	uint8_t *buf = MEMAllocFromDefaultHeap(IO_BUFSIZE);
	if(buf == NULL)
	{
		showUpdateError("Error allocating memory");
		goto zipError1;
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
			showUpdateError("Error extracting zip");
			goto zipError2;
		}
		
		if(unzOpenCurrentFile(zip) != UNZ_OK)
		{
			showUpdateError("Error opening zip file");
			goto zipError2;
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
				showUpdateErrorf("Error creating directory: %s", fileName);
				goto zipError3;
			}
		}
		else
			path[0] = '\0';
		
		sprintf(fnp, "%s%s", path, zipFileName);
		file = openFile(fileName, "wb");
		if(file == NULL)
		{
			showUpdateErrorf("Error opening file: %s", fileName);
			goto zipError3;
		}
		
		while(true)
		{
			extracted = unzReadCurrentFile(zip, buf, IO_BUFSIZE);
			if(extracted < 0)
			{
				showUpdateErrorf("Error extracting file: %s", fileName);
				goto zipError4;
			}
			
			if(extracted != 0)
			{
				if(addToIOQueue(buf, 1, extracted, file) != extracted)
				{
					showUpdateErrorf("Error writing file: %s", fileName);
					goto zipError4;
				}
			}
			else
				break;
		}
		
		unzCloseCurrentFile(zip);
		addToIOQueue(NULL, 0, 0, file);
	}
	while(unzGoToNextFile(zip) == UNZ_OK);
	
	unzClose(zip);
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
			goto updateError;
		}
		
		if(isAroma())
		{
			deinstall(ownInfo, true);
			OSSleepTicks(OSSecondsToTicks(10)); // channelHaxx...
			goto aromaInstallation;
		}
		
		char installPath[strlen(UPDATE_TEMP_FOLDER) + 8];
		strcpy(installPath, UPDATE_TEMP_FOLDER);
		strcat(installPath, "NUSspli");
		
		install("Update", false, NUSDEV_SD, installPath, ownInfo.indexedDevice[0] == 'u', true);
		removeDirectory(UPDATE_TEMP_FOLDER);
		enableShutdown();
	}
	else
	{
		if(isAroma())
		{
			if(dirExists(UPDATE_HBL_FOLDER))
				removeDirectory(UPDATE_HBL_FOLDER);
			
			if(fileExists(UPDATE_AROMA_FOLDER UPDATE_AROMA_FILE))
				remove(UPDATE_AROMA_FOLDER UPDATE_AROMA_FILE);
			
aromaInstallation:
			RL_UnmountCurrentRunningBundle();
			OSDynLoad_Release(mod);
			rename(UPDATE_TEMP_FOLDER UPDATE_AROMA_FILE, UPDATE_AROMA_FOLDER UPDATE_AROMA_FILE);
			goto finishUpdate;
		}
		// else
		if(!dirExists(UPDATE_HBL_FOLDER))
		{
			showUpdateError("Couldn't find NUSspli folder on the SD card");
			goto updateError;
		}
		
		removeDirectory(UPDATE_HBL_FOLDER);
		moveDirectory(UPDATE_TEMP_FOLDER "NUSspli", UPDATE_HBL_FOLDER);
		// endif
finishUpdate:
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
	return;

zipError4:
	addToIOQueue(NULL, 0, 0, file);
zipError3:
	unzCloseCurrentFile(zip);
zipError2:
    MEMFreeToDefaultHeap(buf);
zipError1:
	unzClose(zip);
	clearRamBuf();
updateError:
	removeDirectory(UPDATE_TEMP_FOLDER);
preUpdateError:
	if(isAroma())
		OSDynLoad_Release(mod);
}
