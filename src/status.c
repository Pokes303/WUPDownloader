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

#include <main.h>
#include <renderer.h>
#include <status.h>
#include <utils.h>

#include <coreinit/core.h>
#include <proc_ui/procui.h>
#include <whb/proc.h>

#include <stdbool.h>

int app = 1;
bool appRunning = true;


void exitApp()
{
	if(hbl)
		WHBProcStopRunning();
	else
		ProcUIShutdown();
}


bool AppRunning()
{
	if(appRunning)
	{
		if(hbl)
			appRunning = WHBProcIsRunning();
		else
		{
			switch(ProcUIProcessMessages(true))
			{
				case PROCUI_STATUS_EXITING:
					// Being closed, deinit, free, and prepare to exit
					app = 0;
					appRunning = false;
					break;
				case PROCUI_STATUS_RELEASE_FOREGROUND:
					// Free up MEM1 to next foreground app, deinit screen, etc.
					if(app == 1 || app == 9)
					{
						shutdownRenderer();
						debugPrintf("TO BACKGROUND!");
						shutdownDebug();
					}
					
					app = 2;
					ProcUIDrawDoneRelease();
					break;
				case PROCUI_STATUS_IN_FOREGROUND:
					// Executed while app is in foreground
					if(app == 2)
					{
						debugInit();
						debugPrintf("TO FOREGROUND!");
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
	}
	
	return appRunning;
}