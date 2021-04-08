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

#include <ioThread.h>
#include <renderer.h>
#include <status.h>
#include <usb.h>
#include <utils.h>

#include <coreinit/core.h>
#include <coreinit/dynload.h>
#include <coreinit/energysaver.h>
#include <coreinit/foreground.h>
#include <coreinit/systeminfo.h>
#include <coreinit/title.h>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>

#include <stdbool.h>

volatile APP_STATE app = APP_STATE_RUNNING;
volatile bool shutdownEnabled = true;
volatile bool shutdownRequested = false;
uint32_t standalone = 0xABCD;
bool aroma;

void enableShutdown()
{
	IMEnableAPD();
	shutdownEnabled = true;
}
void disableShutdown()
{
	IMDisableAPD();
	shutdownEnabled = false;
}

bool isAroma()
{
	return aroma;
}

bool isStandalone()
{
	if(aroma)
		return true;
	
	if(standalone == 0xABCD)
		standalone = OSGetTitleID() == 0x000500004E555373;
	
	return standalone;
}

uint32_t homeButtonCallback(void *dummy)
{
	if(shutdownEnabled)
		shutdownRequested = true;
	
	return 0;
}

void initStatus()
{
	ProcUIInit(&OSSavesDone_ReadyToRelease);
	ProcUIRegisterCallback(PROCUI_CALLBACK_HOME_BUTTON_DENIED, &homeButtonCallback, NULL, 100);
	OSEnableHomeButtonMenu(false);
	
	OSDynLoad_Module mod;
	aroma = OSDynLoad_Acquire("homebrew_kernel", &mod) == OS_DYNLOAD_OK;
	if(aroma)
		OSDynLoad_Release(mod);
}

bool AppRunning()
{
	if(OSIsMainCore())
	{
		if(shutdownRequested)
		{
			flushIOQueue();
			unmountUSB();
			if(isStandalone())
				SYSLaunchMenu();
			else
				SYSRelaunchTitle(0, NULL);
			
			shutdownRequested = false;
		}
		
		if(app)
		{
			switch(ProcUIProcessMessages(true))
			{
				case PROCUI_STATUS_EXITING:
					// Being closed, deinit, free, and prepare to exit
					app = APP_STATE_STOPPED;
					break;
				case PROCUI_STATUS_RELEASE_FOREGROUND:
					// Free up MEM1 to next foreground app, deinit screen, etc.
					if(app == APP_STATE_RUNNING || app == APP_STATE_RETURNING)
						shutdownRenderer();
					
					app = APP_STATE_BACKGROUND;
					ProcUIDrawDoneRelease();
					break;
				case PROCUI_STATUS_IN_FOREGROUND:
					// Executed while app is in foreground
					if(app == APP_STATE_BACKGROUND)
					{
						initRenderer();
						app = APP_STATE_RETURNING;
					}
					else
						app = APP_STATE_RUNNING;
					
					break;
				case PROCUI_STATUS_IN_BACKGROUND:
					app = APP_STATE_BACKGROUND;
					break;
			}
		}
	}
	
	return app;
}