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

#include <config.h>
#include <downloader.h>
#include <file.h>
#include <input.h>
#include <installer.h>
#include <ioThread.h>
#include <memdebug.h>
#include <osdefs.h>
#include <otp.h>
#include <renderer.h>
#include <romfs-wiiu.h>
#include <rumbleThread.h>
#include <status.h>
#include <ticket.h>
#include <titles.h>
#include <updater.h>
#include <usb.h>
#include <utils.h>
#include <cJSON.h>
#include <menu/download.h>
#include <menu/main.h>
#include <menu/utils.h>

#include <limits.h>
#include <stdlib.h>
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
#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <whb/crash.h>

int main()
{
	initStatus();
	initASAN();
	
	OSThread *mainThread = OSGetCurrentThread();
	OSSetThreadName(mainThread, "NUSspli");
	
	debugInit();
	debugPrintf("NUSspli " NUSSPLI_VERSION);
	checkStacks("main");
	
	getCommonKey(); // We do this exploit as soon as possible
	
	FSInit();
#ifdef NUSSPLI_HBL
	romfsInit();
#endif
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
	
	srand(OSGetTick());
	
	addToScreenLog("RNG seeded!");
	startNewFrame();
	textToFrame(0, 0, "Initializing rumble...");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	char *lerr = NULL;
	if(initRumble())
	{
		addToScreenLog("Rumble initialized!");
		
		startNewFrame();
		textToFrame(0, 0, "Initializing network...");
		writeScreenLog();
		drawFrame();
		showFrame();
		if(NNResult_IsSuccess(ACInitialize()))
		{
			ACConfigId networkID;
			if(NNResult_IsSuccess(ACGetStartupId(&networkID)))
			{
				ACConnectWithConfigId(networkID);
				addToScreenLog("Network initialized!");
				
				startNewFrame();
				textToFrame(0, 0, "Loading downloader...");
				writeScreenLog();
				drawFrame();
				showFrame();
				
				if(initDownloader())
				{
					addToScreenLog("Downloader initialized!");
					
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
					
					if(initConfig())
					{
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
									
									KPADInit();
									WPADEnableURCC(true);
									
									checkStacks("main()");
									
									if(!updateCheck())
									{
										initTitles();
										
										checkStacks("main");
										
										mainMenu(); // main loop
										
										debugPrintf("Deinitializing libraries...");
										clearTitles();
										saveConfig();
										
										checkStacks("main");
									}
									
									KPADShutdown();
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
					}
					else
						lerr = "Couldn't load config file!";
					
					deinitDownloader();
				}
				else
					lerr = "Couldn't initialize downloader!";
			}
			else
				lerr = "Couldn't get default network connection!";
			
			ACFinalize();
			debugPrintf("Network closed");
		}
		else
			lerr = "Couldn't inititalize network!";
		
		deinitRumble();
		debugPrintf("Rumble closed");
	}
	else
		lerr = "Couldn't initialize rumble!";
	
	if(lerr != NULL)
	{
		drawErrorFrame(lerr, B_RETURN);
			
		while(!(vpad.trigger & VPAD_BUTTON_B))
			showFrame();
	}
	
	shutdownRenderer();
	clearScreenLog();
#ifdef NUSSPLI_HBL
	romfsExit();
#endif
	FSShutdown();
	debugPrintf("libgui closed");
	
#ifdef NUSSPLI_DEBUG
	checkStacks("main");
	debugPrintf("Bye!");
	shutdownDebug();
#endif

#ifdef NUSSPLI_HBL
	SYSRelaunchTitle(0, NULL);
#else
	SYSLaunchMenu();
#endif
	
	do
		AppRunning();
	while(app != APP_STATE_STOPPED)
	
	deinitASAN();
	ProcUIShutdown();
	return 0;
}
