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

#include <stdbool.h>

volatile APP_STATE app = APP_STATE_RUNNING;
volatile bool shutdownEnabled = true;
#ifndef NUSSPLI_HBL
bool channel;
#endif
bool aroma;
bool apdEnabled;
bool apdDisabled = false;

void enableApd()
{
	if(!apdEnabled || !apdDisabled)
		return;
	
	if(IMEnableAPD() == 0)
	{
		apdDisabled = false;
		debugPrintf("APD enabled!");
	}
	else
		debugPrintf("Error enabling APD!");
}

void disableApd()
{
	if(!apdEnabled || apdDisabled)
		return;
	
	if(IMDisableAPD() == 0)
	{
		apdDisabled = true;
		debugPrintf("APD disabled!");
	}
	else
		debugPrintf("Error disabling APD!");
}

void enableShutdown()
{
	if(shutdownEnabled)
		return;
	
	enableApd();
	shutdownEnabled = true;
	debugPrintf("Home key enabled!");
}
void disableShutdown()
{
	if(!shutdownEnabled)
		return;
	
	disableApd();
	shutdownEnabled = false;
	debugPrintf("Home key disabled!");
}

bool isAroma()
{
	return aroma;
}

#ifndef NUSSPLI_HBL
bool isChannel()
{
	return channel;
}
#endif

uint32_t homeButtonCallback(void *dummy)
{
	if(shutdownEnabled)
	{
		shutdownEnabled = false;
		app = APP_STATE_STOPPING;
	}
	
	return 0;
}

void initStatus()
{
	ProcUIInit(&OSSavesDone_ReadyToRelease);
	
	debugInit();
	debugPrintf("NUSspli " NUSSPLI_VERSION);
	
	ProcUIRegisterCallback(PROCUI_CALLBACK_HOME_BUTTON_DENIED, &homeButtonCallback, NULL, 100);
	OSEnableHomeButtonMenu(false);
	
	OSDynLoad_Module mod;
	aroma = OSDynLoad_Acquire("homebrew_kernel", &mod) == OS_DYNLOAD_OK;
	if(aroma)
		OSDynLoad_Release(mod);
#ifndef NUSSPLI_HBL
	channel = OSGetTitleID() == 0x0005000010155373;
#endif
	
	if(IMIsAPDEnabledBySysSettings((uint32_t *)&apdEnabled) != 0)
		apdEnabled = false;
	
}

bool AppRunning()
{
	if(OSIsMainCore() && app != APP_STATE_STOPPED)
	{
		switch(ProcUIProcessMessages(true))
		{
			case PROCUI_STATUS_EXITING:
				// Being closed, deinit, free, and prepare to exit
				app = APP_STATE_STOPPED;
				break;
			case PROCUI_STATUS_RELEASE_FOREGROUND:
				// Free up MEM1 to next foreground app, deinit screen, etc.
				shutdownRenderer();
				ProcUIDrawDoneRelease();
				if(app != APP_STATE_STOPPING)
					app = APP_STATE_BACKGROUND;
				break;
			case PROCUI_STATUS_IN_FOREGROUND:
				// Executed while app is in foreground
				if(app == APP_STATE_STOPPING)
					break;
				if(app == APP_STATE_BACKGROUND)
				{
					initRenderer();
					app = APP_STATE_RETURNING;
				}
				else
					app = APP_STATE_RUNNING;
				
				break;
			case PROCUI_STATUS_IN_BACKGROUND:
				if(app != APP_STATE_STOPPING)
					app = APP_STATE_BACKGROUND;
				break;
		}
	}
	
	return app;
}