#include "ticket.hpp"

#include "utils.hpp"
#include "file.hpp"
#include "menu.hpp"
#include "input.hpp"

#include <whb/proc.h>

void generateTik(FILE* tik, std::string titleID, std::string encKey) { //Based on NUSPacker tik creation function
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
	FSStatus fsr = FSOpenDir(fsCli, fsCmd, installDir.c_str(), &dh, 0);
	WHBLogPrintf("fsr: %u", fsr);
	
	std::vector<std::string> tikFolders;
		
	if (fsr == FS_STATUS_OK) {

		tikFolders.push_back("<CURRENT DIRECTORY>");
		FSDirectoryEntry de;
		while (FSReadDir(fsCli, fsCmd, dh, &de, 0) == FS_STATUS_OK) {
			if (de.info.flags == 0x8C000000) //Check if it's a directory
				tikFolders.push_back(de.name);
		}
		FSCloseDir(fsCli, fsCmd, dh, 0);
	}
	
	uint32_t tikCursor = 0;
	uint32_t tikPos = 0;
	bool mov = tikFolders.size() >= 12;
	
	while(WHBProcIsRunning()) {
		readInput();
		
		startRefresh();
		writeFolderMenu();
		
		if (fsr == FS_STATUS_OK) {
			for (uint32_t i = 0; i < 13 && i < tikFolders.size(); i++) {
				swrite(1, i + 2, tikFolders[i + tikPos] + ((i == 0) ? "" : "/"));
				if (tikCursor == i)
					write(0, tikCursor + 2, ">");
			}
		}
		else {
			write(0, 2, "ERROR OPENING THE FOLDER:");
			swrite(32, 2, std::to_string(fsr));
			write(0, 2, "Try to relaunch the application");
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
							tikPos = tikFolders.size() % 12 - 1;
						}
					}
					else {
						tikCursor = tikFolders.size() - 1;
					}
				}
				else {
					tikCursor--;
				}
				break;
			case VPAD_BUTTON_DOWN:
				if (tikCursor >= 12 || tikCursor >= tikFolders.size() - 1) {
					if (mov && tikPos < tikFolders.size() % 12 - 1)
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
	
inputTikValues:
	std::string titleID;
	std::string encKey;

	while (WHBProcIsRunning()) {
		readInput();

		startRefresh();
		write(0, 0, "Title ID:");
		swrite(1, 1, (titleID == "") ? "NOT SET" : titleID);
		write(0, 2, "Encrypted title key");
		swrite(1, 3, (encKey == "") ? "NOT SET" : encKey);
		
		write(0, 5, "You need to set the title ID and the Encrypted title key");
		write(0, 6, " to generate a fake ticket");
		
		write(0, 8, "Press (A) to continue");
		if (titleID == "" || encKey == "")
			write(0, 8, "=====================");
		write(0, 9, "Press (B) to return");
		write(0, 10, "-------------------------------------------------------");
		write(0, 11, "Press (UP) to set the title ID");
		write(0, 12, "Press (DOWN) to set the Encrypted title key");
		endRefresh();
		
		switch (vpad.trigger) {
			case VPAD_BUTTON_A:
				if (titleID != "" && encKey != "") {
					startRefresh();
					write(0, 0, "Generating fake ticket...");
					endRefresh();
					
					FILE* fakeTik;
					std::string tikPath = ((tikCursor == 0) ? ("ticket_" + titleID + "_" + encKey + ".tik") : tikFolders[tikCursor + tikPos + 1]);
					fakeTik = fopen((installDir + tikPath).c_str(), "wb");
					generateTik(fakeTik, titleID, encKey);
					fclose(fakeTik);
					
					while(WHBProcIsRunning()) {
						readInput();
						
						colorStartRefresh(0x00800000);
						write(0, 0, "Fake ticket generated on:");
						swrite(0, 1, " SD:/install/" + tikPath);
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
				showKeyboard(&titleID, CHECK_HEXADECIMAL, 16, true);
				break;
			case VPAD_BUTTON_DOWN:
				showKeyboard(&encKey, CHECK_HEXADECIMAL, 32, true);
				break;
		}
	}
	return false;
}