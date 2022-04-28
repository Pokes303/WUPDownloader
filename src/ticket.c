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

#include <ticket.h>

#include <utils.h>
#include <file.h>
#include <filesystem.h>
#include <input.h>
#include <ioThread.h>
#include <keygen.h>
#include <renderer.h>
#include <status.h>
#include <titles.h>
#include <tmd.h>
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
void generateTik(const char *path, const TitleEntry *titleEntry)
{
    char encKey[33];
	if(!generateKey(titleEntry, encKey))
		return;
	
	char tid[17];
	hex(titleEntry->tid, 16, tid);
	
	NUSFILE *tik = openFile(path, "wb");
	if(tik == NULL)
	{
		char err[1044];
		sprintf(err, "Could not open path\n%s", path);
		drawErrorFrame(err, ANY_RETURN);
		
		while(AppRunning())
		{
			showFrame();
			
			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(err, ANY_RETURN);
			
			if(vpad.trigger)
				break;
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
	writeCustomBytes(tik, tid);
	
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
}

void generateCert(const char *path)
{
	NUSFILE *cert = openFile(path, "wb");
	if(cert == NULL)
	{
		char err[1044];
		sprintf(err, "Could not open path\n%s", path);
		drawErrorFrame(err, ANY_RETURN);

		while(AppRunning())
		{
			showFrame();

			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame(err, ANY_RETURN);

			if(vpad.trigger)
				break;
		}
		return;
	}

	// NUSspli adds its own header.
	writeHeader(cert, FILE_TYPE_CERT);

	// Some SSH certificate
	writeCustomBytes(cert, "0x526F6F742D43413030303030303033"); // "Root-CA00000003"
	writeVoidBytes(cert, 0x34);
	writeCustomBytes(cert, "0x0143503030303030303062"); // "?CP0000000b"
	writeVoidBytes(cert, 0x36);
	writeRandomBytes(cert, 0x104);
	writeCustomBytes(cert, "0x00010001");
	writeVoidBytes(cert, 0x34);
	writeCustomBytes(cert, "0x00010003");
	writeRandomBytes(cert, 0x200);
	writeVoidBytes(cert, 0x3C);

	// Next certificate
	writeCustomBytes(cert, "0x526F6F74"); // "Root"
	writeVoidBytes(cert, 0x3F);
	writeCustomBytes(cert, "0x0143413030303030303033"); // "?CA00000003"
	writeVoidBytes(cert, 0x36);
	writeRandomBytes(cert, 0x104);
	writeCustomBytes(cert, "0x00010001");
	writeVoidBytes(cert, 0x34);
	writeCustomBytes(cert, "0x00010004");
	writeRandomBytes(cert, 0x100);
	writeVoidBytes(cert, 0x3C);

	// Last certificate
	writeCustomBytes(cert, "0x526F6F742D43413030303030303033"); // "Root-CA00000003"
	writeVoidBytes(cert, 0x34);
	writeCustomBytes(cert, "0x0158533030303030303063"); // "?XS0000000c"
	writeVoidBytes(cert, 0x36);
	writeRandomBytes(cert, 0x104);
	writeCustomBytes(cert, "0x00010001");
	writeVoidBytes(cert, 0x34);

	addToIOQueue(NULL, 0, 0, cert);
}

void drawTicketFrame(const char *titleID)
{
	startNewFrame();
	textToFrame(0, 0, "Title ID:");
	textToFrame(1, 3, titleID[0] == '\0' ? "NOT SET" : titleID);
	
	if(titleID[0] == '\0')
		textToFrame(3, 0, "You need to set the title ID to generate a fake ticket");
	
	int line = MAX_LINES - 3;
	textToFrame(line--, 0, "Press " BUTTON_B " to return");
	if(titleID[0] != '\0')
		textToFrame(line--, 0, "Press " BUTTON_A " to continue");
	lineToFrame(line, SCREEN_COLOR_WHITE);
	
	lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);
	textToFrame(MAX_LINES - 1, 0, "Press " BUTTON_LEFT " to set the title ID");
	drawFrame();
}

void generateFakeTicket()
{
	char *dir = fileBrowserMenu();
	if(dir == NULL)
		return;
	
	char tikPath[1024];
	strcpy(tikPath, dir);
	strcat(tikPath, "title");
	char *ptr = tikPath + strlen(tikPath);
	strcpy(ptr, ".tmd");
	MEMFreeToDefaultHeap(dir);
	FILE *f = fopen(tikPath, "rb");
	if(f == NULL)
	{
		drawErrorFrame("Couldn't open title.tmd", ANY_RETURN);

		while(AppRunning())
		{
			showFrame();

			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame("Couldn't open title.tmd", ANY_RETURN);

			if(vpad.trigger)
				break;
		}
		return;
	}

	size_t fs = getFilesize(f);
	void *buf = MEMAllocFromDefaultHeap(fs);
	if(buf == NULL)
	{
		fclose(f);
		return;
	}

	if(fread(buf, fs, 1, f) != 1)
	{
		fclose(f);
		MEMFreeToDefaultHeap(buf);
		drawErrorFrame("Couldn't read title.tmd", ANY_RETURN);

		while(AppRunning())
		{
			showFrame();

			if(app == APP_STATE_BACKGROUND)
				continue;
			if(app == APP_STATE_RETURNING)
				drawErrorFrame("Couldn't read title.tmd", ANY_RETURN);

			if(vpad.trigger)
				break;
		}
		return;
	}

	fclose(f);
	uint64_t tid = ((TMD *)buf)->tid;
	MEMFreeToDefaultHeap(buf);

	char titleID[17];
	hex(tid, 16, titleID);
	
	drawTicketFrame(titleID);
	
	while(AppRunning())
	{
		showFrame();
		
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawTicketFrame(titleID);
		
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			if(titleID[0] != '\0')
			{
				startNewFrame();
				textToFrame(0, 0, "Generating fake ticket...");
				drawFrame();
				showFrame();
				
				strcpy(ptr, ".tik");
				TitleEntry te = { .name = "UNKNOWN", .tid = tid, .region = 0, .key = TITLE_KEY_mypass };

				const TitleEntry *entry = getTitleEntryByTid(tid);
				if(entry == NULL)
					entry = (const TitleEntry *)&te;

				generateTik(tikPath, entry);

				strcpy(ptr, ".cert");
				generateCert(tikPath);
				strcpy(ptr, ".tik");
				
				colorStartNewFrame(SCREEN_COLOR_D_GREEN);
				textToFrame(0, 0, "Fake ticket generated on:");
				char oldChar = tikPath[3];
				tikPath[3] = '\0';
				if(strcmp(tikPath, "fs:") == 0)
				{
					char *adjPath = tikPath + 15;
					adjPath[0] = 's';
					adjPath[1] = 'd';
					adjPath[2] = ':';
					textToFrame(1, 0, adjPath);
				}
				else
				{
					tikPath[3] = oldChar;
					textToFrame(1, 0, tikPath);
				}
				
				textToFrame(3, 0, "Press any key to return");
				drawFrame();
				
				while(AppRunning())
				{
					showFrame();
					
					if(app == APP_STATE_BACKGROUND)
						continue;
					//TODO: APP_STATE_RETURNING
					
					if(vpad.trigger)
						break;
				}
				return;
			}
		}
		else if(vpad.trigger & VPAD_BUTTON_B)
			return;
		
		if(vpad.trigger & VPAD_BUTTON_LEFT)
		{
			showKeyboard(KEYBOARD_LAYOUT_TID, KEYBOARD_TYPE_RESTRICTED, titleID, CHECK_HEXADECIMAL, 16, true, titleID, NULL);
			toLowercase(titleID);
			drawTicketFrame(titleID);
		}
	}

	return;
}
