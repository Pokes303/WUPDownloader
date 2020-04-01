#include "ticket.h"

#include "utils.h"
#include "file.h"
#include "menu.h"
#include "input.h"
#include "status.h"
#include "screen.h"

#include <stdio.h>
#include <string.h>

#include <whb/log.h>
#include <whb/proc.h>

void generateTik(FILE* tik, char* titleID, char* encKey) { //Based on NUSPacker tik creation function
	WHBLogPrintf("Generate tik function");
	
	writeCustomBytes(tik, "00010004");
	writeRandomBytes(tik, 0x100);
	writeVoidBytes(tik, 0x3C);
	writeCustomBytes(tik, "526F6F742D434130303030303030332D58533030303030303063000000000000");
	writeVoidBytes(tik, 0x5C);
	writeCustomBytes(tik, "010000");
	writeCustomBytes(tik, encKey);
	writeCustomBytes(tik, "000005");
	writeRandomBytes(tik, 0x06);
	writeVoidBytes(tik, 0x04);
	writeCustomBytes(tik, titleID);
	writeCustomBytes(tik, "00000011000000000000000000000005");
	writeVoidBytes(tik, 0xB0);
	writeCustomBytes(tik, "00010014000000AC000000140001001400000000000000280000000100000084000000840003000000000000FFFFFF01");
	writeVoidBytes(tik, 0x7C);
}

bool generateFakeTicket() {
	FSDirectoryHandle dh;
	FSStatus fsr = FSOpenDir(fsCli, fsCmd, INSTALL_DIR, &dh, 0);
	WHBLogPrintf("fsr: %u", fsr);
	
	int tikFoldersSize = 1;
	char* tikFolders[1024];
	tikFolders[0] = "<CURRENT DIRECTORY>";
	FSDirectoryEntry de;
	if (fsr == FS_STATUS_OK)
	{
		while (FSReadDir(fsCli, fsCmd, dh, &de, 0) == FS_STATUS_OK && tikFoldersSize < 1024)
			if (de.info.flags == 0x8C000000) //Check if it's a directory
				tikFolders[tikFoldersSize++] = de.name;
		FSCloseDir(fsCli, fsCmd, dh, 0);
	}
	
	int tikCursor = 0;
	int tikPos = 0;
	bool mov = tikFoldersSize >= 12;
	
	while(AppRunning()) {
		if (app == 2)
			continue;
		
		readInput();
		
		startRefresh();
		writeFolderMenu();
		
		if (fsr == FS_STATUS_OK) {
			char* toWrite;
			for (int i = 0; i < 13 && i < tikFoldersSize; i++) {
				toWrite = tikFolders[i + tikPos];
				strcat(toWrite, i == 0 ? "" : "/");
				write(1, i + 2, toWrite);
				if (tikCursor == i)
					write(0, tikCursor + 2, ">");
			}
		}
		else {
			char toWrite[64];
			sprintf(toWrite, "ERROR OPENING THE FOLDER: %d", fsr);
			write(0, 2, toWrite);
			write(1, 2, "Try to relaunch the application");
		}
		
		switch(vpad.trigger) {
			case VPAD_BUTTON_A:
				if (fsr == FS_STATUS_OK)
					goto inputTikValues;
				else
					break;
			case VPAD_BUTTON_B:
				return true;
			case VPAD_BUTTON_Y:
				return generateFakeTicket();
			case VPAD_BUTTON_UP:
				if (tikCursor <= 0) {
					if (mov) {
						if (tikPos > 0) {
							tikPos--;
						}
						else {
							tikCursor = 12;
							tikPos = tikFoldersSize % 12 - 1;
						}
					}
					else {
						tikCursor = tikFoldersSize - 1;
					}
				}
				else {
					tikCursor--;
				}
				break;
			case VPAD_BUTTON_DOWN:
				if (tikCursor >= 12 || tikCursor >= tikFoldersSize - 1) {
					if (mov && tikPos < tikFoldersSize % 12 - 1)
						tikPos++;
					else {
						tikCursor = 0;
						tikPos = 0;
					}
				}
				else {
					tikCursor++;
				}
				break;
		}
		endRefresh();
	}
	return false;
	
inputTikValues: ;
	char* titleID;
	char* encKey;

	while (AppRunning()) {
		if (app == 2)
			continue;
		
		readInput();

		startRefresh();
		write(0, 0, "Title ID:");
		write(1, 1, titleID[0] == '\0' ? "NOT SET" : titleID);
		write(0, 2, "Encrypted title key");
		write(1, 3, encKey[0] == '\0' ? "NOT SET" : encKey);
		
		write(0, 5, "You need to set the title ID and the Encrypted title key");
		write(0, 6, " to generate a fake ticket");
		
		if (titleID[0] != '\0' && encKey[0] != '\0')
			write(0, 8, "Press (A) to continue");
		write(0, 9, "Press (B) to return");
		paintLine(10, SCREEN_COLOR_WHITE);
		write(0, 11, "Press (UP) to set the title ID");
		write(0, 12, "Press (DOWN) to set the Encrypted title key");
		endRefresh();
		
		switch (vpad.trigger) {
			case VPAD_BUTTON_A:
				if (titleID[0] != '\0' && titleID[0] != '\0') {
					startRefresh();
					write(0, 0, "Generating fake ticket...");
					endRefresh();
					
					FILE* fakeTik;
					char* tikPath;
					if(tikCursor == 0) {
						tikPath = "ticket_";
						strcpy(tikPath, titleID);
						strcpy(tikPath, "_");
						strcpy(tikPath, encKey);
						strcpy(tikPath, ".tik");
					} else
						tikPath = tikFolders[tikCursor + tikPos + 1];
					char tmpPath[256];
					strcpy(tmpPath, INSTALL_DIR);
					strcat(tmpPath, tikPath);
					fakeTik = fopen(tmpPath, "wb");
					generateTik(fakeTik, titleID, encKey);
					fclose(fakeTik);
					
					while(AppRunning()) {
						if (app == 2)
							continue;
		
						readInput();
						
						colorStartRefresh(SCREEN_COLOR_GREEN);
						write(0, 0, "Fake ticket generated on:");
						write(0, 1, " SD:/install/");
						write(13, 1, tikPath);
						write(0, 3, "Press (A) to return");
						endRefresh();
						
						if (vpad.trigger == VPAD_BUTTON_A)
							return true;
					}
					return false;
				}
				break;
			case VPAD_BUTTON_B:
				return true;
			case VPAD_BUTTON_UP:
				showKeyboard(titleID, CHECK_HEXADECIMAL, 16, true);
				break;
			case VPAD_BUTTON_DOWN:
				showKeyboard(encKey, CHECK_HEXADECIMAL, 32, true);
				break;
		}
	}
	return false;
}