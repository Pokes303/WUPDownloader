/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2021 V10lator <v10lator@myway.de>                    *
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

#ifndef NUSSPLI_TITLES_H
#define NUSSPLI_TITLES_H

#include <wut-fixups.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum
{
	TID_HIGH_GAME =			0x00050000,
	TID_HIGH_DEMO =			0x00050002,
	TID_HIGH_SYSTEM_APP =		0x00050010,
	TID_HIGH_SYSTEM_DATA =		0x0005001B,
	TID_HIGH_SYSTEM_APPLET =	0x00050030,
	TID_HIGH_VWII_IOS =		0x00000007,
	TID_HIGH_VWII_SYSTEM_APP =	0x00070002,
	TID_HIGH_VWII_SYSTEM =		0x00070008,
	TID_HIGH_DLC =			0x0005000C,
	TID_HIGH_UPDATE =		0x0005000E,
} TID_HIGH;

typedef enum
{
	TRANSFORMED_TID_HIGH_GAME =		0,
	TRANSFORMED_TID_HIGH_DEMO =		1,
	TRANSFORMED_TID_HIGH_SYSTEM_APP =	2,
	TRANSFORMED_TID_HIGH_SYSTEM_DATA =	3,
	TRANSFORMED_TID_HIGH_SYSTEM_APPLET =	4,
	TRANSFORMED_TID_HIGH_VWII_IOS =		5,
	TRANSFORMED_TID_HIGH_VWII_SYSTEM_APP =	6,
	TRANSFORMED_TID_HIGH_VWII_SYSTEM =	7,
	TRANSFORMED_TID_HIGH_DLC =		8,
	TRANSFORMED_TID_HIGH_UPDATE =		9,
} TRANSFORMED_TID_HIGH;

typedef enum
{
	TITLE_REGION_UNKNOWN =	0,
	TITLE_REGION_EUR = 	1,
	TITLE_REGION_USA = 	1 << 1,
	TITLE_REGION_JAP = 	1 << 2,
	TITLE_REGION_ALL =	TITLE_REGION_EUR | TITLE_REGION_USA | TITLE_REGION_JAP,
} TITLE_REGION;

typedef enum
{
	TITLE_CATEGORY_GAME = 0,
	TITLE_CATEGORY_UPDATE = 1,
	TITLE_CATEGORY_DLC = 2,
	TITLE_CATEGORY_DEMO = 3,
	TITLE_CATEGORY_ALL = 4,
} TITLE_CATEGORY;

typedef enum
{
	TITLE_KEY_mypass	= 0,
	TITLE_KEY_nintendo	= 1,
	TITLE_KEY_test		= 2,
	TITLE_KEY_1234567890	= 3,
	TITLE_KEY_Lucy131211	= 4,
	TITLE_KEY_fbf10		= 5,
	TITLE_KEY_5678		= 6,
	TITLE_KEY_1234		= 7,
	TITLE_KEY_		= 8
} TITLE_KEY;

typedef struct
{
	char *name;
	uint64_t tid;
	bool isDLC;
	bool isUpdate;
	TITLE_REGION region;
	TITLE_KEY key;
} TitleEntry;

#define getTidHighFromTid(tid) *(uint32_t *)&tid

TitleEntry *getTitleEntries(TITLE_CATEGORY cat);
size_t getTitleEntriesSize(TITLE_CATEGORY cat);
TitleEntry *getTitleEntryByTid(uint64_t tid);
char *tid2name(const char *tid);
bool name2tid(const char *name, char *out);
bool initTitles();
void clearTitles();

static inline bool isGame(const TitleEntry *entry)
{
	return (TID_HIGH)(entry->tid >> 32) == TID_HIGH_GAME;
}

static inline bool isDLC(const TitleEntry *entry)
{
	return (TID_HIGH)(entry->tid >> 32) == TID_HIGH_DLC;
}

static inline bool isUpdate(const TitleEntry *entry)
{
	return (TID_HIGH)(entry->tid >> 32) == TID_HIGH_UPDATE;
}

static inline bool isDemo(const TitleEntry *entry)
{
	return (TID_HIGH)(entry->tid >> 32) == TID_HIGH_DEMO;
}

// TODO

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_TITLES_H
