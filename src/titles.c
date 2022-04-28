/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
 * Copyright (c) 2022 Xpl0itU <DaThinkingChair@protonmail.com>             *
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

#include <wut-fixups.h>

#include <config.h>
#include <downloader.h>
#include <gtitles.h>
#include <renderer.h>
#include <romfs.h>
#include <titles.h>
#include <utils.h>
#include <menu/utils.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

static TitleEntry *filteredEntry[4] = { NULL, NULL, NULL, NULL }; // Games, Updates, DLC, Demos

const TitleEntry *getTitleEntries(TITLE_CATEGORY cat)
{
	return cat == TITLE_CATEGORY_ALL ? getAllTitleEntries() : filteredEntry[cat];
	
}

const TitleEntry *getTitleEntryByTid(uint64_t tid)
{
	const TitleEntry *haystack;
	size_t haySize;
	
	switch(getTidHighFromTid(tid))
	{
		case TID_HIGH_GAME:
			haystack = filteredEntry[TITLE_CATEGORY_GAME];
			haySize = getTitleEntriesSize(TITLE_CATEGORY_GAME);
			break;
		case TID_HIGH_UPDATE:
			haystack = filteredEntry[TITLE_CATEGORY_UPDATE];
			haySize = getTitleEntriesSize(TITLE_CATEGORY_UPDATE);
			break;
		case TID_HIGH_DLC:
			haystack = filteredEntry[TITLE_CATEGORY_DLC];
			haySize = getTitleEntriesSize(TITLE_CATEGORY_DLC);
			break;
		case TID_HIGH_DEMO:
			haystack = filteredEntry[TITLE_CATEGORY_DEMO];
			haySize = getTitleEntriesSize(TITLE_CATEGORY_DEMO);
			break;
		default:
			haystack = getAllTitleEntries();
			haySize = getTitleEntriesSize(TITLE_CATEGORY_ALL);
		
	}
	
	for(size_t i = 0; i < haySize; ++i, ++haystack)
		if(haystack->tid == tid)
			return haystack;
	
	return NULL;
}

const char *tid2name(const char *tid)
{
	uint64_t rtid;
	hexToByte(tid, (uint8_t *)&rtid);
	const TitleEntry *e = getTitleEntryByTid(rtid);
	return e == NULL ? "UNKNOWN" : e->name;
}

bool name2tid(const char *name, char *out)
{
	size_t lower = 0;
	size_t upper = getTitleEntriesSize(TITLE_CATEGORY_ALL);
	size_t current = upper / 2;
	int strret;
	
	const TitleEntry *titleEntry = getAllTitleEntries();
	while(lower != upper)
	{
		strret =  strcmp(titleEntry[current].name, name);
		if(strret == 0)
        {
			hex(titleEntry[current].tid, 16, out);
            return true;
        }
		
		if(strret < 0)
			upper = current;
		else
			lower = current;
		
/*		if(lower == upper + 1)
		{
			current = current == lower ? current + 1 : current - 1;
			strret =  strcmp(titleEntry[current].name, name);
			return strret == 0 ? titleEntry[current].tid : 0;
		}
		*/
		current = ((upper - lower) / 2) + lower;
	}
	return false;
}

bool initTitles()
{
	startNewFrame();
	textToFrame(0, 0, "Initialising title database");
	writeScreenLog(1);
	drawFrame();
	showFrame();
	
	const TitleEntry *entries = getAllTitleEntries();
	uint32_t cat;
	uint32_t oldCat = 99;
	for(size_t i = 0; i < getTitleEntriesSize(TITLE_CATEGORY_ALL); ++i)
	{
		switch(getTidHighFromTid(entries[i].tid))
		{
			case TID_HIGH_GAME:
				cat = 0;
				break;
			case TID_HIGH_UPDATE:
				cat = 1;
				break;
			case TID_HIGH_DLC:
				cat = 2;
				break;
			case TID_HIGH_DEMO:
				cat = 3;
				break;
		}

		if(oldCat != cat)
		{
			filteredEntry[cat] = (TitleEntry *)entries + i;
			oldCat = cat;
		}
	}

	addToScreenLog("title database ready!");
	debugPrintf("%d titles", getTitleEntriesSize(TITLE_CATEGORY_ALL));
	return true;
}

void clearTitles()
{
	// STUB
}
