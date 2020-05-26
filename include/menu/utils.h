/****************************************************************
 * This file is part of NUSspli.                                *
 * (c) 2019 Pokes303                                            *
 * (c) 2020 V10lator <v10lator@myway.de>                        *
 *                                                              *
 * Licensed under the MIT license (see LICENSE.txt for details) *
 ****************************************************************/

#ifndef NUSSPLI_MENU_UTILS_H
#define NUSSPLI_MENU_UTILS_H

#include <wut-fixups.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum {
	B_RETURN = 1,
	A_CONTINUE = 2,
	Y_RETRY = 4,
} ErrorOptions;

void addToScreenLog(const char *str, ...);
void clearScreenLog();
void writeScreenLog();
void drawErrorFrame(const char *text, ErrorOptions option);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_MENU_UTILS_H
