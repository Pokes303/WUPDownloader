#ifndef WUPD_SCREEN_H
#define WUPD_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

#define SCREEN_COLOR_BLACK	0x00000000
#define SCREEN_COLOR_WHITE	0xFFFFFF00
#define SCREEN_COLOR_RED	0xc0000000
#define SCREEN_COLOR_GREEN	0x00800000
#define SCREEN_COLOR_LILA	0xC000BE00

#define MAX_LINE_DRC 14
#define MAX_LINE_TV 24
#define MAX_X_DRC 802
#define MAX_X_TV 1236

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum {
	B_RETURN = 0,
	B_RETURN__A_CONTINUE = 1,
	B_RETURN__Y_RETRY = 2
} ErrorOptions;

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
void errorScreen(int line, ErrorOptions option);
void writeRetry();

#ifdef __cplusplus
	}
#endif

#endif // ifndef WUPD_SCREEN_H
