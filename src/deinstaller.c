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
	
	//err = MCP_UninstallTitleAsync(mcpHandle, title.path, &info);
	// The above crashes MCP, so let's leave WUT:
	debugPrintf("Deleting %s", title.path);
	MCPError err = MCP_DeleteTitleAsync(mcpHandle, title.path, &info);
	if(err != 0)
	{
		debugPrintf("Err1: %#010x (%d)", err, err);
		if(showFinishScreen)
			enableShutdown();
		return false;
	}
	
	showMcpProgress(&data, game, false);
	debugPrintf("Done!");
	
	if(!showFinishScreen)
		return true;
	
	enableShutdown();
	startRumble();
	
	colorStartNewFrame(SCREEN_COLOR_D_GREEN);
	textToFrame(0, 0, game);
	textToFrame(1, 0, "Uninstalled successfully!");
	writeScreenLog();
	drawFrame();
	
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
