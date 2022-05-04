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

#include <input.h>
#include <renderer.h>
#include <state.h>
#include <utils.h>
#include <updater.h>
#include <menu/utils.h>

#include <string.h>

static void drawUpdateMenuFrame(char *newVersion)
{
	startNewFrame();
	boxToFrame(0, 5);
	textToFrame(1, ALIGNED_CENTER, "NUSspli");
	char toScreen[256];
	strcpy(toScreen, "NUS simple packet loader/installer [");
	strcat(toScreen, NUSSPLI_VERSION);
	strcat(toScreen, "]");
	textToFrame(3, ALIGNED_CENTER, toScreen);
	
	textToFrame(4, ALIGNED_CENTER, NUSSPLI_COPYRIGHT);
	
	textToFrame(7, 0, "Update available!");
	lineToFrame(MAX_LINES - 3, SCREEN_COLOR_WHITE);
	strcpy(toScreen, "Press " BUTTON_A " to update to ");
	strcat(toScreen, newVersion);
	textToFrame(MAX_LINES - 2, 0, toScreen);
	textToFrame(MAX_LINES - 1, 0, "Press " BUTTON_B " to cancel");
	drawFrame();
}

bool updateMenu(char *newVersion)
{
	drawUpdateMenuFrame(newVersion);
	
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawUpdateMenuFrame(newVersion);
		
		showFrame();
		
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			update(newVersion);
			return true;
		}
		if(vpad.trigger & VPAD_BUTTON_B)
			break;
	}
	
	return false;
}
