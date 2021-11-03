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

#include <errno.h>
#include <file.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include <config.h>
#include <downloader.h>
#include <file.h>
#include <input.h>
#include <installer.h>
#include <ioThread.h>
#include <memdebug.h>
#include <menu/utils.h>
#include <osdefs.h>
#include <renderer.h>
#include <rumbleThread.h>
#include <status.h>
#include <ticket.h>
#include <titles.h>
#include <tmd.h>
#include <utils.h>

#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <curl/curl.h>
#include <nsysnet/socket.h>

#define USERAGENT		"NUSspli/" NUSSPLI_VERSION // TODO: Spoof eShop here?
#define DLBGT_STACK_SIZE	0x2000
#define SOCKLIB_BUFSIZE		(IO_BUFSIZE * 4) // For send & receive + double buffering

double downloaded;
double onDisc;

char *ramBuf = NULL;
size_t ramBufSize = 0;

char *downloading;
bool downloadPaused = false;
OSTime lastDraw = 0;
OSTime lastTransfair;

static OSThread dlbgThread;
static uint8_t *dlbgThreadStack;

static size_t headerCallback(void *buf, size_t size, size_t multi, void *rawData)
{
	size *= multi;
	((char *)buf)[size - 1] = '\0'; //TODO: This should transform the newline at the end to a string terminator - but are we allowed to modify the buffer?
	
	toLowercase(buf);
	uint32_t contentLength = 0;
	if(sscanf(buf, "content-length: %u", &contentLength) != 1)
		return size;
	
	debugPrintf("rawData: %d", *(uint32_t *)rawData);
	debugPrintf("contentLength: %d", contentLength);
	
	if(*(uint32_t *)rawData == contentLength)
	{
		debugPrintf("equal!");
		*(uint32_t *)rawData = 0;
		return 0;
	}
		
	return size;
}

typedef enum
{
	CDE_OK,
	CDE_TIMEOUT,
	CDE_CANCELLED,
} CURL_DATA_ERROR;

typedef struct
{
	CURL *curl;
	CURL_DATA_ERROR error;
	downloadData *data;
} curlProgressData;

static int progressCallback(void *rawData, double dltotal, double dlnow, double ultotal, double ulnow)
{
	curlProgressData *data = (curlProgressData *)rawData;
	if(AppRunning())
	{
/*
		if(app == APP_STATE_BACKGROUND)
		{
			if(!downloadPaused && curl_easy_pause(data->curl, CURLPAUSE_ALL) == CURLE_OK)
			{
				downloadPaused = true;
				debugPrintf("Download paused!");
			}
			return 0;
		}
		if(downloadPaused && curl_easy_pause(data->curl, CURLPAUSE_CONT) == CURLE_OK)
		{
			downloadPaused = false;
			lastDraw = lastTransfair = 0;
			debugPrintf("Download resumed");
		}
*/
	}
	else
	{
		debugPrintf("Download cancelled!");
		data->error = CDE_CANCELLED;
		return 1;
	}
	
	OSTime now = OSGetSystemTime();
	
	if(lastDraw > 0 && OSTicksToMilliseconds(now - lastDraw) < 1000)
		return 0;
	
	bool dling = dltotal > 0.0d && !isinf(dltotal) && !isnan(dltotal);
	double dls;
	if(dling)
	{
		dlnow += onDisc;
		dltotal += onDisc;
		
		dls = dlnow - downloaded;
		if(dls < 0.01d)
		{
			if(lastTransfair > 0 && OSTicksToSeconds(now - lastTransfair) > 30)
			{
				((curlProgressData *)rawData)->error = CDE_TIMEOUT;
				return 1;
			}
		}
		else
			lastTransfair = now;
	}
	else
		lastTransfair = now;
	
	lastDraw = now;
	//debugPrintf("Downloading: %s (%u/%u) [%u%%] %u / %u bytes", downloading, data->data->dcontent, data->data->contents, (uint32_t)(dlnow / ((dltotal > 0) ? dltotal : 1) * 100), (uint32_t)dlnow, (uint32_t)dltotal);
	startNewFrame();
	char tmpString[13 + strlen(downloading)];
	
	int multiplier;
	char *multiplierName;
	if(!dling)
	{
		strcpy(tmpString, "Preparing ");
		strcat(tmpString, downloading);
		textToFrame(0, 0, tmpString);
		if(data->data != NULL)
			data->data->dlnow = data->data->dltmp;
	}
	else
	{
		strcpy(tmpString, "Downloading ");
		strcat(tmpString, downloading);
		textToFrame(0, 0, tmpString);
		
		if(dltotal < 1024.0D)
		{
			multiplier = 1;
			multiplierName = "B";
		}
		else if(dltotal < 1024.0D * 1024.0D)
		{
			multiplier = 1 << 10;
			multiplierName = "KB";
		}
		else if(dltotal < 1024.0D * 1024.0D * 1024.0D)
		{
			multiplier = 1 << 20;
			multiplierName = "MB";
		}
		else
		{
			multiplier = 1 << 30;
			multiplierName = "GB";
		}
		barToFrame(1, 0, 30, (float)(dlnow / dltotal) * 100.0f);
		sprintf(tmpString, "%.2f / %.2f %s", dlnow / multiplier, dltotal / multiplier, multiplierName);
		textToFrame(1, 31, tmpString);
		if(data->data != NULL)
			data->data->dlnow = data->data->dltmp + dlnow;
	}
	
	if(data->data != NULL)
	{
		sprintf(tmpString, "(%d/%d)", data->data->dcontent + 1, data->data->contents);
		textToFrame(0, ALIGNED_CENTER, tmpString);
		
		if(data->data->dltotal < 1024.0D)
		{
			multiplier = 1;
			multiplierName = "B";
		}
		else if(data->data->dltotal < 1024.0D * 1024.0D)
		{
			multiplier = 1 << 10;
			multiplierName = "KB";
		}
		else if(data->data->dltotal < 1024.0D * 1024.0D * 1024.0D)
		{
			multiplier = 1 << 20;
			multiplierName = "MB";
		}
		else
		{
			multiplier = 1 << 30;
			multiplierName = "GB";
		}
		barToFrame(1, 65, 30, (float)(data->data->dlnow / data->data->dltotal) * 100.0f);
		sprintf(tmpString, "%.2f / %.2f %s", data->data->dlnow / multiplier, data->data->dltotal / multiplier, multiplierName);
		textToFrame(1, 96, tmpString);
	}
	
	if(dling)
	{
		char buf[32];
		getSpeedString(dls, buf);
		
		downloaded = dlnow;
		textToFrame(0, ALIGNED_RIGHT, buf);
	}
	
	writeScreenLog();
	drawFrame();
	showFrame();
	return 0;
}

int dlbgThreadMain(int argc, const char **argv)
{
	debugPrintf("Socket optimizer running!");
	
	void *buf = MEMAllocFromDefaultHeapEx(SOCKLIB_BUFSIZE, 64);
	if(buf == NULL)
	{
		debugPrintf("Socket optimizer: OUT OF MEMORY!");
		return 1;
	}
	
	if(somemopt(0x01, buf, SOCKLIB_BUFSIZE, 0) == -1 && socketlasterr() != 50)
	{
		debugPrintf("somemopt failed!");
		return 1;
	}
	
	MEMFreeToDefaultHeap(buf);
	// debugPrintf("Socket optimizer finished!"); // We have no network at this point in time, so we can't use the UDP log.
	return 0;
}

bool initDownloader()
{
	dlbgThreadStack = MEMAllocFromDefaultHeapEx(DLBGT_STACK_SIZE, 8);
	
	if(dlbgThreadStack == NULL || !OSCreateThread(&dlbgThread, dlbgThreadMain, 0, NULL, dlbgThreadStack + DLBGT_STACK_SIZE, DLBGT_STACK_SIZE, 0, OS_THREAD_ATTRIB_AFFINITY_ANY))
		return false;
	
	OSSetThreadName(&dlbgThread, "NUSspli socket optimizer");
	OSResumeThread(&dlbgThread);
	return true;
}

void deinitDownloader()
{
	shutdownDebug();
	socket_lib_finish();
	debugInit();
	int ret;
	OSJoinThread(&dlbgThread, &ret);
	MEMFreeToDefaultHeap(dlbgThreadStack);
	debugPrintf("Socket optimizer returned: %d", ret);
}

static curl_off_t initSocket(void *ptr, curl_socket_t socket, curlsocktype type)
{
	int o = 1;
	// Activate WinScale
	if(setsockopt(socket, SOL_SOCKET, SO_WINSCALE, &o, sizeof(o)) != 0)
		return 1;
	
	//Activate TCP SAck
	if(setsockopt(socket, SOL_SOCKET, SO_TCPSACK, &o, sizeof(o)) != 0)
		return 1;
	
	// Activate userspace buffer (fom our socket optimizer)
	if(setsockopt(socket, SOL_SOCKET, 0x10000, &o, sizeof(o)) != 0)
		return 1;
	
	o = IO_BUFSIZE;
	// Set send buffersize
	if(setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &o, sizeof(o)) != 0)
		return 1;
	
	// Set receive buffersize
	if(setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &o, sizeof(o)) != 0)
		return 1;
	
	return 0;
}

int downloadFile(const char *url, char *file, downloadData *data, FileType type, bool resume)
{
	//Results: 0 = OK | 1 = Error | 2 = No ticket aviable | 3 = Exit
	//Types: 0 = .app | 1 = .h3 | 2 = title.tmd | 3 = tilte.tik
	bool toRam = (type & FILE_TYPE_TORAM) == FILE_TYPE_TORAM;
	
	debugPrintf("Download URL: %s", url);
	debugPrintf("Download PATH: %s", toRam ? "<RAM>" : file);
	
	if(toRam)
		downloading = file;
	else
	{
		int haystack;
		for(haystack = strlen(file); file[haystack] != '/'; haystack--)
		{
		}
		downloading = &file[++haystack];
	}
	
	downloaded = onDisc = 0.0D;
	
	bool fileExist;
	void *fp;
	uint32_t fileSize;
	if(toRam)
	{
		fileExist = false;
		fp = (void *)open_memstream(&ramBuf, &ramBufSize);
	}
	else
	{
		fileExist = resume && fileExists(file);
		fp = (void *)openFile(file, fileExist ? "rb+" : "wb");
		
		if(fileExist)
		{
			fileSize = getFilesize(((NUSFILE *)fp)->fd);
			fileExist = fileSize > 0;
		}
		else
			fileSize = 0;
	}
	
	CURL *curl = curl_easy_init();
	if(curl == NULL)
	{
		if(toRam)
			fclose((FILE *)fp);
		else
			addToIOQueue(NULL, 0, 0, (NUSFILE *)fp);
		
		char err[128];
		sprintf(err, "ERROR: curl_easy_init failed\nFile: %s", file);
		drawErrorFrame(err, B_RETURN);
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(err, B_RETURN);
		
			showFrame();
			if(vpad.trigger & VPAD_BUTTON_B)
				break;
		}
		return 1;
	}
	
	CURLcode ret = curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, (curl_sockopt_callback)initSocket);
	
	ret |= curl_easy_setopt(curl, CURLOPT_URL, url);
	
	char curlError[CURL_ERROR_SIZE];
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlError);
	curlError[0] = '\0';
	
	ret |= curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
	
	uint32_t realFileSize = fileSize;
	if(fileExist)
	{
		if(resume)
		{
			onDisc = fileSize;
			ret |= curl_easy_setopt(curl, CURLOPT_RESUME_FROM, fileSize);
			fseek(((NUSFILE *)fp)->fd, 0, SEEK_END);
		}
		ret |= curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
		ret |= curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &fileSize);
	}
	
	ret |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, toRam ? fwrite : (size_t (*)(const void *, size_t, size_t, FILE *))addToIOQueue);
	ret |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, (FILE *)fp);

	ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
	curlProgressData cdata;
	cdata.curl = curl;
	cdata.error = CDE_OK;
	cdata.data = data;
	ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &cdata);
	ret |= curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	
	ret |= curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	
	if(ret != CURLE_OK)
	{
		if(toRam)
			fclose((FILE *)fp);
		else
			addToIOQueue(NULL, 0, 0, (NUSFILE *)fp);
		
		curl_easy_cleanup(curl);
		debugPrintf("curl_easy_setopt error: %s", curlError);
		return 1;
	}
	
	debugPrintf("Calling curl_easy_perform()");
	lastTransfair = 0;
	ret = curl_easy_perform(curl);
	debugPrintf("curl_easy_perform() returned: %d", ret);
	
	if(toRam)
	{
		fflush((FILE *)fp);
		fclose((FILE *)fp);
	}
	else
		addToIOQueue(NULL, 0, 0, (NUSFILE *)fp);
	
	if(ret != CURLE_OK && !(fileExist && ret == CURLE_WRITE_ERROR && fileSize == 0))
	{
		debugPrintf("curl_easy_perform returned an error: %s (%d)\nFile: %s\n\n", curlError, ret, toRam ? "<RAM>" : file);
		curl_easy_cleanup(curl);
		
		char *toScreen = getToFrameBuffer();
		sprintf(toScreen, "curl_easy_perform returned a non-valid value: %d\n\n", ret);
		if(ret == CURLE_ABORTED_BY_CALLBACK)
		{
			switch(cdata.error)
			{
				case CDE_TIMEOUT:
					ret = CURLE_OPERATION_TIMEDOUT;
					break;
				case CDE_OK:
					ret = CURLE_OK;
					break;
				case CDE_CANCELLED:
					// Do nothing
					;
			}
		}
		
		switch(ret) {
			case CURLE_RANGE_ERROR:
				return downloadFile(url, file, data, type, false);
			case CURLE_FAILED_INIT:
			case CURLE_COULDNT_RESOLVE_HOST:
				strcat(toScreen, "---> Network error\nYour WiiU is not connected to the internet,\ncheck the network settings and try again");
				break;
			case CURLE_WRITE_ERROR:
				strcat(toScreen, "---> Error from SD Card\nThe SD Card was extracted or invalid to write data,\nre-insert it or use another one and restart the app");
				break;
			case CURLE_OPERATION_TIMEDOUT:
			case CURLE_GOT_NOTHING:
			case CURLE_SEND_ERROR:
			case CURLE_RECV_ERROR:
			case CURLE_PARTIAL_FILE:
				strcat(toScreen, "---> Network error\nFailed while trying to download data, probably your router was turned off,\ncheck the internet connection and try again");
				break;
			case CURLE_ABORTED_BY_CALLBACK:
				debugPrintf("CURLE_ABORTED_BY_CALLBACK");
				return 1;
			default:
				strcat(toScreen, "---> Unknown error\n");
				strcat(toScreen, curlError);
				break;
		}
		
		drawErrorFrame(toScreen, B_RETURN | Y_RETRY);
		size_t framesLeft = 30 * 60; // 30 seconds with 60 FPS
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(toScreen, B_RETURN | Y_RETRY);
			
			showFrame();
			
			if(vpad.trigger & VPAD_BUTTON_B)
				break;
			if(vpad.trigger & VPAD_BUTTON_Y || (autoResumeEnabled() && --framesLeft == 0))
				return downloadFile(url, file, data, type, resume);
		}
		return 1;
	}
	debugPrintf("curl_easy_perform executed successfully");
	
	double dld;
	long resp;
	if(!toRam && fileExist && fileSize == 0) // File skipped by headerCallback
	{
		dld = 0.0D;
		resp = 200;
	}
	else
	{
		ret = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &dld);
		if(ret != CURLE_OK)
			dld = 0.0D;
		
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp);
		switch(resp)
		{
			case 206: // Resumed download OK
			case 416: // Invalid range request (we assume file already completely on disc)
				resp = 200;
		}
	}
	
	curl_easy_cleanup(curl);
	debugPrintf("The download returned: %u", resp);
	if(resp != 200)
	{
		if(!toRam)
		{
			flushIOQueue();
			remove(file);
		}
		
		if(resp == 404 && (type & FILE_TYPE_TMD) == FILE_TYPE_TMD) //Title.tmd not found
		{
			drawErrorFrame("The download of title.tmd failed with error: 404\n\nThe title cannot be found on the NUS, maybe the provided title ID doesn't exists or\nthe TMD was deleted", B_RETURN | Y_RETRY);
			
			while(AppRunning())
			{
				if(app == APP_STATE_BACKGROUND)
					continue;
				if(app == APP_STATE_RETURNING)
					drawErrorFrame("The download of title.tmd failed with error: 404\n\nThe title cannot be found on the NUS, maybe the provided title ID doesn't exists or\nthe TMD was deleted", B_RETURN | Y_RETRY);
				
				showFrame();

				if(vpad.trigger & VPAD_BUTTON_B)
					break;
				if(vpad.trigger & VPAD_BUTTON_Y)
					return downloadFile(url, file, data, type, resume);
			}
			return 1;
		}
		else if (resp == 404 && (type & FILE_TYPE_TIK) == FILE_TYPE_TIK) { //Fake ticket needed
			return 2;
		}
		else
		{
			char *toScreen = getToFrameBuffer();
			sprintf(toScreen, "The download returned a result different to 200 (OK): %ld\nFile: %s\n\n", resp, file);
			if(resp == 400)
				strcat(toScreen, "Request failed. Try again\n\n");
			drawErrorFrame(toScreen, B_RETURN | Y_RETRY);
			
			while(AppRunning())
			{
				if(app == APP_STATE_BACKGROUND)
					continue;
				if(app == APP_STATE_RETURNING)
					drawErrorFrame(toScreen, B_RETURN | Y_RETRY);
				
				showFrame();
				
				if(vpad.trigger & VPAD_BUTTON_B)
					break;
				if(vpad.trigger & VPAD_BUTTON_Y)
					return downloadFile(url, file, data, type, resume);
			}
			return 1;
		}
	}
	
	if(data != NULL)
	{
		if(fileExist)
			dld += (double) realFileSize;
		
		data->dltmp += dld;
	}
	
	debugPrintf("The file was downloaded successfully");
	addToScreenLog("Download %s finished!", downloading);
	
	return 0;
}

void showPrepScreen(const char *gameName)
{
	startNewFrame();
	textToFrame(0, 0, "Preparing the download of");
	textToFrame(1, 3, gameName == NULL ? "NULL" : gameName);
	writeScreenLog();
	drawFrame();
	showFrame();
}

bool downloadTitle(const TMD *tmd, size_t tmdSize, const TitleEntry *titleEntry, const char *titleVer, char *folderName, bool inst, bool dlToUSB, bool toUSB, bool keepFiles)
{
	char *tid = hex(tmd->tid, 16);
	
	showPrepScreen(titleEntry->name);
	debugPrintf("Downloading title... tID: %s, tVer: %s, name: %s, folder: %s", tid, titleVer, titleEntry->name, folderName);
	
	char downloadUrl[128];
	strcpy(downloadUrl, DOWNLOAD_URL);
	strcat(downloadUrl, tid);
	strcat(downloadUrl, "/");
	
	if(folderName[0] == '\0')
	{
		for(int i = 0; i < strlen(titleEntry->name); i++)
			folderName[i] = isAllowedInFilename(titleEntry->name[i]) ? titleEntry->name[i] : '_';
	}
	strcpy(folderName + strlen(titleEntry->name), " [");
	strcat(folderName, tid);
	strcat(folderName, "]");
	MEMFreeToDefaultHeap(tid);
	
	if(strlen(titleVer) > 0)
	{
		strcat(folderName, " v");
		strcat(folderName, titleVer);
	}
	
	char installDir[FILENAME_MAX + 37];
	strcpy(installDir, dlToUSB ? INSTALL_DIR_USB : INSTALL_DIR_SD);
	if(!dirExists(installDir))
	{
		debugPrintf("Creating directory \"%s\"", installDir);
		NUSFS_ERR err = createDirectory(installDir, 777);
		if(err == NUSFS_ERR_NOERR)
			addToScreenLog("Download directory successfully created");
		else
		{
			char *toScreen = getToFrameBuffer();
			strcpy(toScreen, "Error creating temporary directory!\n\n");
			const char *errStr = translateNusfsErr(err);
			if(errStr == NULL)
				sprintf(toScreen, "Error creating directory: %d", err);
			else
				strcpy(toScreen, errStr);

			drawErrorFrame(toScreen, B_RETURN);

			while(AppRunning())
			{
				if(app == APP_STATE_BACKGROUND)
					continue;
				if(app == APP_STATE_RETURNING)
					drawErrorFrame(toScreen, B_RETURN);

				showFrame();

				if(vpad.trigger & VPAD_BUTTON_B)
					break;
			}
			return false;
		}
	}
	
	strcat(installDir, folderName);
	strcat(installDir, "/");
	
	addToScreenLog("Started the download of \"%s\"", titleEntry->name);
	addToScreenLog("The content will be saved on \"%s:/install/%s\"", dlToUSB ? "usb" : "sd", folderName);
	
	if(!dirExists(installDir))
	{
		debugPrintf("Creating directory \"%s\"", installDir);
		NUSFS_ERR err = createDirectory(installDir, 777);
		if(err == NUSFS_ERR_NOERR)
			addToScreenLog("Download directory successfully created");
		else
		{
			char *toScreen = getToFrameBuffer();
			strcpy(toScreen, "Error creating temporary directory!\n\n");
			const char *errStr = translateNusfsErr(err);
			if(errStr == NULL)
				sprintf(toScreen, "Error creating directory: %d", err);
			else
				strcpy(toScreen, errStr);
			
			drawErrorFrame(toScreen, B_RETURN);
			
			while(AppRunning())
			{
				if(app == APP_STATE_BACKGROUND)
					continue;
				if(app == APP_STATE_RETURNING)
					drawErrorFrame(toScreen, B_RETURN);
				
				showFrame();
				
				if(vpad.trigger & VPAD_BUTTON_B)
					break;
			}
			return false;
		}
	}
	else
		addToScreenLog("WARNING: The download directory already exists");
	
	debugPrintf("Saving TMD...");
	char tmdp[FILENAME_MAX + 37];
	strcpy(tmdp, installDir);
	strcat(tmdp, "title.tmd");
	
	NUSFILE *fp = openFile(tmdp, "wb");
	if(fp == NULL)
	{
		drawErrorFrame("Can't save TMD file!", B_RETURN);
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame("Can't save TMD file!", B_RETURN);
			
			showFrame();
			
			if(vpad.trigger & VPAD_BUTTON_B)
				break;
		}
		return false;
	}
	
	addToIOQueue(tmd, 1, tmdSize, fp);
	addToIOQueue(NULL, 0, 0, fp);
	addToScreenLog("TMD saved");
	
	char *toScreen = getToFrameBuffer();
	strcpy(toScreen, "=>Title type: ");
	bool hasDependencies;
	switch(*(uint32_t *)&(tmd->tid)) //Title type
	{
		case TID_HIGH_GAME:
			strcat(toScreen, "eShop or Packed");
			hasDependencies = false;
			break;
		case TID_HIGH_DEMO:
			strcat(toScreen, "eShop/Kiosk demo");
			hasDependencies = false;
			break;
		case TID_HIGH_DLC:
			strcat(toScreen, "eShop DLC");
			hasDependencies = true;
			break;
		case TID_HIGH_UPDATE:
			strcat(toScreen, "eShop Update");
			hasDependencies = true;
			break;
		case TID_HIGH_SYSTEM_APP:
			strcat(toScreen, "System Application");
			hasDependencies = false;
			break;
		case TID_HIGH_SYSTEM_DATA:
			strcat(toScreen, "System Data Archive");
			hasDependencies = false;
			break;
		case TID_HIGH_SYSTEM_APPLET:
			strcat(toScreen, "Applet");
			hasDependencies = false;
			break;
		// vWii //
		case TID_HIGH_VWII_IOS:
			strcat(toScreen, "Wii IOS");
			hasDependencies = false;
			break;
		case TID_HIGH_VWII_SYSTEM_APP:
			strcat(toScreen, "vWii System Application");
			hasDependencies = false;
			break;
		case TID_HIGH_VWII_SYSTEM:
			strcat(toScreen, "vWii System Channel");
			hasDependencies = false;
			break;
		default:
			sprintf(toScreen + strlen(toScreen), "Unknown (0x%08X)", *(uint32_t *)&(tmd->tid));
			hasDependencies = false;
			break;
	}
	addToScreenLog(toScreen);
	char tDownloadUrl[256];
	strcpy(tDownloadUrl, downloadUrl);
	strcat(tDownloadUrl, "cetk");
	
	char tInstallDir[FILENAME_MAX + 37];
	strcpy(tInstallDir, installDir);
	strcat(tInstallDir, "title.tik");
	FileType ft;
	if(!fileExists(tInstallDir))
	{
		ft = FILE_TYPE_TIK;
		if(dlToUSB)
			ft |= FILE_TYPE_TOUSB;
		
		int tikRes = downloadFile(tDownloadUrl, tInstallDir, NULL, ft, true);
		if(tikRes == 1)
		{
			return true;
		}
		if(tikRes == 2)
		{
			addToScreenLog("title.tik not found on the NUS. Generating...");
			startNewFrame();
			textToFrame(0, 0, "Creating fake title.tik");
			writeScreenLog();
			drawFrame();
			showFrame();
			
			generateTik(tInstallDir, titleEntry);
			addToScreenLog("Fake ticket created successfully");
		}
	}
	else
		addToScreenLog("title.tik skipped!");
	
	if(!AppRunning())
		return true;
	
	strcpy(tInstallDir, installDir);
	strcat(tInstallDir, "title.cert");
	
	if(!fileExists(tInstallDir))
	{
		debugPrintf("Creating CERT...");
		startNewFrame();
		textToFrame(0, 0, "Creating CERT");
		writeScreenLog();
		drawFrame();
		showFrame();
		
		NUSFILE *cert = openFile(tInstallDir, "wb");
		
		// NUSspli adds its own header.
		writeHeader(cert, FILE_TYPE_CERT);
		
		// Some SSH certificate
		writeCustomBytes(cert, "0x526F6F742D43413030303030303033"); // "Root-CA00000003"
		writeVoidBytes(cert, 0x34);
		writeCustomBytes(cert, "0x0143503030303030303062"); // "?CP0000000b"
		writeVoidBytes(cert, 0x36);
		writeRandomBytes(cert, 0x104);
		writeCustomBytes(cert, "0x00010001");
		writeVoidBytes(cert, 0x34);
		writeCustomBytes(cert, "0x00010003");
		writeRandomBytes(cert, 0x200);
		writeVoidBytes(cert, 0x3C);
		
		// Next certificate
		writeCustomBytes(cert, "0x526F6F74"); // "Root"
		writeVoidBytes(cert, 0x3F);
		writeCustomBytes(cert, "0x0143413030303030303033"); // "?CA00000003"
		writeVoidBytes(cert, 0x36);
		writeRandomBytes(cert, 0x104);
		writeCustomBytes(cert, "0x00010001");
		writeVoidBytes(cert, 0x34);
		writeCustomBytes(cert, "0x00010004");
		writeRandomBytes(cert, 0x100);
		writeVoidBytes(cert, 0x3C);
		
		// Last certificate
		writeCustomBytes(cert, "0x526F6F742D43413030303030303033"); // "Root-CA00000003"
		writeVoidBytes(cert, 0x34);
		writeCustomBytes(cert, "0x0158533030303030303063"); // "?XS0000000c"
		writeVoidBytes(cert, 0x36);
		writeRandomBytes(cert, 0x104);
		writeCustomBytes(cert, "0x00010001");
		writeVoidBytes(cert, 0x34);
		
		addToIOQueue(NULL, 0, 0, cert);
		
		addToScreenLog("Cert created!");
		if(!AppRunning())
			return true;
	}
	else
		addToScreenLog("Cert skipped!");
	
	downloadData data;
	data.contents = tmd->num_contents;
	data.dcontent = 0;
	data.dlnow = data.dltmp = data.dltotal = 0.0D;
	
	char *apps[data.contents];
	bool h3[data.contents];
	
	//Get .app and .h3 files
	for(int i = 0; i < tmd->num_contents; i++)
	{
		apps[i] = hex(tmd->contents[i].cid, 8);
		h3[i] = (tmd->contents[i].type & 0x0003) == 0x0003;
		if(h3[i])
		{
			data.contents++;
			data.dltotal += 20;
		}
		
		data.dltotal += (double)(tmd->contents[i].size);
	}
	
	disableApd();
	char tmpFileName[FILENAME_MAX + 37];
	for(int i = 0; i < tmd->num_contents && AppRunning(); i++)
	{
		strcpy(tDownloadUrl, downloadUrl);
		strcat(tDownloadUrl, apps[i]);
		strcpy(tmpFileName, installDir);
		strcat(tmpFileName, apps[i]);
		MEMFreeToDefaultHeap(apps[i]);
		
		strcpy(tInstallDir, tmpFileName);
		strcat(tInstallDir, ".app");
		
		ft = FILE_TYPE_APP;
		if(dlToUSB)
			ft |= FILE_TYPE_TOUSB;
	
		if(downloadFile(tDownloadUrl, tInstallDir, &data, ft, true) == 1)
		{
			for(int j = ++i; j < tmd->num_contents; j++)
				MEMFreeToDefaultHeap(apps[j]);
			enableApd();
			return true;
		}
		data.dcontent++;
		
		if(h3[i])
		{
			strcat(tDownloadUrl, ".h3");
			strcat(tmpFileName, ".h3");
			
			ft = FILE_TYPE_H3;
			if(dlToUSB)
				ft |= FILE_TYPE_TOUSB;
			
			if(downloadFile(tDownloadUrl, tmpFileName, &data, ft, true) == 1)
			{
				for(int j = ++i; j < tmd->num_contents; j++)
					MEMFreeToDefaultHeap(apps[j]);
				enableApd();
				return true;
			}
			data.dcontent++;
		}
	}
	
	if(!AppRunning())
	{
		enableApd();
		return true;
	}
	
	if(inst)
		return install(titleEntry->name, hasDependencies, dlToUSB, installDir, toUSB, keepFiles);
	
	colorStartNewFrame(SCREEN_COLOR_D_GREEN);
	textToFrame(0, 0, titleEntry->name);
	textToFrame(1, 0, "Downloaded successfully!");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	startRumble();
	enableApd();
	
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
		{
			colorStartNewFrame(SCREEN_COLOR_D_GREEN);
			textToFrame(0, 0, titleEntry->name);
			textToFrame(1, 0, "Downloaded successfully!");
			writeScreenLog();
			drawFrame();
		}
		
		showFrame();
		
		if(vpad.trigger)
			break;
	}
	
	return true;
}

void clearRamBuf()
{
	if(ramBuf == NULL)
		return;
	
	ramBufSize = 0;
	MEMFreeToDefaultHeap(ramBuf);
	ramBuf = NULL;
}
