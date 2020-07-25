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

#include <input.h>
#include <installer.h>
#include <renderer.h>
#include <status.h>
#include <ticket.h>
#include <menu/config.h>
#include <menu/download.h>
#include <menu/filebrowser.h>
#include <menu/installer.h>
#include <menu/main.h>
#include <menu/utils.h>

#include <coreinit/memdefaultheap.h>

#include <string.h>

void drawMainMenuFrame()
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
	
	textToFrame(0, 13, "Press \uE000 to download content");
	textToFrame(0, 14, "Press \uE002 to install content");
	textToFrame(0, 15, "Press \uE003 to generate a fake <title.tik> file");
	textToFrame(0, 16, "Press \uE041 LEFT for options");
	textToFrame(0, 17, "Press \uE044 or \uE001 to exit");
	
	textToFrame(MAX_CHARS - 24, 9, "Thanks to:      ");
	textToFrame(MAX_CHARS - 23, 10, "• cJSON        ");
	textToFrame(MAX_CHARS - 23, 11, "• E1ite007     ");
	textToFrame(MAX_CHARS - 23, 12, "• libgui       ");
	textToFrame(MAX_CHARS - 23, 13, "• Pokes303     ");
	textToFrame(MAX_CHARS - 23, 14, "• Quarky       ");
	textToFrame(MAX_CHARS - 23, 15, "• Simone Z.    ");
	textToFrame(MAX_CHARS - 23, 16, "• WUP installer");
	
	textToFrame(MAX_CHARS - 24, 18, "Beta testers:   ");
	textToFrame(MAX_CHARS - 23, 19, "• Anonym       ");
	textToFrame(MAX_CHARS - 23, 20, "• huma_dawii   ");
	textToFrame(MAX_CHARS - 23, 21, "• jacobsson    ");
	textToFrame(MAX_CHARS - 23, 22, "• pirate       ");
	
	textToFrame(0, MAX_LINES - 3, "WARNING:");
	textToFrame(1, MAX_LINES - 2, "• Don't eject the SD Card or the application will crash!");
	textToFrame(1, MAX_LINES - 1, "• You are unable to exit while installing a game");
	drawFrame();
}

void mainMenu()
{
	drawMainMenuFrame();
	
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawMainMenuFrame();
		
		showFrame();
		
		switch(vpad.trigger)
		{
			case VPAD_BUTTON_A:
				downloadMenu();
				drawMainMenuFrame();
				break;
			case VPAD_BUTTON_X:
				;
				char *dir = fileBrowserMenu();
				if(dir != NULL)
				{
					installerMenu(dir);
					MEMFreeToDefaultHeap(dir);
				}
				drawMainMenuFrame();
				break;
			case  VPAD_BUTTON_LEFT:
				configMenu();
				drawMainMenuFrame();
				break;
			case VPAD_BUTTON_Y:
				generateFakeTicket();
				drawMainMenuFrame();
				break;
			case VPAD_BUTTON_B:
				return;
		}
	}
}
