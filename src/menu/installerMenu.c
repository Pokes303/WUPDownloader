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

#include <file.h>
#include <input.h>
#include <installer.h>
#include <renderer.h>
#include <status.h>
#include <tmd.h>
#include <utils.h>
#include <menu/utils.h>

#include <coreinit/memdefaultheap.h>

#include <stdio.h>
#include <string.h>

static void drawInstallerMenuFrame(const char *name, NUSDEV dev, bool keepFiles)
{
	startNewFrame();
	
	textToFrame(0, 0, name);
	
	lineToFrame(MAX_LINES - 6, SCREEN_COLOR_WHITE);
	textToFrame(MAX_LINES - 5, 0, "Press " BUTTON_A " to install to USB");
	textToFrame(MAX_LINES - 4, 0, "Press " BUTTON_X " to install to NAND");
	textToFrame(MAX_LINES - 3, 0, "Press " BUTTON_B " to return");
	
	lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);
	if(dev != NUSDEV_SD)
		textToFrame(MAX_LINES - 1, 0, "WARNING: Files on USB/NAND will always be deleted after installing!");
	else
	{
		char *toFrame = getToFrameBuffer();
		strcpy(toFrame, "Press " BUTTON_LEFT " to ");
		strcat(toFrame, keepFiles ? "delete" : "keep");
		strcat(toFrame, " files after the installation");
		textToFrame(MAX_LINES - 1, 0, toFrame);
	}
	
	drawFrame();
}

static bool brickCheck(const char* dir)
{
	char tmdPath[strlen(dir) + 11];
	strcpy(tmdPath, dir);
	strcat(tmdPath, "/title.tmd");

	FILE *f = fopen(tmdPath, "rb");
	if(f == NULL)
	{
		debugPrintf("Error opening %s", tmdPath);
		return false;
	}

	size_t s = getFilesize(f);
	TMD *tmd = MEMAllocFromDefaultHeap(s);
	if(tmd == NULL)
	{
		debugPrintf("OUT OF MEMORY!", tmdPath);
		fclose(f);
		return false;
	}

	if(fread(tmd, s, 1, f) != 1)
	{
		debugPrintf("Error reading %s", tmdPath);
		fclose(f);
		MEMFreeToDefaultHeap(tmd);
		return false;
	}

	fclose(f);
	bool ret = checkSystemTitleFromTid(tmd->tid);
	MEMFreeToDefaultHeap(tmd);
	return ret;
}

void installerMenu(const char *dir)
{
	NUSDEV dev = dir[3] == ':' ? dir[0] == 'u' ? NUSDEV_USB : NUSDEV_MLC : NUSDEV_SD;
	bool keepFiles;
	char name[strlen(dir) + 1];
	if(dev == NUSDEV_SD)
	{
		keepFiles = true;
		strcpy(name, "SD:");
		strcat(name, dir + 18);
	}
	else
	{
		keepFiles = false;
		strcpy(name, dir);
	}
	
	drawInstallerMenuFrame(name, dev, keepFiles);
	
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawInstallerMenuFrame(name, dev, keepFiles);
		
		showFrame();
		
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			if(brickCheck(dir))
				install(name, false, dev, dir, true, keepFiles);
			return;
		}
		if(vpad.trigger & VPAD_BUTTON_B)
			return;
		if(vpad.trigger & VPAD_BUTTON_X)
		{
			if(brickCheck(dir))
				install(name, false, dev, dir, false, keepFiles);
			return;
		}
		
		if(vpad.trigger & VPAD_BUTTON_LEFT)
		{
			keepFiles = !keepFiles;
			drawInstallerMenuFrame(name, dev, keepFiles);
		}
	}
}
