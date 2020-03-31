#include "main.h"
#include "menu.h"
#include "utils.h"
#include "screen.h"

void writeMainMenu() {
	startRefresh();
	paintLine(0, SCREEN_COLOR_WHITE);
	write(0, 1, "|        WUPDownloader by Pokes303 and V10lator");
	char toScreen[256];
	strcpy(toScreen, "[");
	strcat(toScreen, VERSION);
	strcat(toScreen, "] |");
	write(65, 1, toScreen);
	paintLine(2, SCREEN_COLOR_WHITE);
	
	write(0, 4, "Press (A) to download a content from the NUS with the title ID");
	write(0, 5, "Press (Y) to generate a fake <title.tik> file");
	
	write(0, 7, "Press (HOME) or (B) to exit");
	
	
	write(0, 12, "Warning:");
	write(0, 13, "-Don't eject the SD Card or the application will crash!");
	write(0, 14, "-You are unable to exit while downloading a game");
	write(0, 15, "-This app doesn't support piracy. Only download games that you");
	write(1, 16, "had bought before!");
	endRefresh();
}

void writeFolderMenu() {
	write(3, 0, "Select a folder to place the generated fake ticket.tik:");
	paintLine(1, SCREEN_COLOR_WHITE);
	
	
	paintLine(15, SCREEN_COLOR_WHITE);
	write(0, 16, "Press (A) to select || (B) to return || (Y) to refresh");
	write(16, 17, "Searching on => SD:/install/");
}