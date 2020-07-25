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

#include <input.h>
#include <renderer.h>
#include <status.h>
#include <updater.h>
#include <menu/utils.h>

#include <string.h>

void drawUpdateMenuFrame(char *newVersion)
{
	startNewFrame();
	boxToFrame(0, 5);
	textToFrame(ALIGNED_CENTER, 1, "NUSspli");
	char toScreen[256];
	strcpy(toScreen, "NUS simple packet loader/installer [");
	strcat(toScreen, NUSSPLI_VERSION);
	strcat(toScreen, "]");
	textToFrame(ALIGNED_CENTER, 3, toScreen);
	
	textToFrame(ALIGNED_CENTER, 4, "© 2020 V10lator <v10lator@myway.de>");
	
	textToFrame(0, 7, "Update available!");
	lineToFrame(MAX_LINES - 3, SCREEN_COLOR_WHITE);
	strcpy(toScreen, "Press \uE000 to update to ");
	strcat(toScreen, newVersion);
	textToFrame(0, MAX_LINES - 2, toScreen);
	textToFrame(0, MAX_LINES - 1, "Press \uE001 to cancel");
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
		
		switch(vpad.trigger)
		{
			case VPAD_BUTTON_A:
				update(newVersion);
				return true;
			case VPAD_BUTTON_B:
				return false;
		}
	}
	
	// 0xDEADC0DE
	return false;
}
