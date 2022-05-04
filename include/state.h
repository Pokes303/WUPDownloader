/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *
 ***************************************************************************/

#ifndef NUSSPLI_STATE_H
#define NUSSPLI_STATE_H

#include <wut-fixups.h>

#include <stdbool.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum
{
	APP_STATE_STOPPING,
	APP_STATE_HOME,
	APP_STATE_STOPPED,
	APP_STATE_RUNNING,
	APP_STATE_BACKGROUND,
	APP_STATE_RETURNING,
	
} APP_STATE;

extern APP_STATE app;

void initState();
void enableApd();
void disableApd();
void enableShutdown();
void disableShutdown();
bool isAroma();
#ifdef NUSSPLI_HBL
#define isChannel() false
#else
bool isChannel();
#endif
bool AppRunning();
uint32_t homeButtonCallback(void *dummy);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_STATE_H
