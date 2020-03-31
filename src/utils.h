#ifndef WUPD_UTILS_H
#define WUPD_UTILS_H

#include <stdbool.h>

#include "main.h"

bool initScreen();
void shutdownScreen();

void startRefresh();
void endRefresh();

void write(int row, int column, const char* str);
void writeParsed(int row, int column, const char* str);

void addToDownloadLog(char* str);
void clearDownloadLog();
void writeDownloadLog();

void colorStartRefresh(uint32_t color);

typedef enum {
	B_RETURN = 0,
	B_RETURN__A_CONTINUE = 1,
	B_RETURN__Y_RETRY = 2
} ErrorOptions;

void errorScreen(int line, ErrorOptions option);
void writeRetry();

void enableShutdown();
void disableShutdown();

char* b_tostring(bool b);

char* hex(uint64_t i, int digits); //ex: 000050D1

#endif // ifndef WUPD_UTILS_H
