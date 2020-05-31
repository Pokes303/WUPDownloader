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

#include <config.h>
#include <input.h>
#include <renderer.h>
#include <status.h>
#include <main.h>
#include <menu/download.h>
#include <menu/main.h>

#include <string.h>

void drawConfigMenu()
{
	startNewFrame();
	textToFrame(0, 0, "That Title Key Site:");
	textToFrame(0, 1, getTitleKeySite());
	textToFrame(0, 4, "Press \uE000 to change");
	textToFrame(0, 5, "Press \uE001 to go back");
	drawFrame();
}

void configMenu()
{
	drawConfigMenu();
	
	while(AppRunning())
	{
		if(app == 2)
			continue;
		
		showFrame();
		
		switch(vpad.trigger)
		{
			case VPAD_BUTTON_A:
				;
				char newUrl[1024];
				if(showKeyboard(KEYBOARD_TYPE_NORMAL, newUrl, CHECK_URL, 1024, false, getTitleKeySite(), "SAVE"))
					setTitleKeySite(newUrl);
				drawConfigMenu();
				break;
			case VPAD_BUTTON_B:
				saveConfig();
				return;
		}
	}
}
