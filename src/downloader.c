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

#include <errno.h>
#include <file.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include <config.h>
#include <downloader.h>
#include <input.h>
#include <installer.h>
#include <ioThread.h>
#include <memdebug.h>
#include <menu/utils.h>
#include <renderer.h>
#include <rumbleThread.h>
#include <status.h>
#include <ticket.h>
#include <titles.h>
#include <utils.h>

#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/time.h>
#include <curl/curl.h>

#define USERAGENT "NUSspli/"NUSSPLI_VERSION" (WarezLoader, like WUPDownloader)" // TODO: Spoof eShop here?

uint16_t contents = 0xFFFF; //Contents count
uint16_t dcontent = 0xFFFF; //Actual content number

double downloaded;
double onDisc;

char *ramBuf = NULL;
size_t ramBufSize = 0;

char *downloading = "UNKNOWN";
bool downloadPaused = false;
OSTime lastDraw = 0;
OSTime lastTransfair = 0;

static size_t headerCallback(void *buf, size_t size, size_t multi, void *rawData)
{
	size *= multi;
	((char *)buf)[size - 1] = '\0'; //TODO: This should transform the newline at the end to a string terminator - but are we allowed to modify the buffer?
	
	toLowercase(buf);
	uint32_t contentLength = 0;
	if(sscanf(buf, "content-length: %u", &contentLength) != 1)
		return size;
	
	if(*(uint32_t *)rawData == contentLength)
	{
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
} curlProgressData;

static int progressCallback(void *rawData, double dltotal, double dlnow, double ultotal, double ulnow)
{
	if(AppRunning())
	{
/*
		if(app == APP_STATE_BACKGROUND)
		{
			if(!downloadPaused && curl_easy_pause(((curlProgressData *)rawData)->curl, CURLPAUSE_ALL) == CURLE_OK)
			{
				downloadPaused = true;
				debugPrintf("Download paused!");
			}
			return 0;
		}
		if(downloadPaused && curl_easy_pause(((curlProgressData *)rawData)->curl, CURLPAUSE_CONT) == CURLE_OK)
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
		((curlProgressData *)rawData)->error = CDE_CANCELLED;
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
	//debugPrintf("Downloading: %s (%u/%u) [%u%%] %u / %u bytes", downloading, dcontent, contents, (uint32_t)(dlnow / ((dltotal > 0) ? dltotal : 1) * 100), (uint32_t)dlnow, (uint32_t)dltotal);
	startNewFrame();
	char tmpString[13 + strlen(downloading)];
	if(!dling)
	{
		strcpy(tmpString, "Preparing ");
		strcat(tmpString, downloading);
		textToFrame(0, 0, tmpString);
	}
	else
	{
		strcpy(tmpString, "Downloading ");
		strcat(tmpString, downloading);
		textToFrame(0, 0, tmpString);
		
		int multiplier;
		char *multiplierName;
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
		barToFrame(1, 0, 40, (float)(dlnow / dltotal) * 100.0f);
		sprintf(tmpString, "%.2f / %.2f %s", dlnow / multiplier, dltotal / multiplier, multiplierName);
		textToFrame(1, 41, tmpString);
	}
	
	if(contents < 0xFFFF)
	{
		sprintf(tmpString, "(%d/%d)", dcontent + 1, contents);
		textToFrame(0, ALIGNED_RIGHT, tmpString);
	}
	
	if(dling)
	{
		char buf[32];
		getSpeedString(dls, buf);
		
		downloaded = dlnow;
		textToFrame(1, ALIGNED_RIGHT, buf);
	}
	
	writeScreenLog();
	drawFrame();
	showFrame();
	return 0;
}

static size_t headerResumeCallback(void *buf, size_t size, size_t multi, void *rawData)
{
	char *cb = (char *)buf;
	if(cb[0] == '\r')
		return 0;
	
	size *= multi;
	cb[size - 2] = '\0';
	
	toLowercase(cb);
	if(strcmp(cb, "accept-ranges: bytes") == 0)
	{
		*(bool *)rawData = true;
		return 0;
	}
	
	return size;
}

bool isResumeable(const char *url)
{
	CURL *curl = curl_easy_init();
	if(curl == NULL)
		return false;
	
	CURLcode ret = curl_easy_setopt(curl, CURLOPT_URL, url);
	ret |= curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
	ret |= curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	
	bool out = false;
	ret |= curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerResumeCallback);
	ret |= curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &out);
	
	if(ret == CURLE_OK)
		curl_easy_perform(curl);
	
	curl_easy_cleanup(curl);
	return out;
}

int downloadFile(const char *url, char *file, FileType type)
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
	FILE *fp;
	uint32_t fileSize;
	if(toRam)
		fp = open_memstream(&ramBuf, &ramBufSize);
	else
	{
		fileExist = fileExists(file);
		fp = fopen(file, fileExist ? "rb+" : "wb");
		
		if(fileExist)
		{
			fileSize = getFilesize(fp);
			fileExist = fileSize > 0;
		}
		else
			fileSize = 0;
	}
	
	CURL *curl = curl_easy_init();
	if(curl == NULL)
	{
		fclose(fp);
		
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
			if(vpad.trigger ==  VPAD_BUTTON_B)
				return 1;
		}
	}
	
	CURLcode ret = curl_easy_setopt(curl, CURLOPT_URL, url);
	
	char curlError[CURL_ERROR_SIZE];
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlError);
	curlError[0] = '\0';
	
	ret |= curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
	
	if(!toRam && fileExist)
	{
		if(isResumeable(url))
		{
			onDisc = fileSize;
			ret |= curl_easy_setopt(curl, CURLOPT_RESUME_FROM, fileSize);
			fseek(fp, 0, SEEK_END);
		}
		else
		{
			ret |= curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
			ret |= curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &fileSize);
		}
	}
	
	ret |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, toRam ? fwrite : addToIOQueue);
    ret |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
	curlProgressData data;
	data.curl = curl;
	data.error = CDE_OK;
	ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &data);
	ret |= curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	
	ret |= curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	
	if(ret != CURLE_OK)
	{
		fclose(fp);
		curl_easy_cleanup(curl);
		debugPrintf("curl_easy_setopt error: %s", curlError);
		return 1;
	}
	
	debugPrintf("Calling curl_easy_perform()");
	ret = curl_easy_perform(curl);
	debugPrintf("curl_easy_perform() returned: %d", ret);
	
	if(toRam)
	{
		fflush(fp);
		fclose(fp);
	}
	else
		addToIOQueue(NULL, 0, 0, fp);
	
	if(ret != CURLE_OK && (toRam || !(fileExist && ret == CURLE_WRITE_ERROR && fileSize == 0)))
	{
		debugPrintf("curl_easy_perform returned an error: %s (%d)\nFile: %s\n\n", curlError, ret, toRam ? "<RAM>" : file);
		curl_easy_cleanup(curl);
		
		char toScreen[1024];
		sprintf(toScreen, "curl_easy_perform returned a non-valid value: %d\n\n", ret);
		if(ret == CURLE_ABORTED_BY_CALLBACK)
		{
			switch(data.error)
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
				strcat(toScreen, "---> Network error\nFailed while trying to download data, probably your router was turned off,\ncheck the internet connecition and try again");
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
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(toScreen, B_RETURN | Y_RETRY);
			
			showFrame();
			
			switch (vpad.trigger) {
				case VPAD_BUTTON_B:
					return 1;
				case VPAD_BUTTON_Y:
					return downloadFile(url, file, type);
			}
		
		}
	}
	debugPrintf("curl_easy_perform executed successfully");
	
	long resp;
	if(!toRam && fileExist && fileSize == 0) // File skipped by headerCallback
		resp = 200;
	else
	{
		ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp);
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

				switch(vpad.trigger)
				{
					case VPAD_BUTTON_B:
						return 1;
					case VPAD_BUTTON_Y:
						return downloadFile(url, file, type);
				}
			}
		}
		else if (resp == 404 && (type & FILE_TYPE_TIK) == FILE_TYPE_TIK) { //Fake ticket needed
			return 2;
		}
		else
		{
			char toScreen[1024];
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
				
				switch (vpad.trigger) {
					case VPAD_BUTTON_B:
						return 1;
					case VPAD_BUTTON_Y:
						return downloadFile(url, file, type);
				}
			}
		}
	}
	
	debugPrintf("The file was downloaded successfully");
	addToScreenLog("Download %s finished!", downloading);
	
	return 0;
}

bool downloadTitle(const char *tid, const char *titleVer, char *folderName, bool inst, bool dlToUSB, bool toUSB, bool keepFiles)
{
	const char *gameName = tid2name(tid);
	debugPrintf("Downloading title... tID: %s, tVer: %s, name: %s, folder: %s", tid, titleVer, gameName == NULL ? "NULL" : gameName, folderName);
	
	char downloadUrl[128];
	strcpy(downloadUrl, DOWNLOAD_URL);
	strcat(downloadUrl, tid);
	strcat(downloadUrl, "/");
	
	if(folderName[0] == '\0')
	{
		if(gameName != NULL)
		{
			for(int i = 0; i < strlen(gameName); i++)
				folderName[i] = isSpecial(gameName[i]) ? gameName[i] : '_';
			
			strcpy(folderName + strlen(gameName), " [");
			strcat(folderName, tid);
		}
		else
		{
			folderName[0] = '[';
			strcpy(folderName + 1, tid);
		}
			
	}
	else
	{
		strcat(folderName, " [");
		strcat(folderName, tid);
	}
	strcat(folderName, "]");
	
	if(strlen(titleVer) > 0)
	{
		strcat(folderName, " v");
		strcat(folderName, titleVer);
	}
	
	char installDir[FILENAME_MAX + 37];
	strcpy(installDir, dlToUSB ? INSTALL_DIR_USB : INSTALL_DIR_SD);
	if(!dirExists(installDir))
	{
		debugPrintf("Creating directroty \"%s\"", installDir);
		mkdir(installDir, 777);
	}
	
	strcat(installDir, folderName);
	strcat(installDir, "/");
	
	if(gameName == NULL)
		gameName = tid;
	
	addToScreenLog("Started the download of \"%s\"", gameName);
	addToScreenLog("The content will be saved on \"%s:/install/%s\"", dlToUSB ? "usb" : "sd", folderName);
	
	if(!dirExists(installDir))
	{
		debugPrintf("Creating directory");
		errno = 0;
		if(mkdir(installDir, 777) == -1)
		{
			int ie = errno;
			char toScreen[1024];
			switch(ie)
			{
				case FS_ERROR_WRITE_PROTECTED:	// Cafe, not yet mapped by WUT to
				case EROFS:						// POSIX
					strcpy(toScreen, "SD card write locked!");
					break;
				case FS_ERROR_MAX_FILES:
				case FS_ERROR_MAX_DIRS:
				case ENOSPC:
					strcpy(toScreen, "Filesystem limits reached!");
					break;
				default:
					sprintf(toScreen, "Error creating directory: %d %s", ie, strerror(ie));
			}
			
			drawErrorFrame(toScreen, B_RETURN);
			
			while(AppRunning())
			{
				if(app == APP_STATE_BACKGROUND)
					continue;
				if(app == APP_STATE_RETURNING)
					drawErrorFrame(toScreen, B_RETURN);
				
				showFrame();
				
				if(vpad.trigger == VPAD_BUTTON_B)
					return false;
			}
		}
		else
			addToScreenLog("Download directory successfully created");
	}
	else
		addToScreenLog("WARNING: The download directory already exists");
	
	debugPrintf("Downloading TMD...");
	char tDownloadUrl[256];
	strcpy(tDownloadUrl, downloadUrl);
	strcat(tDownloadUrl, "tmd");
	if(strlen(titleVer) > 0)
	{
		strcat(tDownloadUrl, ".");
		strcat(tDownloadUrl, titleVer);
	}
	char tmd[FILENAME_MAX + 37];
	strcpy(tmd, installDir);
	strcat(tmd, "title.tmd");
	contents = 0xFFFF;
	if(downloadFile(tDownloadUrl, tmd, FILE_TYPE_TMD))
	{
		debugPrintf("Error downloading TMD");
		return true;
	}
	flushIOQueue();
	addToScreenLog("TMD Downloaded");
	
	char toScreen[128];
	strcpy(toScreen, "=>Title type: ");
	bool hasDependencies;
	switch(readUInt32(tmd, 0x18C)) //Title type
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
			strcat(toScreen, "Unknown (");
			char *h = hex(readUInt32(tmd, 0x18C), 8);
			strcat(toScreen, h);
			MEMFreeToDefaultHeap(h);
			strcat(toScreen, ")");
			hasDependencies = false;
			break;
	}
	addToScreenLog(toScreen);
	strcpy(tDownloadUrl, downloadUrl);
	strcat(tDownloadUrl, "cetk");
	
	char tInstallDir[FILENAME_MAX + 37];
	strcpy(tInstallDir, installDir);
	strcat(tInstallDir, "title.tik");
	if(!fileExists(tInstallDir))
	{
		int tikRes = downloadFile(tDownloadUrl, tInstallDir, FILE_TYPE_TIK);
		if(tikRes == 1)
			return true;
		if(tikRes == 2)
		{
			addToScreenLog("title.tik not found on the NUS. Generating...");
			startNewFrame();
			textToFrame(0, 0, "Creating fake title.tik");
			writeScreenLog();
			drawFrame();
			showFrame();
			
			generateTik(tInstallDir, tid);
			addToScreenLog("Fake ticket created successfully");
			if(!AppRunning())
				return true;
		}
	}
	else
		addToScreenLog("title.tik skipped!");
	
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
		
		FILE *cert = fopen(tInstallDir, "wb");
		
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

	uint16_t conts = readUInt16(tmd, 0x1DE);
	char *apps[conts];
	bool h3[conts];
	contents = conts;
	
	//Get .app and .h3 files
	for(int i = 0; i < conts; i++)
	{
		apps[i] = hex(readUInt32(tmd, 0xB04 + i * 0x30), 8);
		h3[i] = readUInt8(tmd, 0xB0B + i * 0x30) == 0x03;
		if(h3[i])
			contents++;
	}
	
	char tmpFileName[FILENAME_MAX + 37];
	dcontent = 0;
	for(int i = 0; i < conts && AppRunning(); i++)
	{
		strcpy(tDownloadUrl, downloadUrl);
		strcat(tDownloadUrl, apps[i]);
		strcpy(tmpFileName, installDir);
		strcat(tmpFileName, apps[i]);
		MEMFreeToDefaultHeap(apps[i]);
		
		strcpy(tInstallDir, tmpFileName);
		strcat(tInstallDir, ".app");
		if(downloadFile(tDownloadUrl, tInstallDir, FILE_TYPE_APP) == 1)
		{
			for(int j = ++i; j < conts; j++)
				MEMFreeToDefaultHeap(apps[j]);
			return true;
		}
		dcontent++;
		
		if(h3[i])
		{
			strcat(tDownloadUrl, ".h3");
			strcat(tmpFileName, ".h3");
			if(downloadFile(tDownloadUrl, tmpFileName, FILE_TYPE_H3) == 1)
			{
				for(int j = ++i; j < conts; j++)
					MEMFreeToDefaultHeap(apps[j]);
				return true;
			}
			dcontent++;
		}
	}
	
	if(!AppRunning())
		return true;
	
	startNewFrame();
	textToFrame(0, 0, "Flushing I/O queue");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	flushIOQueue();
	if(inst)
		return install(gameName, hasDependencies, dlToUSB, installDir, toUSB, keepFiles);
	
	colorStartNewFrame(SCREEN_COLOR_D_GREEN);
	textToFrame(0, 0, gameName);
	textToFrame(1, 0, "Downloaded successfully!");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	startRumble();
	
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
		{
			colorStartNewFrame(SCREEN_COLOR_D_GREEN);
			textToFrame(0, 0, gameName);
			textToFrame(1, 0, "Downloaded successfully!");
			writeScreenLog();
			drawFrame();
		}
		
		showFrame();
		
		if(vpad.trigger == VPAD_BUTTON_A || vpad.trigger == VPAD_BUTTON_B)
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
