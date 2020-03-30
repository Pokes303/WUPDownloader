#include "utils.h"

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
	OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
	OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);
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

/*void dwrite(uint32_t column, const char* str, ...) { //const char* write
	OSScreenPutFontEx(SCREEN_TV, 0, column, str);
	OSScreenPutFontEx(SCREEN_DRC, 0, column, str);
}*/

char* downloadLog[MAX_DOWNLOADLOG_DRC + 1];
size_t downloadLogSize = 0;
void addToDownloadLog(char* str) {
	//TODO: We copy the string here for fast porting purposes
	char *toAdd = malloc(sizeof(char) * (strlen(str) + 1));
	if(toAdd == NULL)
		return;
	strcpy(toAdd, str);
	
	bool needsIncrement;
	if(downloadLogSize == MAX_DOWNLOADLOG_DRC)
	{
		free(downloadLog[0]);
		for(int i = 0; i < MAX_DOWNLOADLOG_DRC - 1; i++)
			downloadLog[i] = downloadLog[i + 1];
		needsIncrement = false;
	}
	else
		needsIncrement = true;
	
	WHBLogPrintf("Adding %s at %d", toAdd, downloadLogSize);
	downloadLog[downloadLogSize] = toAdd;
	WHBLogPrintf(toAdd);
	
	if(needsIncrement)
		downloadLogSize++;
}
void clearDownloadLog() {
	for(int i = 0; i < downloadLogSize; i++)
		free(downloadLog[i]);
	downloadLogSize = 0;
}
void writeDownloadLog() {
	write(0, 2, "------------------------------------------------------------");
	for (uint8_t i = 0; i < downloadLogSize; i++)
		write(0, i + 3, downloadLog[i]);
}

void colorStartRefresh(uint32_t color) {
	OSScreenClearBufferEx(SCREEN_TV, color);
	OSScreenClearBufferEx(SCREEN_DRC, color);
}

void errorScreen(uint32_t line, ErrorOptions option) {
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

char* hex(uint64_t i, uint8_t digits) {
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
