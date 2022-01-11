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
#include <crypto.h>
#include <downloader.h>
#include <file.h>
#include <filesystem.h>
#include <hbl.h>
#include <input.h>
#include <installer.h>
#include <ioThread.h>
#include <memdebug.h>
#include <osdefs.h>
#include <otp.h>
#include <renderer.h>
#include <romfs-wiiu.h>
#include <rumbleThread.h>
#include <sanity.h>
#include <status.h>
#include <thread.h>
#include <ticket.h>
#include <titles.h>
#include <updater.h>
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
#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <whb/crash.h>

int main()
{
	initStatus();
	initASAN();

#ifdef NUSSPLI_HBL
	uint64_t tid = OSGetTitleID();
	bool breakingout = isTiramisu() && (tid & 0xFFFFFFFFFFFFF0FF) == 0x000500101004A000; // Mii Maker
	if(breakingout)
		breakingout = breakOut();

	if(!breakingout)
	{
#endif
		OSThread *mainThread = OSGetCurrentThread();
		OSSetThreadName(mainThread, "NUSspli");
		addEntropy(&(mainThread->id), sizeof(uint16_t));
		addEntropy(mainThread->stackStart, 4);

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

		if(OSSetThreadPriority(mainThread, THREAD_PRIORITY_HIGH))
			addToScreenLog("Changed main thread priority!");
		else
			addToScreenLog("WARNING: Error changing main thread priority!");

		startNewFrame();
		textToFrame(0, 0, "Loading OpenSSL...");
		writeScreenLog();
		drawFrame();

		char *lerr = NULL;
		if(initCrypto())
		{
			addToScreenLog("OpenSSL initialized!");

			startNewFrame();
			textToFrame(0, 0, "Loading MCP...");
			writeScreenLog();
			drawFrame();

			mcpHandle = MCP_Open();
			if(mcpHandle != 0)
			{
				addToScreenLog("MCP initialized!");

				startNewFrame();
				textToFrame(0, 0, "Checking sanity...");
				writeScreenLog();
				drawFrame();

				if(sanityCheck())
				{
					addToScreenLog("Sanity checked!");

					startNewFrame();
					textToFrame(0, 0, "Initializing rumble...");
					writeScreenLog();
					drawFrame();

					if(initRumble())
					{
						addToScreenLog("Rumble initialized!");

						startNewFrame();
						textToFrame(0, 0, "Loading downloader...");
						writeScreenLog();
						drawFrame();

						if(initDownloader())
						{
							addToScreenLog("Downloader initialized!");

							startNewFrame();
							textToFrame(0, 0, "Loading cJSON...");
							writeScreenLog();
							drawFrame();

							cJSON_Hooks ch;
							ch.malloc_fn = MEMAllocFromDefaultHeap;
							ch.free_fn = MEMFreeToDefaultHeap;
							cJSON_InitHooks(&ch);

							addToScreenLog("cJSON initialized!");
							startNewFrame();
							textToFrame(0, 0, "Loading SWKBD...");
							writeScreenLog();
							drawFrame();

							if(initConfig())
							{
								addToScreenLog("Config loaded!");
								startNewFrame();
								textToFrame(0, 0, "Loading SWKBD...");
								writeScreenLog();
								drawFrame();

								if(SWKBD_Init())
								{
									addToScreenLog("SWKBD initialized!");
									startNewFrame();
									textToFrame(0, 0, "Loading I/O thread...");
									writeScreenLog();
									drawFrame();

									if(initIOThread())
									{
										addToScreenLog("I/O thread initialized!");
										startNewFrame();
										textToFrame(0, 0, "Loading config file...");
										writeScreenLog();
										drawFrame();

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
											saveConfig(true);

											checkStacks("main");
										}

										KPADShutdown();
										shutdownIOThread();
										debugPrintf("I/O thread closed");
									}
									else
										lerr = "Couldn't load I/O thread!";

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

						deinitRumble();
						debugPrintf("Rumble closed");
					}
					else
						lerr = "Couldn't initialize rumble!";
				}
				else
					lerr = "No support for rebrands, use original NUSspli!";

				unmountAll();
				MCP_Close(mcpHandle);
				debugPrintf("MCP closed");
			}
			else
				lerr = "Couldn't initialize MCP!";

			deinitCrypto();
			debugPrintf("OpenSSL closed");
		}
		else
			lerr = "Couldn't initialize OpenSSL!";

		if(lerr != NULL)
		{
			drawErrorFrame(lerr, ANY_RETURN);

			while(!(vpad.trigger))
				showFrame();
		}

		shutdownRenderer();
		debugPrintf("Clearing screen log");
		clearScreenLog();
#ifdef NUSSPLI_HBL
		romfsExit();
#endif
		debugPrintf("Shutting down filesystem");
		FSShutdown();
		debugPrintf("libgui-sdl closed");

#ifdef NUSSPLI_DEBUG
		checkStacks("main");
		debugPrintf("Bye!");
//		shutdownDebug();
#endif

#ifdef NUSSPLI_HBL
	}
#endif
	
	if(app != APP_STATE_STOPPED)
	{
#ifdef NUSSPLI_HBL
		if(breakingout)
		{
			tid &= 0xFFFFFFFFFFFF0FFF;
			tid |= 0x000000000000E000;
		}
		else if(isTiramisu() && (tid & 0xFFFFFFFFFFFFF0FF) == 0x000500101004E000) // Health & Safety
		{
			tid &= 0xFFFFFFFFFFFF0FFF;
			tid |= 0x000000000000A000;
		}

		_SYSLaunchTitleWithStdArgsInNoSplash(tid, 0);
#else
		SYSLaunchMenu();
#endif
		if(app == APP_STATE_HOME)
		{
			app = APP_STATE_RUNNING;
			while(AppRunning())
				;
		}
		
		if(app == APP_STATE_STOPPING)
			ProcUIDrawDoneRelease();
		
		int ps;
		do
		{
			ps = ProcUIProcessMessages(true);
			if(ps == PROCUI_STATUS_RELEASE_FOREGROUND)
				ProcUIDrawDoneRelease();
		}
		while(ps != PROCUI_STATUS_EXITING);
	}
	
	deinitASAN();
	ProcUIShutdown();
	return 0;
}
