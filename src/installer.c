/***************************************************************************
 * This file is part of NUSspli.                                           *
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

#include <crypto.h>
#include <file.h>
#include <filesystem.h>
#include <input.h>
#include <installer.h>
#include <ioQueue.h>
#include <renderer.h>
#include <rumbleThread.h>
#include <state.h>
#include <staticMem.h>
#include <utils.h>
#include <menu/utils.h>

static void cleanupCancelledInstallation(NUSDEV dev, const char *path, bool toUsb, bool keepFiles)
{
	debugPrintf("Cleaning up...");

	startNewFrame();
	textToFrame(0, 0, "Cancelling installation.");
	textToFrame(1, 0, "Please wait...");
	writeScreenLog(2);
	drawFrame();
	showFrame();

	switch(dev)
	{
		case NUSDEV_USB:
		case NUSDEV_MLC:
			keepFiles = false;
		default:
			break;
	}

	if(!keepFiles)
		removeDirectory(path);

	char *importPath = getStaticPathBuffer(2);
	strcpy(importPath, toUsb ? "usb:/usr/import/" : "mlc:/usr/import/");
	DIR *dir = opendir(importPath);

	if(dir != NULL)
	{
		char *ptr = importPath + 16;

		for(struct dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir))
		{
			if(entry->d_name[0] == '.' || !(entry->d_type & DT_DIR) || strlen(entry->d_name) != 8)
				continue;

			strcpy(ptr, entry->d_name);
			debugPrintf("Removing directory %s", importPath);
			removeDirectory(importPath);
		}

		closedir(dir);
	}
}

bool install(const char *game, bool hasDeps, NUSDEV dev, const char *path, bool toUsb, bool keepFiles)
{
	startNewFrame();
	char *toScreen = getToFrameBuffer();
	strcpy(toScreen, "Installing ");
	strcat(toScreen, game);
	textToFrame(0, 0, toScreen);
	barToFrame(1, 0, 40, 0);
	textToFrame(1, 41, "Preparing. This might take some time. Please be patient.");
	writeScreenLog(2);
	drawFrame();
	showFrame();
	
	char *newPath = MEMAllocFromDefaultHeapEx(0x27F, 0x40); // MCP mounts the card at another point, so we have to adjust the path -  Size and alignmnt is important!
	if(newPath == NULL)
		return false;

	MCPInstallTitleInfo *info = MEMAllocFromDefaultHeapEx(sizeof(MCPInstallTitleInfo), 0x40);
	if(info == NULL)
	{
		MEMFreeToDefaultHeap(newPath);
		return false;
	}

	switch(dev)
	{
		case NUSDEV_USB:
			strcpy(newPath, isUSB01() ? "/vol/storage_usb01" : "/vol/storage_usb02");
			strcpy(newPath + 18, path + 4);
			break;
		case NUSDEV_SD:
			strcpy(newPath, "/vol/app_sd");
			strcpy(newPath + 11, path + 18);
			break;
		case NUSDEV_MLC:
			strcpy(newPath, "/vol/storage_mlc01");
			strcpy(newPath + 18, path + 4);
	}
	
	McpData data;
	flushIOQueue(); // Make sure all game files are on disc

	// Let's see if MCP is able to parse the TMD...
	OSTime t = OSGetSystemTime();
	data.err = MCP_InstallGetInfo(mcpHandle, newPath, (MCPInstallInfo *)info);
    t = OSGetSystemTime() - t;
	addEntropy(&t, sizeof(OSTime));
	if(data.err != 0)
	{
		switch(data.err)
		{
			case 0xfffbf3e2:
				sprintf(toScreen, "No title.tmd found at \"%s\"", newPath);
				break;
			case 0xfffbfc17:
				sprintf(toScreen,
						"Internal error installing \"%s\""
#ifdef NUSSPLI_HBL
						"We're supporting HBL on Tiramisu and HBLC v2.1 fix by Gary only!"
#endif
						, newPath
				);
				break;
			default:
				sprintf(toScreen, "Error getting info for \"%s\" from MCP: %#010x", newPath, data.err);
		}
		
		debugPrintf(toScreen);
		addToScreenLog("Installation failed!");
		drawErrorFrame(toScreen, ANY_RETURN);
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(toScreen, ANY_RETURN);
			
			showFrame();
			
			if(vpad.trigger)
				break;
		}
		goto installError;
	}
	
	// Allright, let's set if we want to install to USB or NAND
	MCPInstallTarget target = toUsb ? MCP_INSTALL_TARGET_USB : MCP_INSTALL_TARGET_MLC;

	data.err = MCP_InstallSetTargetDevice(mcpHandle, target);
	if(data.err == 0)
	{
		if(toUsb && !isUSB01())
			++target;

		data.err = MCP_InstallSetTargetUsb(mcpHandle, target);
	}

	if(data.err != 0)
	{
		char *err = toUsb ? "Error opening USB device" : "Error opening internal memory";
		debugPrintf(err);
		addToScreenLog("Installation failed!");
		drawErrorFrame(err, ANY_RETURN);
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(err, ANY_RETURN);
			
			showFrame();
			
			if(vpad.trigger)
				break;
		}
		goto installError;
	}
	
	// Just some debugging stuff
	debugPrintf("NUSspli path:  %s (%d)", path, strlen(path));
	debugPrintf("MCP Path:      %s (%d)", newPath, strlen(newPath));
	
	// Last preparing step...
	disableShutdown();
	glueMcpData(info, &data);
	
	// Start the installation process
	t = OSGetSystemTime();
	MCPError err = MCP_InstallTitleAsync(mcpHandle, newPath, info);
	
	if(err != 0)
	{
		sprintf(toScreen, "Error starting async installation of \"%s\": %#010x", newPath, data.err);
		debugPrintf(toScreen);
		addToScreenLog("Installation failed!");
		drawErrorFrame(toScreen, ANY_RETURN);
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(toScreen, ANY_RETURN);
			
			showFrame();
			
			if(vpad.trigger)
				break;
		}
		enableShutdown();
		goto installError;
	}
	
	showMcpProgress(&data, game, true);
    t = OSGetSystemTime() - t;
	addEntropy(&t, sizeof(OSTime));
	// Quarkys ASAN catched this / seems like MCP already frees it for us
//	MEMFreeToDefaultHeap(progress);
	MEMFreeToDefaultHeap(newPath);
	MEMFreeToDefaultHeap(info);
	
	// MCP thread finished. Let's see if we got any error - TODO: This is a 1:1 copy&paste from WUP Installer GX2 which itself stole it from WUP Installer Y mod which got it from WUP Installer minor edit by Nexocube who got it from WUP installer JHBL Version by Dimrok who portet it from the ASM of WUP Installer. So I think it's time for something new... ^^
	if(data.err != 0)
	{
		debugPrintf("Installation failed with result: %#010x", data.err);
		strcpy(toScreen, "Installation failed!\n\n");
		switch(data.err)
		{
			case CUSTOM_MCP_ERROR_CANCELLED:
				cleanupCancelledInstallation(dev, path, toUsb, keepFiles);
			case CUSTOM_MCP_ERROR_EOM:
				enableShutdown();
				return true;
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
				strcat(toScreen, "Not enough free space on target device");
				break;
			case 0xFFFFF825:
			case 0xFFFFF82E:
				strcat(toScreen, "Files might be corrupt or bad storage medium.\nTry redownloading files or reformat/replace target device");
				break;
			default:
				if ((data.err & 0xFFFF0000) == 0xFFFB0000)
				{
					if(dev == NUSDEV_USB)
						strcat(toScreen, "Possible USB failure. Check your drives power source.\nAlso f");
					else
						strcat(toScreen, "F");

					strcat(toScreen, "iles might be corrupt");
				}
				else
					sprintf(toScreen + 12, "Unknown Error: %#010x", data.err);
		}
		
		addToScreenLog("Installation failed!");
		drawErrorFrame(toScreen, ANY_RETURN);
		
		while(AppRunning())
		{
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(toScreen, ANY_RETURN);
			
			showFrame();
			
			if(vpad.trigger)
				break;
		}
		enableShutdown();
		return false;
	}

	addToScreenLog("Installation finished!");
	
	if(!keepFiles && dev == NUSDEV_SD)
	{
		debugPrintf("Removing installation files...");
		removeDirectory(path);
	}
	
	colorStartNewFrame(SCREEN_COLOR_D_GREEN);
	textToFrame(0, 0, game);
	textToFrame(1, 0, "Installed successfully!");
	writeScreenLog(2);
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
			writeScreenLog(2);
			drawFrame();
		}
		
		showFrame();
		
		if(vpad.trigger)
			break;
	}
	return true;

installError:
	MEMFreeToDefaultHeap(newPath);
	MEMFreeToDefaultHeap(info);
	return false;
}
