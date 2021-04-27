/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2021 V10lator <v10lator@myway.de>                         *
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

#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>

#include <coreinit/ios.h>
#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <deinstaller.h>
#include <file.h>
#include <input.h>
#include <ioThread.h>
#include <osdefs.h>
#include <renderer.h>
#include <rumbleThread.h>
#include <status.h>
#include <usb.h>
#include <utils.h>
#include <menu/utils.h>

bool deinstall(MCPTitleListType title, bool showFinishScreen)
{
	char *tids = hex(title.titleId, 16);
	if(tids == NULL)
		return false;
	
	char *game = tid2name(tids);
	MEMFreeToDefaultHeap(tids);
	
	startNewFrame();
	textToFrame(0, 0, "Uninstalling");
	textToFrame(0, 19, game);
	textToFrame(1, 0, "Preparing...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	McpData data;
	MCPInstallTitleInfo info;
	initMCPInstallTitleInfo(&info, &data);
	
	unmountUSB();
	if(showFinishScreen)
		disableShutdown();
	
	MCPInstallProgress *progress = MEMAllocFromDefaultHeapEx(sizeof(MCPInstallProgress), 0x40);
	if(progress == NULL)
	{
		debugPrintf("Error allocating memory!");
		enableShutdown();
		return false;
	}
	
	progress->inProgress = 0;
	char multiplierName[3];
	int multiplier = 0;
	char toScreen = getToScreenBuffer();
	MCPError err;
	OSTime lastSpeedCalc = 0;
	OSTime now;
	uint64_t lsp = 0;
	char speedBuf[32];
	
	//err = MCP_UninstallTitleAsync(mcpHandle, title.path, &info);
	// The above crashes MCP, so let's leave WUT:
	debugPrintf("Deleting %s", title.path);
	err = MCP_DeleteTitleAsync(mcpHandle, title.path, &info);
	if(err != 0)
	{
		debugPrintf("Err1: %#010x (%d)", err, err);
		if(showFinishScreen)
			enableShutdown();
		return false;
	}
	
	while(data.processing)
	{
		err = MCP_InstallGetProgress(mcpHandle, progress);
		if(err == IOS_ERROR_OK)
		{
			if(progress->inProgress == 1)
			{
				if(multiplier == 0)
				{
					if(progress->sizeTotal == 0) // TODO
					{
						strcpy(multiplierName, "B");
						progress->sizeTotal = 9999999;
					}
					else if(progress->sizeTotal < 1 << 10)
					{
						multiplier = 1;
						strcpy(multiplierName, "B");
					}
					else if(progress->sizeTotal < 1 << 20)
					{
						multiplier = 1 << 10;
						strcpy(multiplierName, "KB");
					}
					else if(progress->sizeTotal < 1 << 30)
					{
						multiplier = 1 << 20;
						strcpy(multiplierName, "MB");
					}
					else
					{
						multiplier = 1 << 30;
						strcpy(multiplierName, "GB");
					}
				}
				startNewFrame();
				strcpy(toScreen, "Uninstalling ");
				strcat(toScreen, game);
				textToFrame(0, 0, toScreen);
				barToFrame(1, 0, 40, progress->sizeProgress * 100.0f / progress->sizeTotal);
				sprintf(toScreen, "%.2f / %.2f %s", ((float)progress->sizeProgress) / multiplier, ((float)progress->sizeTotal) / multiplier, multiplierName);
				textToFrame(1, 41, toScreen);
				
				if(progress->sizeProgress != 0)
				{
					now = OSGetSystemTime();
					if(OSTicksToMilliseconds(now - lastSpeedCalc) > 333)
					{
						getSpeedString(progress->sizeProgress - lsp, speedBuf);
						lsp = progress->sizeProgress;
						lastSpeedCalc = now;
					}
					textToFrame(1, ALIGNED_RIGHT, speedBuf);
				}
				
				writeScreenLog();
				drawFrame();
			}
		}
		else
			debugPrintf("MCP_InstallGetProgress() returned %#010x", err);
		
		showFrame();
	}
	debugPrintf("Done!");
	
	enableShutdown();
	startRumble();
	
	colorStartNewFrame(SCREEN_COLOR_D_GREEN);
	textToFrame(0, 0, game);
	textToFrame(1, 0, "Uninstalled successfully!");
	writeScreenLog();
	drawFrame();
	
	if(!showFinishScreen)
		return true;
	
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
		{
			colorStartNewFrame(SCREEN_COLOR_D_GREEN);
			textToFrame(0, 0, game);
			textToFrame(1, 0, "Uninstalled successfully!");
			writeScreenLog();
			drawFrame();
		}
		
		showFrame();
		
		if(vpad.trigger)
			break;
	}
	return true;
}
