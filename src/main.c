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

#include <config.h>
#include <file.h>
#include <input.h>
#include <installer.h>
#include <ioThread.h>
#include <memdebug.h>
#include <otp.h>
#include <renderer.h>
#include <status.h>
#include <ticket.h>
#include <usb.h>
#include <utils.h>
#include <cJSON.h>
#include <menu/download.h>
#include <menu/main.h>
#include <menu/utils.h>

#include <limits.h>
#include <string.h>
#include <sys/stat.h>

#include <coreinit/filesystem.h>
#include <coreinit/foreground.h>
#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/title.h>
#include <curl/curl.h>
#include <nn/ac/ac_c.h>
#include <nn/result.h>
#include <proc_ui/procui.h>
#include <whb/crash.h>

int main()
{
	initStatus();
	
	OSThread *mainThread = OSGetCurrentThread();
	OSSetThreadName(mainThread, "NUSspli");
	
	debugInit();
	debugPrintf("main()");
#ifdef NUSSPLI_DEBUG
	WHBInitCrashHandler();
	OSCheckActiveThreads();
#endif
	
	getCommonKey(); // We do this exploit as soon as possible
	
	initRenderer();
	
	// ASAN Trigger
/*	char *asanHeapTrigger = MEMAllocFromDefaultHeap(1);
	debugPrintf("ASAN Debug: Triggering buffer-read overflow:");
	if(asanHeapTrigger[1] == ' ') ;
	debugPrintf("ASAN Debug: Triggering buffer-write overflow:");
	asanHeapTrigger[1] = '?';
	debugPrintf("ASAN Debug: Triggering double-free:");
	MEMFreeToDefaultHeap(asanHeapTrigger);
	MEMFreeToDefaultHeap(asanHeapTrigger);*/
	
	if(OSSetThreadPriority(mainThread, 1))
		addToScreenLog("Changed main thread priority!");
	else
		addToScreenLog("WARNING: Error changing main thread priority!");
	startNewFrame();
	textToFrame(0, 0, "Seeding RNG...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	initRandom();
	
	addToScreenLog("RNG seeded!");
	startNewFrame();
	textToFrame(0, 0, "Initializing network...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	char *lerr = NULL;
	
	FSInit(); // We need to start this before the SWKBD.
	if(NNResult_IsSuccess(ACInitialize()))
	{
		ACConfigId networkID;
		if(NNResult_IsSuccess(ACGetStartupId(&networkID)))
		{
			ACConnectWithConfigId(networkID);
			addToScreenLog("Network initialized!");
			
			startNewFrame();
			textToFrame(0, 0, "Loading cJSON...");
			writeScreenLog();
			drawFrame();
			showFrame();
			
			cJSON_Hooks ch;
			ch.malloc_fn = MEMAllocFromDefaultHeap;
			ch.free_fn = MEMFreeToDefaultHeap;
			cJSON_InitHooks(&ch);
			
			addToScreenLog("cJSON initialized!");
			startNewFrame();
			textToFrame(0, 0, "Loading SWKBD...");
			writeScreenLog();
			drawFrame();
			showFrame();
			
			FSInit();
			if(SWKBD_Init())
			{
				addToScreenLog("SWKBD initialized!");
				startNewFrame();
				textToFrame(0, 0, "Loading MCP...");
				writeScreenLog();
				drawFrame();
				showFrame();
				
				mcpHandle = MCP_Open();
				if(mcpHandle != 0)
				{
					addToScreenLog("MCP initialized!");
					startNewFrame();
					textToFrame(0, 0, "Loading I/O thread...");
					writeScreenLog();
					drawFrame();
					showFrame();
					
					if(initIOThread())
					{
						addToScreenLog("I/O thread initialized!");
						startNewFrame();
						textToFrame(0, 0, "Loading config file...");
						writeScreenLog();
							drawFrame();
						showFrame();
						
						#ifdef NUSSPLI_DEBUG
						debugPrintf("Checking thread stacks...");
						OSCheckActiveThreads();
						#endif
					
						if(initConfig())
						{
							if(downloadJSON())
								// Main loop
								mainMenu();
							
							debugPrintf("Deinitializing libraries...");
							saveConfig();
							
							#ifdef NUSSPLI_DEBUG
							debugPrintf("Checking thread stacks...");
							OSCheckActiveThreads();
							#endif
						}
						else
							lerr = "Couldn't load config file!";
						
						shutdownIOThread();
						debugPrintf("I/O thread closed");
					}
					else
						lerr = "Couldn't load I/O thread!";
					
					unmountUSB();
					MCP_Close(mcpHandle);
					debugPrintf("MCP closed");
				}
				else
					lerr = "Couldn't initialize MCP!";
				
				SWKBD_Shutdown();
				debugPrintf("SWKBD closed");
			}
			else
				lerr = "Couldn't initialize SWKBD!";
			
			FSShutdown();
			freeJSON();
		}
		else
			lerr = "Couldn't get default network connection!";
		
		ACFinalize();
		debugPrintf("Network closed");
	}
	else
		lerr = "Couldn't inititalize network!";
	
	if(lerr != NULL)
	{
		drawErrorFrame(lerr, B_RETURN);
			
		while(vpad.trigger != VPAD_BUTTON_B)
			showFrame();
	} 
	
	shutdownRenderer();
	clearScreenLog();
	debugPrintf("libgui closed");
	
#ifdef NUSSPLI_DEBUG
	debugPrintf("Checking thread stacks...");
	OSCheckActiveThreads();
	debugPrintf("Bye!");
	shutdownDebug();
#endif
	
	deinitASAN();
	
	if(AppRunning())
	{
		homeButtonCallback(NULL);
		while(AppRunning())
			;
	}
	
	ProcUIShutdown();
	
	return 0;
}

void __preinit_user(MEMHeapHandle *mem1, MEMHeapHandle *fg, MEMHeapHandle *mem2)
{
	debugInit();
	debugPrintf("__preinit_user()");
	initASAN(*mem2);
	shutdownDebug();
}
