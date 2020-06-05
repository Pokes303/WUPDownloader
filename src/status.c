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

#include <renderer.h>
#include <status.h>
#include <usb.h>
#include <utils.h>

#include <coreinit/core.h>
#include <coreinit/energysaver.h>
#include <coreinit/foreground.h>
#include <coreinit/systeminfo.h>
#include <coreinit/title.h>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>

#include <stdbool.h>

int app = 1;
bool appRunning = true;
bool shutdownEnabled = true;

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

static uint32_t homeButtonCallback(void *dummy)
{
	if(shutdownEnabled)
	{
		unmountUSB();
		if(OSGetTitleID() == 0x000500004E555373)
			SYSLaunchMenu();
		else
			SYSRelaunchTitle(0, NULL);
	}
	
	return 0;
}

void initStatus()
{
	ProcUIInit(&OSSavesDone_ReadyToRelease);
	ProcUIRegisterCallback(PROCUI_CALLBACK_HOME_BUTTON_DENIED, &homeButtonCallback, NULL, 100);
	OSEnableHomeButtonMenu(false);
}

bool AppRunning()
{
	if(appRunning)
	{
		switch(ProcUIProcessMessages(true))
		{
			case PROCUI_STATUS_EXITING:
				// Being closed, deinit, free, and prepare to exit
				app = 0;
				appRunning = false;
				debugPrintf("Exiting!");
				break;
			case PROCUI_STATUS_RELEASE_FOREGROUND:
				// Free up MEM1 to next foreground app, deinit screen, etc.
				if(app == 1 || app == 9)
					shutdownRenderer();
				
				app = 2;
				ProcUIDrawDoneRelease();
				break;
			case PROCUI_STATUS_IN_FOREGROUND:
				// Executed while app is in foreground
				if(app == 2)
				{
					initRenderer();
					app = 9;
				}
				else
					app = 1;
				
				break;
			case PROCUI_STATUS_IN_BACKGROUND:
				app = 2;
				break;
		}
	}
	
	return appRunning;
}