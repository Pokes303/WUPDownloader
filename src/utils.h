#ifndef WUPD_UTILS_H
#define WUPD_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_LINE_DRC 14
#define MAX_LINE_TV 22
#define MAX_X_DRC 802
#define MAX_X_TV 1236

#ifdef __cplusplus
	extern "C" {
#endif

bool initScreen();
void shutdownScreen();

void startRefresh();
void endRefresh();

void write(uint32_t row, uint32_t column, const char* str);
void paintLine(uint32_t row, uint32_t colour);

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

#ifdef __cplusplus
	}
#endif

#endif // ifndef WUPD_UTILS_H
