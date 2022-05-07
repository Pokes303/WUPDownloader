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

#include <config.h>
#include <deinstaller.h>
#include <downloader.h>
#include <filesystem.h>
#include <input.h>
#include <renderer.h>
#include <state.h>
#include <titles.h>
#include <tmd.h>
#include <utils.h>
#include <menu/predownload.h>
#include <menu/utils.h>

#include <stdbool.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

static inline bool isInstalled(const TitleEntry *entry, MCPTitleListType *out)
{
	if(out == NULL)
	{
		MCPTitleListType titleList;
		out = &titleList;
	}
	return MCP_GetTitleInfo(mcpHandle, entry->tid, out) == 0;
}

static void drawPDMenuFrame(const TitleEntry *entry, const char *titleVer, uint64_t size, bool installed, const char *folderName, bool usbMounted, NUSDEV dlDev, bool keepFiles)
{
	startNewFrame();
	textToFrame(0, 0, "Name:");
	
	char *toFrame = getToFrameBuffer();
	strcpy(toFrame, entry->name);
    char tid[17];
	hex(entry->tid, 16, tid);
	strcat(toFrame, " [");
	strcat(toFrame, tid);
	strcat(toFrame, "]");
	textToFrame(1, 3, toFrame);
	
	char *bs;
	float fsize;
	if(size > 1024 * 1024 * 1024)
	{
		bs = "GB";
		fsize = ((float)(size / 1024 / 1024)) / 1024.0F;
	}
	else if(size > 1024 * 1924)
	{
		bs = "MB";
		fsize = ((float)(size / 1024)) / 1024.0F;
	}
	else if(size > 1024)
	{
		bs = "KB";
		fsize = ((float)size) / 1024.0F;
	}
	else
	{
		bs = "B";
		fsize = (float)size;
	}
	
	sprintf(toFrame, "%.02f %s", fsize, bs);
	textToFrame(2, 0, "Size:");
	textToFrame(3, 3, toFrame);
	
	textToFrame(4, 0, "Provided title version [Only numbers]:");
	textToFrame(5, 3, titleVer[0] == '\0' ? "<LATEST>" : titleVer);
	
	textToFrame(6, 0, "Custom folder name [ASCII only]:");
	textToFrame(7, 3, folderName);
	
	int line = MAX_LINES - 1;
	strcpy(toFrame, "Press " BUTTON_MINUS " to download to ");
	switch(dlDev)
	{
		case NUSDEV_USB:
			strcat(toFrame, "SD");
			break;
		case NUSDEV_SD:
			strcat(toFrame, "NAND");
			break;
		case NUSDEV_MLC:
			strcat(toFrame, usbMounted ? "USB" : "SD");
	}
	textToFrame(line--, 0, toFrame);

	if(dlDev != NUSDEV_SD)
		textToFrame(line--, 0, "WARNING: Files on USB/NAND will always be deleted after installing!");
	else
	{
		strcpy(toFrame, "Press " BUTTON_LEFT " to ");
		strcat(toFrame, keepFiles ? "delete" : "keep");
		strcat(toFrame, " downloaded files after the installation");
		textToFrame(line--, 0, toFrame);
	}
	
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	textToFrame(line--, 0, "Press " BUTTON_DOWN " to set a custom name to the download folder");
	textToFrame(line--, 0, "Press " BUTTON_RIGHT " to set the title version");
	lineToFrame(line--, SCREEN_COLOR_WHITE);
	
	textToFrame(line--, 0, "Press " BUTTON_B " to return");
	
	strcpy(toFrame, "Press " BUTTON_Y " to download to ");
	strcat(toFrame, dlDev == NUSDEV_USB ? "USB" : dlDev == NUSDEV_SD ? "SD" : "NAND");
	strcat(toFrame, " only");
	textToFrame(line--, 0, toFrame);
	
	textToFrame(line--, 0, "Press " BUTTON_X " to install to NAND");
	if(usbMounted)
		textToFrame(line--, 0, "Press " BUTTON_A " to install to USB");
	
	if(installed)
		textToFrame(line--, 0, "Press \uE079 to uninstall");
	
	lineToFrame(line, SCREEN_COLOR_WHITE);
	
	drawFrame();
}

static void drawPDDemoFrame(const TitleEntry *entry, bool inst)
{
	startNewFrame();
	char *toFrame = getToFrameBuffer();
	strcpy(toFrame, entry->name);
	strcat(toFrame, " is a demo.");
	textToFrame(0, 0, toFrame);
	
	int line = MAX_LINES - 1;
	textToFrame(line--, 0, "Press \uE001 to continue");
    sprintf(toFrame, "Press \uE000 to %s the main game instead", inst ? "install" : "download");
	textToFrame(line--, 0, toFrame);
	lineToFrame(line, SCREEN_COLOR_WHITE);
	
	drawFrame();
}

#include <inttypes.h>

void predownloadMenu(const TitleEntry *entry)
{
	MCPTitleListType titleList;
	bool installed = isInstalled(entry, &titleList);
	bool usbMounted = mountUSB();
    mountMLC();
    NUSDEV dlDev = usbMounted && dlToUSBenabled() ? NUSDEV_USB : NUSDEV_SD;
	bool keepFiles = true;
	char folderName[FILENAME_MAX - 11];
	char titleVer[33];
	folderName[0] = titleVer[0] = '\0';
	TMD *tmd;
	uint64_t dls;
	
	bool loop = true;
	bool inst, toUSB;
	bool redraw = false;
	
	char tid[17];
	char downloadUrl[256];
downloadTMD:
	hex(entry->tid, 16, tid);
	
	debugPrintf("Downloading TMD...");
	strcpy(downloadUrl, DOWNLOAD_URL);
	strcat(downloadUrl, tid);
	strcat(downloadUrl, "/tmd");
	
	if(strlen(titleVer) > 0)
	{
		strcat(downloadUrl, ".");
		strcat(downloadUrl, titleVer);
	}
	
	if(downloadFile(downloadUrl, "TMD", NULL, FILE_TYPE_TMD | FILE_TYPE_TORAM, true))
	{
		clearRamBuf();
		debugPrintf("Error downloading TMD");
		saveConfig(false);
		return;
	}
	
	tmd = (TMD *)getRamBuf();
	dls = 0;
	for(uint16_t i = 0; i < tmd->num_contents; ++i)
	{
		if((tmd->contents[i].type & 0x0003) == 0x0003)
			dls += 20;
		
		dls += tmd->contents[i].size;
	}

naNedNa:
	drawPDMenuFrame(entry, titleVer, dls, installed, folderName, usbMounted, dlDev, keepFiles);
	
	while(loop && AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawPDMenuFrame(entry, titleVer, dls, installed, folderName, usbMounted, dlDev, keepFiles);
		
		showFrame();
		
		if(vpad.trigger & VPAD_BUTTON_B)
		{
			clearRamBuf();
			saveConfig(false);
			return;
		}
		
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
		
		if(vpad.trigger & VPAD_BUTTON_RIGHT)
		{
			if(!showKeyboard(KEYBOARD_LAYOUT_TID, KEYBOARD_TYPE_RESTRICTED, titleVer, CHECK_NUMERICAL, 5, false, titleVer, NULL))
				titleVer[0] = '\0';
			clearRamBuf();
			goto downloadTMD;
		}
		if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			if(!showKeyboard(KEYBOARD_LAYOUT_TID, KEYBOARD_TYPE_NORMAL, folderName, CHECK_ALPHANUMERICAL, FILENAME_MAX - 11, false, folderName, NULL))
				folderName[0] = '\0';
			redraw = true;
		}
		
		if(vpad.trigger & VPAD_BUTTON_MINUS)
		{
			switch(dlDev)
			{
				case NUSDEV_USB:
					dlDev = NUSDEV_SD;
					keepFiles = true;
					setDlToUSB(false);
					break;
				case NUSDEV_SD:
					dlDev = NUSDEV_MLC;
					keepFiles = false;
					break;
				case NUSDEV_MLC:
					if(usbMounted)
					{
						dlDev = NUSDEV_USB;
						keepFiles = false;
						setDlToUSB(true);
					}
					else
					{
						dlDev = NUSDEV_SD;
						keepFiles = false;
						setDlToUSB(false);
					}
			}
			redraw = true;
		}
		if(vpad.trigger & VPAD_BUTTON_LEFT && dlDev == NUSDEV_SD)
		{
			keepFiles = !keepFiles;
			redraw = true;
		}
		if(installed && vpad.trigger & VPAD_BUTTON_UP)
		{
			clearRamBuf();
			saveConfig(false);
			deinstall(&titleList, entry->name, false);
			return;
		}
		
		if(redraw)
		{
			drawPDMenuFrame(entry, titleVer, dls, installed, folderName, usbMounted, dlDev, keepFiles);
			redraw = false;
		}
	}
	
	if(!AppRunning())
	{
		clearRamBuf();
		return;
	}
	
	saveConfig(false);
	
	if(isDemo(entry))
	{
        uint64_t t = entry->tid;
		t &= 0xFFFFFFF0FFFFFFF0; // TODO
		const TitleEntry *te = getTitleEntryByTid(t);
		if(te != NULL)
		{
			drawPDDemoFrame(entry, inst);

			while(AppRunning())
			{
				if(app == APP_STATE_BACKGROUND)
					continue;
				if(app == APP_STATE_RETURNING)
					drawPDDemoFrame(entry, inst);

				showFrame();

				if(vpad.trigger & VPAD_BUTTON_B)
					break;
				if(vpad.trigger & VPAD_BUTTON_A)
				{
					clearRamBuf();
					entry = te;
					goto downloadTMD;
				}
			}
		}
	}

	if(dlDev == NUSDEV_MLC)
	{
		int ovl = addErrorOverlay(
			"Downloading to NAND is dangerous,\n"
			"it could brick your Wii U!\n\n"

			"Are you sure you want to do this?"
		);

		while(AppRunning())
		{
			showFrame();

			if(vpad.trigger & VPAD_BUTTON_B)
			{
				removeErrorOverlay(ovl);
				loop = true;
				goto naNedNa;
			}
			if(vpad.trigger & VPAD_BUTTON_A)
				break;
		}

		removeErrorOverlay(ovl);
	}

	if(checkSystemTitleFromEntry(entry))
		downloadTitle(tmd, getRamBufSize(), entry, titleVer, folderName, inst, dlDev, toUSB, keepFiles);

	clearRamBuf();
}
