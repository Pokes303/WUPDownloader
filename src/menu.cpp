#include "menu.hpp"
#include "utils.hpp"

void writeMainMenu() {
	startRefresh();
	write(0, 0, "==============================================================");
	write(0, 1, "=              WUPDownloader by Pokes303               [1.0] =");
	write(0, 2, "==============================================================");
	
	//write(0, 1, "=           WiiU Title Downloader by Pokes303          [1.0] =");
	
	write(0, 4, "Press (A) to download a content from the NUS with the title ID");
	write(0, 5, "Press (Y) to generate a fake <title.tik> file");
	
	write(0, 7, "Press (HOME) to exit on HBL");
	
	write(0, 14, "DO NOT EJECT THE SD CARD OR THE APPLICATION WILL CRASH!");
	
	write(0, 16, "Content = Game/Update/Dlc/Demo/Applet/etc...");
	endRefresh();
}

void writeFolderMenu() {
	write(3, 0, "Select a folder to place the generated fake ticket.tik:");
	write(0, 1, "==============================================================");
	
	
	write(0, 15, "==============================================================");
	write(0, 16, "Press (A) to select || (B) to return || (Y) to refresh");
	write(16, 17, "Searching on => SD:/install/");
}

/*	Some ideas:
*Support for modding games (like VC black screen)??
*Multiple downloads
*Custom download servers
*Use WiiMote
*Download's error code buffer set
*Download titles to USB
*Create manual
*Decrypt downloads
*/
