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

#include <ticket.h>

#include <utils.h>
#include <file.h>
#include <input.h>
#include <ioThread.h>
#include <keygen.h>
#include <renderer.h>
#include <status.h>
#include <usb.h>
#include <utils.h>
#include <menu/filebrowser.h>
#include <menu/utils.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <coreinit/memdefaultheap.h>

/* This generates a ticket and writes it to a file.
 * The ticket algorithm is ported from FunkiiU combined
 * with the randomness of NUSPackager to create unique
 * NUSspli tickets.
 */
void generateTik(const char *path, const char *titleID)
{
	char *encKey = generateKey(titleID);
	if(encKey == NULL)
		return;
	
	FILE *tik = fopen(path, "wb");
	if(tik == NULL)
	{
		char err[1044];
		sprintf(err, "Could not open path\n%s", path);
		drawErrorFrame(err, B_RETURN);
		
		while(AppRunning())
		{
			showFrame();
			
			if(app == 2)
				continue;
			
			if(vpad.trigger == VPAD_BUTTON_B)
				return;
		}
		return;
	}
	
	debugPrintf("Generating fake ticket at %s", path);
	
	// NUSspli adds its own header.
	writeHeader(tik, FILE_TYPE_TIK);
	
	// SSH certificate
	writeCustomBytes(tik, "0x526F6F742D434130303030303030332D58533030303030303063"); // "Root-CA00000003-XS0000000c"writeVoidBytes(tik, 0x26);
	writeVoidBytes(tik, 0x26);
	writeRandomBytes(tik, 0x3B);
	
	// The title data
	writeCustomBytes(tik, "0x00010000");
	writeCustomBytes(tik, encKey);
	writeVoidBytes(tik, 0xD); // 0x00000000, so one of the magic things again
	writeCustomBytes(tik, titleID);
	
	writeVoidBytes(tik, 0x3D);
	writeCustomBytes(tik, "01"); // Not sure what shis is for
	writeVoidBytes(tik, 0x82);
	
	//And some footer. Also not sure what it means
	//Let's write it out in parts so we might be able to find patterns
	writeCustomBytes(tik, "0x00010014"); // Magic thing A
	writeCustomBytes(tik, "0x000000ac");
	writeCustomBytes(tik, "0x00000014"); // Might be connected to magic thing A
	writeCustomBytes(tik, "0x00010014"); // Magic thing A
	writeCustomBytes(tik, "0x0000000000000028");
	writeCustomBytes(tik, "0x00000001"); // Magic thing B ?
	writeCustomBytes(tik, "0x0000008400000084"); // Pattern
	writeCustomBytes(tik, "0x0003000000000000");
	writeCustomBytes(tik, "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
	writeVoidBytes(tik, 0x60);
	
	addToIOQueue(NULL, 0, 0, tik);
	MEMFreeToDefaultHeap(encKey);
}

void drawTicketFrame(const char *titleID)
{
	startNewFrame();
	textToFrame(0, 0, "Title ID:");
	textToFrame(3, 1, titleID[0] == '\0' ? "NOT SET" : titleID);
	
	textToFrame(0, 3, "You need to set the title ID to generate a fake ticket");
	
	int line = MAX_LINES - 3;
	textToFrame(0, line--, "Press \uE001 to return");
	if (titleID[0] != '\0')
		textToFrame(0, line--, "Press \uE000 to continue");
	lineToFrame(line, SCREEN_COLOR_WHITE);
	
	lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);
	textToFrame(0, MAX_LINES - 1, "Press \uE041 UP to set the title ID");
	drawFrame();
}

bool generateFakeTicket()
{
	char *dir = fileBrowserMenu();
	if(dir == NULL)
		return true;
	
	char titleID[17];
	titleID[0] = '\0';
	
	drawTicketFrame(titleID);
	
	while(AppRunning())
	{
		showFrame();
		
		if(app == 2)
			continue;
		
		switch(vpad.trigger)
		{
			case VPAD_BUTTON_A:
				if(titleID[0] != '\0')
				{
					startNewFrame();
					textToFrame(0, 0, "Generating fake ticket...");
					drawFrame();
					showFrame();
					
					char tikPath[1024];
					strcpy(tikPath, dir);
					strcat(tikPath, dir[0] == '.' ? titleID : "title");
					MEMFreeToDefaultHeap(dir);
					strcat(tikPath, ".tik");
					
					generateTik(tikPath, titleID);
					
					colorStartNewFrame(SCREEN_COLOR_D_GREEN);
					textToFrame(0, 0, "Fake ticket generated on:");
					textToFrame(0, 1, tikPath);
					textToFrame(0, 3, "Press \uE000 to return");
					drawFrame();
					
					while(AppRunning())
					{
						showFrame();
						
						if(app == 2)
							continue;
						
						if(vpad.trigger == VPAD_BUTTON_A)
							return true;
					}
					return false;
				}
				break;
			case VPAD_BUTTON_B:
				MEMFreeToDefaultHeap(dir);
				return true;
			case VPAD_BUTTON_UP:
				showKeyboard(KEYBOARD_TYPE_RESTRICTED, titleID, CHECK_HEXADECIMAL, 16, true, titleID, NULL);
				toLowercase(titleID);
				drawTicketFrame(titleID);
				break;
		}
	}
	MEMFreeToDefaultHeap(dir);
	return false;
}
