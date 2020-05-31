/****************************************************************
 * This file is part of NUSspli.                                *
 * (c) 2019 Pokes303                                            *
 * (c) 2020 V10lator <v10lator@myway.de>                        *
 *                                                              *
 * Licensed under the MIT license (see LICENSE.txt for details) *
 ****************************************************************/

#ifndef NUSSPLI_MENU_DOWNLOAD_H
#define NUSSPLI_MENU_DOWNLOAD_H

#include <wut-fixups.h>

#ifdef __cplusplus
	extern "C" {
#endif

//TODO: Replace this with something better.
typedef struct
{
	char *tid;
	char *name;
} GameInfo;

void downloadMenu();
bool downloadJSON();
void freeJSON();

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_MENU_DOWNLOAD_H
