/***************************************************************************
 * This file is part of NUSspli.                                           *
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
#include <renderer.h>
#include <status.h>
#include <usb.h>
#include <utils.h>
#include <menu/utils.h>

// A struct to pass data between the install function and the callback
typedef struct
{
	bool processing;
	MCPError err;
} McpInstallationData;

uint8_t vibrationPattern[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// The callback function. It tells the install function that installation finished and passes the error code (if any)
static void mcpCallback(IOSError err, void *rawData)
{
	McpInstallationData *data = (McpInstallationData *)rawData;
    data->err = (MCPError)err;
    data->processing = false;
}

bool install(const char *game, bool hasDeps, bool fromUSB, const char *path, bool toUsb, bool keepFiles)
{
	startNewFrame();
	textToFrame(0, 0, "Installing");
	textToFrame(11, 0, game);
	textToFrame(0, 1, "[  0%] Preparing...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	unmountUSB();
	
	char newPath[0x27F]; // MCP mounts the card at another point, so we have to adjust the path -  The length of 0x27F is important!
	if(fromUSB)
	{
		//TODO
		strcpy(newPath, "/vol/storage_usb01");
		strcat(newPath, path + 4);
	}
	else
	{
		strcpy(newPath, "/vol/app_sd");
		strcat(newPath, path + 15);
	}
	
	uint32_t info[80]; // WUT doesn't define MCPInstallInfo, so we define it like WUP Installer does, just without the malloc() nonsense. TDOD: Does it really have to be that big?
	McpInstallationData data;
	
	// Let's see if MCP is able to parse the TMD...
	data.err = MCP_InstallGetInfo(mcpHandle, newPath, (MCPInstallInfo *)info);
	if(data.err != 0)
	{
		char toScreen[2048];
		enableShutdown(); //TODO
		
		switch(data.err)
		{
			case 0xfffbf3e2:
				sprintf(toScreen, "No title.tmd found at \"%s\"", newPath);
				break;
			case 0xfffbfc17:
				sprintf(toScreen, "Internal error installing \"%s\"\nPlease report this!", newPath);
				break;
			default:
				sprintf(toScreen, "Error getting info for \"%s\" from MCP: %#010x", newPath, data.err);
		}
		
		debugPrintf(toScreen);
		addToScreenLog("Installation failed!");
		drawErrorFrame(toScreen, B_RETURN);
		
		while(AppRunning())
		{
			showFrame();
			
			if(app == 2)
				continue;
			
			if(vpad.trigger == VPAD_BUTTON_B)
				return false;
		}
	}
	
	// Allright, let's set if we want to install to USB or NAND
	data.err = MCP_InstallSetTargetDevice(mcpHandle, toUsb ? MCP_INSTALL_TARGET_USB : MCP_INSTALL_TARGET_MLC);
	if(data.err != 0 || MCP_InstallSetTargetUsb(mcpHandle, toUsb) != 0)
	{
		debugPrintf(toUsb ? "Error opening USB device" : "Error opening internal memory");
		enableShutdown(); //TODO
		addToScreenLog("Installation failed!");
		drawErrorFrame(toUsb ? "Error opening USB device" : "Error opening internal memory", B_RETURN);
		
		while(AppRunning())
		{
			showFrame();
			
			if(app == 2)
				continue;
			
			if(vpad.trigger == VPAD_BUTTON_B)
				return false;
		}
		return false;
	}
	
	// This vector holds the path to the installation files
	IOSVec vec;
	vec.vaddr = (void *)newPath;
	vec.len = 0x27F;
	vec.paddr = NULL;
	
	// Just some debugging stuff
	debugPrintf("NUSspli path:  %s (%d)", path, strlen(path));
	debugPrintf("MCP Path:      %s (%d)", newPath, strlen(newPath));
	
	// Last prepairing step...
	MCPInstallProgress *progress = MEMAllocFromDefaultHeapEx(sizeof(MCPInstallProgress), 0x40);
	if(progress == NULL)
	{
		debugPrintf("Error allocating memory!");
		enableShutdown(); //TODO
		return false;
	}
	
	progress->inProgress = 0;
	data.processing = true;
	char multiplierName[3];
	int multiplier = 12 + strlen(game);
	char toScreen[multiplier > 256 ? multiplier : 256];
	multiplier = 0;
	MCPError err;
	OSTime lastSpeedCalc = 0;
	OSTime now;
	uint64_t lsp = 0;
	char speedBuf[32];
	
	// Start the installation process
	// err = MCP_InstallTitleAsync(mcpHandle, newPath, (MCPInstallTitleInfo *)info);
	// We don't know how to give a callback function to the above but that is needed for propper error handling, so let's do the IOCTL directly:
	err = (MCPError)IOS_IoctlvAsync(mcpHandle, 0x81, 1, 0, &vec, mcpCallback, &data);
	
	if(err != 0)
	{
		char toScreen[2048];
		sprintf(toScreen, "Error starting async installation of \"%s\": %#010x", newPath, data.err);
		debugPrintf(toScreen);
		enableShutdown(); //TODO
		addToScreenLog("Installation failed!");
		drawErrorFrame(toScreen, B_RETURN);
		
		while(AppRunning())
		{
			showFrame();
			
			if(app == 2)
				continue;
			
			if(vpad.trigger == VPAD_BUTTON_B)
				return false;
		}
		return false;
		return false;
	}
	
	// Wait for the MCP thread and show status
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
				strcpy(toScreen, "Installing ");
				strcat(toScreen, game);
				textToFrame(0, 0, toScreen);
				barToFrame(1, 0, 40, progress->sizeProgress * 100.0f / progress->sizeTotal);
				sprintf(toScreen, "%.2f / %.2f %s", ((float)progress->sizeProgress) / multiplier, ((float)progress->sizeTotal) / multiplier, multiplierName);
				textToFrame(41, 1, toScreen);
				
				if(progress->sizeProgress != 0)
				{
					now = OSGetSystemTime();
					if(OSTicksToMilliseconds(now - lastSpeedCalc) > 333)
					{
						getSpeedString(progress->sizeProgress - lsp, speedBuf);
						lsp = progress->sizeProgress;
						lastSpeedCalc = now;
					}
					textToFrame(ALIGNED_RIGHT, 1, speedBuf);
				}
				
				writeScreenLog();
				drawFrame();
			}
		}
		else
			debugPrintf("MCP_InstallGetProgress() returned %#010x", err);
		
		showFrame();
	}
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
				strcat(toScreen, "Possible bad SD card.  Reformat (32k blocks) or replace");
				break;
			default:
				if ((data.err & 0xFFFF0000) == 0xFFFB0000)
					strcat(toScreen, "Verify WUP files are correct & complete.\nDLC/E-shop require Sig Patch");
				else
					sprintf(toScreen + 12, "Unknown Error: %#010x", data.err);
		}
		
		enableShutdown(); //TODO
		addToScreenLog("Installation failed!");
		drawErrorFrame(toScreen, B_RETURN);
		
		while(AppRunning())
		{
			showFrame();
			
			if(app == 2)
			continue;
			
			if(vpad.trigger == VPAD_BUTTON_B)
				return false;
		}
	}
	else 
		addToScreenLog("Installation finished!");
	
	if(!fromUSB && !keepFiles)
		removeDirectory(path);
	
	for (int i = 0; i < 0x10; i++)
		VPADControlMotor(VPAD_CHAN_0, vibrationPattern, 0xF);
	
	enableShutdown(); //TODO
	
	colorStartNewFrame(SCREEN_COLOR_GREEN);
	textToFrame(0, 0, game);
	textToFrame(0, 1, "Installed successfully!");
	writeScreenLog();
	drawFrame();
	
	while(AppRunning())
	{
		showFrame();
		
		if(app == 2)
			continue;
		
		if(vpad.trigger == VPAD_BUTTON_A || vpad.trigger == VPAD_BUTTON_B)
			break;
	}
	return true;
}
