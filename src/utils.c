#include "utils.h"
#include "screen.h"

#ifdef DEBUG_BUILD
#include <stdio.h>
#endif

#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <coreinit/systeminfo.h>
#include <coreinit/energysaver.h>

#define MAX_DOWNLOADLOG_DRC 14

size_t tvBufferSize;
size_t drcBufferSize;

void* tvBuffer;
void* drcBuffer;

bool initScreen() {
	OSScreenInit();

	tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
	drcBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

	tvBuffer = memalign(0x100, tvBufferSize);
	drcBuffer = memalign(0x100, drcBufferSize);

	if (!tvBuffer || !drcBuffer) {
		WHBLogPrintf("Error initialising screen library");
		shutdownScreen();
		return false;
	}

	OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
	OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);

	OSScreenEnableEx(SCREEN_TV, true);
	OSScreenEnableEx(SCREEN_DRC, true);
		WHBLogPrintf("Screen library initialised successfully");
	return true;
}
void shutdownScreen() {
	if (tvBuffer) free(tvBuffer);
	if (drcBuffer) free(drcBuffer);
	
	OSScreenShutdown();
}

void startRefresh() {
	colorStartRefresh(SCREEN_COLOR_BLACK);
}
void endRefresh() {
	DCFlushRange(tvBuffer, tvBufferSize);
	DCFlushRange(drcBuffer, drcBufferSize);

	OSScreenFlipBuffersEx(SCREEN_TV);
	OSScreenFlipBuffersEx(SCREEN_DRC);
}

void write(uint32_t row, uint32_t column, const char* str) { //Write to the two screens
	OSScreenPutFontEx(SCREEN_TV, row, column, str);
	OSScreenPutFontEx(SCREEN_DRC, row, column, str);
}

struct DownloadLogList;
typedef struct DownloadLogList DownloadLogList;
struct DownloadLogList
{
	char *line;
	DownloadLogList *nextEntry;
};
DownloadLogList *downloadLogList = NULL;
void addToDownloadLog(char* str) {
	WHBLogPrintf(str);
	
	DownloadLogList *newEntry = malloc(sizeof(DownloadLogList));
	if(newEntry ==  NULL)
		return;
	
	//TODO: We copy the string here for fast porting purposes
	newEntry->line = malloc(sizeof(char) * (strlen(str) + 1));
	if(newEntry->line == NULL)
	{
		free(newEntry);
		return;
	}
	strcpy(newEntry->line, str);
	newEntry->nextEntry = NULL;
	
	if(downloadLogList == NULL)
	{
		downloadLogList = newEntry;
		return;
	}
	
	DownloadLogList *last;
	int i = 0;
	for(DownloadLogList *c = downloadLogList; c != NULL; c = c->nextEntry)
	{
		
		i++;
		last = c;
	}
	
	if(i == MAX_DOWNLOADLOG_DRC)
	{
		DownloadLogList *tmpList = downloadLogList;
		downloadLogList = tmpList->nextEntry;
		free(tmpList->line);
		free(tmpList);
	}
	
	last->nextEntry = newEntry;
}
void clearDownloadLog() {
	DownloadLogList *tmpList;
	while(downloadLogList != NULL)
	{
		tmpList = downloadLogList;
		downloadLogList = tmpList->nextEntry;
		free(tmpList->line);
		free(tmpList);
	}
}
void writeDownloadLog() {
	write(0, 2, "------------------------------------------------------------");
	int i = 3;
	for(DownloadLogList *entry = downloadLogList; entry != NULL; entry = entry->nextEntry)
		write(0, i++, entry->line);
}

void colorStartRefresh(uint32_t color) {
	OSScreenClearBufferEx(SCREEN_TV, color);
	OSScreenClearBufferEx(SCREEN_DRC, color);
}

void errorScreen(int line, ErrorOptions option) {
	write(0, line++, "------------------------------------------------------------");
	switch (option) {
		case B_RETURN:
			write(0, line, "Press (B) to return");
			return;
		case B_RETURN__A_CONTINUE:
			write(0, line++, "Press (A) to continue");
			write(0, line, "Press (B) to return");
			return;
		case B_RETURN__Y_RETRY:
			write(0, line++, "Press (Y) to retry");
			write(0, line, "Press (B) to return");
			return;
	}
}

void writeRetry() {
	startRefresh();
	write(0, 0, "Preparing the retry...");
	writeDownloadLog();
	endRefresh();
}

void enableShutdown() {
	if (!hbl)
		OSEnableHomeButtonMenu(true);
	IMEnableAPD();
}
void disableShutdown() {
	if (!hbl)
		OSEnableHomeButtonMenu(false);
	IMDisableAPD();
}

char* hex(uint64_t i, int digits) {
	unsigned long l1 = (unsigned long)((i & 0xFFFF0000) >> 16 );
	unsigned long l2 = (unsigned long)((i & 0x0000FFFF));
	char h[32];
	sprintf(h, "%lx%lx", l1, l2); //TODO: We removed 0x as it's not shown in the headers example and to reuse this in function hex0
	size_t hexDigits = strlen(h);
	if (hexDigits > digits)
		return "too few digits error";
	
	char *result = malloc(sizeof(char) * (digits + 1));
	if(result == NULL)
		return NULL;
	
	int n = digits - hexDigits;
	if(n > 0)
	{
		int i = 0;
		for( ; i < n; i++)
			result[i] = '0';
		result[i] = '\0';
		strcat(result, h);
	}
	else
		strcat(result, h);
	
	return result;
}
