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

#include <file.h>
#include <input.h>
#include <installer.h>
#include <ioThread.h>
#include <renderer.h>
#include <rumbleThread.h>
#include <status.h>
#include <usb.h>
#include <utils.h>
#include <menu/utils.h>

bool install(const char *game, bool hasDeps, bool fromUSB, const char *path, bool toUsb, bool keepFiles)
{
	startNewFrame();
	char *toScreen = getToFrameBuffer();
	strcpy(toScreen, "Installing ");
	strcat(toScreen, game);
	textToFrame(0, 0, toScreen);
	barToFrame(1, 0, 40, 0);
	textToFrame(1, 41, "Preparing...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	char newPath[0x27F]; // MCP mounts the card at another point, so we have to adjust the path -  The length of 0x27F is important!
	if(fromUSB)
	{
		//TODO
		strcpy(newPath, isUSB01() ? "/vol/storage_usb01" : "/vol/storage_usb02");
		strcat(newPath, path + 4);
	}
	else
	{
		strcpy(newPath, "/vol/app_sd");
		strcat(newPath, path + 18);
	}
	
	MCPInstallTitleInfo info;
	McpData data;
	
	if(!isAroma())
	{
		flushIOQueue(); // Make sure all game files are on disc
		unmountUSB(); // Get MCP ready
	}
	
	// Let's see if MCP is able to parse the TMD...
	data.err = MCP_InstallGetInfo(mcpHandle, newPath, (MCPInstallInfo *)&info);
	if(data.err != 0)
	{
		char toScreen[2048];
		
		switch(data.err)
		{
			case 0xfffbf3e2:
				sprintf(toScreen, "No title.tmd found at \"%s\"", newPath);
				break;
			case 0xfffbfc17:
				sprintf(toScreen, "Internal error installing \"%s\"\nYour Homebrew Launcher Channel is probably outdated!", newPath);
				break;
			default:
				sprintf(toScreen, "Error getting info for \"%s\" from MCP: %#010x", newPath, data.err);
		}
		
		debugPrintf(toScreen);
		addToScreenLog("Installation failed!");
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
	
	// Allright, let's set if we want to install to USB or NAND
	MCPInstallTarget target = toUsb ? MCP_INSTALL_TARGET_USB : MCP_INSTALL_TARGET_MLC;
	data.err = MCP_InstallSetTargetDevice(mcpHandle, target);
	if(data.err != 0 || MCP_InstallSetTargetUsb(mcpHandle, target) != 0)
	{
		char *err = toUsb ? "Error opening USB device" : "Error opening internal memory";
		debugPrintf(err);
		addToScreenLog("Installation failed!");
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
		return false;
	}
	
	// Just some debugging stuff
	debugPrintf("NUSspli path:  %s (%d)", path, strlen(path));
	debugPrintf("MCP Path:      %s (%d)", newPath, strlen(newPath));
	
	// Last preparing step...
	disableShutdown();
	glueMcpData(&info, &data);
	
	if(isAroma())
		flushIOQueue(); // Make sure all game files are on disc
	
	// Start the installation process
	MCPError err = MCP_InstallTitleAsync(mcpHandle, newPath, &info);
	
	if(err != 0)
	{
		sprintf(toScreen, "Error starting async installation of \"%s\": %#010x", newPath, data.err);
		debugPrintf(toScreen);
		addToScreenLog("Installation failed!");
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
		enableShutdown();
		return false;
	}
	
	showMcpProgress(&data, game, true);
	// Quarkys ASAN catched this / seems like MCP already frees it for us
//	MEMFreeToDefaultHeap(progress);
	
	// MCP thread finished. Let's see if we got any error - TODO: This is a 1:1 copy&paste from WUP Installer GX2 which itself stole it from WUP Installer Y mod which got it from WUP Installer minor edit by Nexocube who got it from WUP installer JHBL Version by Dimrok who portet it from the ASM of WUP Installer. So I think it's time for something new... ^^
	if(data.err != 0)
	{
		debugPrintf("Installation failed with result: %#010x", data.err);
		strcpy(toScreen, "Installation failed!\n\n");
		switch(data.err)
		{
			case 0xFFFCFFE9:
				if(hasDeps)
				{
					strcat(toScreen, "Install the main game to the same storage medium first");
					if(toUsb)
						strcat(toScreen, "\nAlso make sure there is no error with the USB drive");
				}
				else if(toUsb)
					strcat(toScreen, "Possible USB error");
				break;
			case 0xFFFBF446:
			case 0xFFFBF43F:
				strcat(toScreen, "Possible missing or bad title.tik file");
				break;
			case 0xFFFBF441:
				strcat(toScreen, "Possible incorrect console for DLC title.tik file");
				break;
			case 0xFFFCFFE4:
				strcat(toScreen, "Possible not enough memory on target device");
				break;
			case 0xFFFFF825:
			case 0xFFFFF82E:
				strcat(toScreen, "Possible corrupted files or bad storage medium.\nTry redownloading files or reformat/replace target device");
				break;
			default:
				if ((data.err & 0xFFFF0000) == 0xFFFB0000)
					strcat(toScreen, "Verify WUP files are correct & complete.\nDLC/E-shop require Sig Patch");
				else
					sprintf(toScreen + 12, "Unknown Error: %#010x", data.err);
		}
		
		addToScreenLog("Installation failed!");
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
		enableShutdown();
		return false;
	}

	addToScreenLog("Installation finished!");
	
	if(!fromUSB && !keepFiles)
	{
		debugPrintf("Removing installation files...");
		removeDirectory(path);
	}
	
	colorStartNewFrame(SCREEN_COLOR_D_GREEN);
	textToFrame(0, 0, game);
	textToFrame(1, 0, "Installed successfully!");
	writeScreenLog();
	drawFrame();

	startRumble();
	enableShutdown();
	
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
		{
			colorStartNewFrame(SCREEN_COLOR_D_GREEN);
			textToFrame(0, 0, game);
			textToFrame(1, 0, "Installed successfully!");
			writeScreenLog();
			drawFrame();
		}
		
		showFrame();
		
		if(vpad.trigger)
			break;
	}
	return true;
}
