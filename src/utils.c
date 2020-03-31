#include "main.h"
#include "utils.h"
#include "screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <coreinit/cache.h>
#include <coreinit/energysaver.h>
#include <coreinit/foreground.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/screen.h>
#include <coreinit/systeminfo.h>

size_t tvBufferSize;
size_t drcBufferSize;

void* tvBuffer;
void* drcBuffer;

bool initScreen() {
	OSScreenInit();

	tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
	drcBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

	tvBuffer = MEMAllocFromDefaultHeapEx(tvBufferSize, 0x100);
	drcBuffer = MEMAllocFromDefaultHeapEx(drcBufferSize, 0x100);

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
	if (tvBuffer) MEMFreeToDefaultHeap(tvBuffer);
	if (drcBuffer) MEMFreeToDefaultHeap(drcBuffer);
	
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

void paintLine(uint32_t column, uint32_t colour)
{
	column *= 24;
	column += 45;
	uint32_t c2;
	for(uint32_t x = 53; x < MAX_X_DRC; x++)
	{
		OSScreenPutPixelEx(SCREEN_DRC, x, column, colour);
		OSScreenPutPixelEx(SCREEN_TV, x, column, colour);
		c2 = column + 1;
		OSScreenPutPixelEx(SCREEN_DRC, x, c2, colour);
		OSScreenPutPixelEx(SCREEN_TV, x, c2, colour);
	}
	for(uint32_t x = MAX_X_DRC; x < MAX_X_TV; x++)
	{
		OSScreenPutPixelEx(SCREEN_TV, x, column, colour);
		OSScreenPutPixelEx(SCREEN_TV, x, c2, colour);
	}
}

void write(uint32_t row, uint32_t column, const char* str) { //Write to the two screens
	uint32_t dRow;
	if(row > 64)
	{
		uint32_t len = strlen(str);
		row = 98 - len;
		dRow = 62 - len;
	}
	else
		dRow = row;
	OSScreenPutFontEx(SCREEN_TV, row, column, str);
	OSScreenPutFontEx(SCREEN_DRC, dRow, column, str);
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
	
	DownloadLogList *newEntry = MEMAllocFromDefaultHeap(sizeof(DownloadLogList));
	if(newEntry ==  NULL)
		return;
	
	//TODO: We copy the string here for fast porting purposes
	newEntry->line = MEMAllocFromDefaultHeap(sizeof(char) * (strlen(str) + 1));
	if(newEntry->line == NULL)
	{
		MEMFreeToDefaultHeap(newEntry);
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
	
	if(i == MAX_LINE_DRC)
	{
		DownloadLogList *tmpList = downloadLogList;
		downloadLogList = tmpList->nextEntry;
		MEMFreeToDefaultHeap(tmpList->line);
		MEMFreeToDefaultHeap(tmpList);
	}
	
	last->nextEntry = newEntry;
}
void clearDownloadLog() {
	DownloadLogList *tmpList;
	while(downloadLogList != NULL)
	{
		tmpList = downloadLogList;
		downloadLogList = tmpList->nextEntry;
		MEMFreeToDefaultHeap(tmpList->line);
		MEMFreeToDefaultHeap(tmpList);
	}
}
void writeDownloadLog() {
	paintLine(2, SCREEN_COLOR_WHITE);
	int i = 3;
	for(DownloadLogList *entry = downloadLogList; entry != NULL; entry = entry->nextEntry)
		write(0, i++, entry->line);
}

void colorStartRefresh(uint32_t color) {
	OSScreenClearBufferEx(SCREEN_TV, color);
	OSScreenClearBufferEx(SCREEN_DRC, color);
}

void errorScreen(int line, ErrorOptions option) {
	paintLine(line++, SCREEN_COLOR_WHITE);
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
	
	char *result = MEMAllocFromDefaultHeap(sizeof(char) * (digits + 1));
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
