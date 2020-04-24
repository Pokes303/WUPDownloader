#include "utils.hpp"
#include "log.hpp"

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
		wlogf("Error initialising screen library");
		shutdownScreen();
		return false;
	}

	OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
	OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);

	OSScreenEnableEx(SCREEN_TV, true);
	OSScreenEnableEx(SCREEN_DRC, true);
		wlogf("Screen library initialised successfully");
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

void write(uint32_t row, uint32_t column, const char* str) {
	OSScreenPutFontEx(SCREEN_TV, row, column, str);
	OSScreenPutFontEx(SCREEN_DRC, row, column, str);
}
void swrite(uint32_t row, uint32_t column, std::string str) {
	OSScreenPutFontEx(SCREEN_TV, row, column, str.c_str());
	OSScreenPutFontEx(SCREEN_DRC, row, column, str.c_str());
}

std::vector<std::string> downloadLog;
void addToDownloadLog(std::string str) {
	if (downloadLog.size() >= MAX_DOWNLOADLOG_DRC)
		downloadLog.erase(downloadLog.begin());
	downloadLog.push_back(str);
	wlogf(str.c_str());
}
void clearDownloadLog() {
	downloadLog.clear();
}
void writeDownloadLog() {
	write(0, 2, "------------------------------------------------------------");
	for (uint8_t i = 0; i < MAX_DOWNLOADLOG_DRC; i++) {
		if (i >= downloadLog.size())
			break;
		write(0, i + 3, downloadLog[i].c_str());
	}
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

std::string hex(uint64_t i) {
	std::string result;
	while (true) {
		uint64_t div = i / 16;
		uint64_t remainder = i % 16;
		std::string wRemainder = std::to_string(remainder);
		if (remainder > 9) {
			switch (remainder) {
			case 10:
				wRemainder = "A";
				break;
			case 11:
				wRemainder = "B";
				break;
			case 12:
				wRemainder = "C";
				break;
			case 13:
				wRemainder = "D";
				break;
			case 14:
				wRemainder = "E";
				break;
			case 15:
				wRemainder = "F";
				break;
			}
		}
		result = wRemainder + result;

		if (div != 0)
			i = div;
		else
			break;
	}
	result = "0x" + result;
	return result;
}
std::string hex0(uint64_t i, uint8_t digits) {
	std::string result;
	while (true) {
		uint64_t div = i / 16;
		uint64_t remainder = i % 16;
		std::string wRemainder = std::to_string(remainder);
		if (remainder > 9) {
			switch (remainder) {
			case 10:
				wRemainder = "A";
				break;
			case 11:
				wRemainder = "B";
				break;
			case 12:
				wRemainder = "C";
				break;
			case 13:
				wRemainder = "D";
				break;
			case 14:
				wRemainder = "E";
				break;
			case 15:
				wRemainder = "F";
				break;
			}
		}
		result = wRemainder + result;

		if (div != 0)
			i = div;
		else
			break;
	}
	if (result.size() > digits)
		return std::string("too few digits error");
	for (int i = digits - result.size(); i > 0; i--)
		result = "0" + result;
	return result;
}