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

#include <wut-fixups.h>

#include <config.h>
#include <downloader.h>
#include <renderer.h>
#include <romfs.h>
#include <titles.h>
#include <utils.h>
#include <menu/utils.h>

#include <cJSON.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#define TITLE_DB NAPI_URL "t"

char *titleMemArea = NULL;
size_t titleEntries[5] = { 0, 0, 0, 0, 0 };
TitleEntry *titleEntry = NULL;
TitleEntry *filteredEntry[4] = { NULL, NULL, NULL, NULL }; // Games, Updates, DLC, Demos

TRANSFORMED_TID_HIGH transformTidHigh(TID_HIGH tidHigh)
{
	switch(tidHigh)
	{
		case TID_HIGH_GAME:
			return TRANSFORMED_TID_HIGH_GAME;
		case TID_HIGH_DEMO:
			return TRANSFORMED_TID_HIGH_DEMO;
		case TID_HIGH_SYSTEM_APP:
			return TRANSFORMED_TID_HIGH_SYSTEM_APP;
		case TID_HIGH_SYSTEM_DATA:
			return TRANSFORMED_TID_HIGH_SYSTEM_DATA;
		case TID_HIGH_SYSTEM_APPLET:
			return TRANSFORMED_TID_HIGH_SYSTEM_APPLET;
		case TID_HIGH_VWII_IOS:
			return TRANSFORMED_TID_HIGH_VWII_IOS;
		case TID_HIGH_VWII_SYSTEM_APP:
			return TRANSFORMED_TID_HIGH_VWII_SYSTEM_APP;
		case TID_HIGH_VWII_SYSTEM:
			return TRANSFORMED_TID_HIGH_VWII_SYSTEM;
		case TID_HIGH_DLC:
			return TRANSFORMED_TID_HIGH_DLC;
		case TID_HIGH_UPDATE:
			return TRANSFORMED_TID_HIGH_UPDATE;
		default:
			return -1;
	}
}

TID_HIGH retransformTidHigh(TRANSFORMED_TID_HIGH transformedTidHigh)
{
	switch(transformedTidHigh)
	{
		case TRANSFORMED_TID_HIGH_GAME:
			return TID_HIGH_GAME;
		case TRANSFORMED_TID_HIGH_DEMO:
			return TID_HIGH_DEMO;
		case TRANSFORMED_TID_HIGH_SYSTEM_APP:
			return TID_HIGH_SYSTEM_APP;
		case TRANSFORMED_TID_HIGH_SYSTEM_DATA:
			return TID_HIGH_SYSTEM_DATA;
		case TRANSFORMED_TID_HIGH_SYSTEM_APPLET:
			return TID_HIGH_SYSTEM_APPLET;
		case TRANSFORMED_TID_HIGH_VWII_IOS:
			return TID_HIGH_VWII_IOS;
		case TRANSFORMED_TID_HIGH_VWII_SYSTEM_APP:
			return TID_HIGH_VWII_SYSTEM_APP;
		case TRANSFORMED_TID_HIGH_VWII_SYSTEM:
			return TID_HIGH_VWII_SYSTEM;
		case TRANSFORMED_TID_HIGH_DLC:
			return TID_HIGH_DLC;
		case TRANSFORMED_TID_HIGH_UPDATE:
			return TID_HIGH_UPDATE;
		default:
			return -1;
	}
}


TitleEntry *getTitleEntries(TITLE_CATEGORY cat)
{
	return cat == TITLE_CATEGORY_ALL ? titleEntry : filteredEntry[cat];
	
}

size_t getTitleEntriesSize(TITLE_CATEGORY cat)
{
	return cat == TITLE_CATEGORY_ALL ?
		titleEntries[0] + titleEntries[1] + titleEntries[2] + titleEntries[3] + titleEntries[4] :
		titleEntries[cat];
}

TitleEntry *getTitleEntryByTid(uint64_t tid)
{
	TitleEntry *haystack;
	size_t haySize;
	
	switch((TID_HIGH)(tid >> 32))
	{
		case TID_HIGH_GAME:
			haystack = filteredEntry[TITLE_CATEGORY_GAME];
			haySize = titleEntries[TITLE_CATEGORY_GAME];
			break;
		case TID_HIGH_UPDATE:
			haystack = filteredEntry[TITLE_CATEGORY_UPDATE];
			haySize = titleEntries[TITLE_CATEGORY_UPDATE];
			break;
		case TID_HIGH_DLC:
			haystack = filteredEntry[TITLE_CATEGORY_DLC];
			haySize = titleEntries[TITLE_CATEGORY_DLC];
			break;
		case TID_HIGH_DEMO:
			haystack = filteredEntry[TITLE_CATEGORY_DEMO];
			haySize = titleEntries[TITLE_CATEGORY_DEMO];
			break;
		default:
			haystack = titleEntry;
			haySize = titleEntries[TITLE_CATEGORY_ALL];
		
	}
	
	for(size_t i = 0; i < haySize; i++, haystack++)
		if(haystack->tid == tid)
			return haystack;
	
	return NULL;
}

char *tid2name(const char *tid)
{
	uint64_t rtid;
	hexToByte(tid, (uint8_t *)&rtid);
	TitleEntry *e = getTitleEntryByTid(rtid);
	return e == NULL ? "UNKNOWN" : e->name;
}

bool name2tid(const char *name, char *out)
{
	size_t lower = 0;
	size_t upper = getTitleEntriesSize(TITLE_CATEGORY_ALL);
	size_t current = upper / 2;
	int strret;
	
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
	bool useOnline = useOnlineTitleDB();
	if(useOnline)
	{
		startNewFrame();
		textToFrame(0, 0, "Preparing download");
		writeScreenLog();
		drawFrame();
		showFrame();
		
		if(downloadFile(TITLE_DB, "JSON", NULL, FILE_TYPE_JSON | FILE_TYPE_TORAM, false) != 0)
		{
			clearRamBuf();
			debugPrintf("Error downloading %s", TITLE_DB);
			useOnline = false;
		}
	}
	
	startNewFrame();
	textToFrame(0, 0, "Parsing JSON");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	FILE *f = fopen(ROMFS_PATH "titleDB.json", "rb"); //TODO: Error handling...
	size_t fileSize = getFilesize(f);
	void *raw = MEMAllocFromDefaultHeap(fileSize);
	fread(raw, 1, fileSize, f); //TODO: Error handling...
	fclose(f);
	cJSON *json = cJSON_ParseWithLength(useOnline ? ramBuf : (char *)raw, useOnline ? ramBufSize : fileSize);
	if(json == NULL)
	{
		clearRamBuf();
		MEMFreeToDefaultHeap(raw);
		debugPrintf("json == NULL");
		return false;
	}
	
	cJSON *curr[3];
	TRANSFORMED_TID_HIGH i;
	size_t ma = 0;
	size_t size;
	size_t entries = 0;
	cJSON_ArrayForEach(curr[0], json)
	{
		if(curr[0]->string[0] == 's')
			continue; // TODO
		
		i = atoi(curr[0]->string);
		if(i > TRANSFORMED_TID_HIGH_UPDATE)
			continue;
		
		cJSON_ArrayForEach(curr[1], curr[0])
		{
			size = strlen(curr[1]->string);
			if(size != 7)
				continue;
			
			curr[2] = cJSON_GetArrayItem(curr[1], 0);
			size = strlen(curr[2]->valuestring) + 1;
			if(size > 256)
			{
				debugPrintf("Too long title name detected: %s", curr[2]->valuestring);
				continue;
			}
			
			ma += size;
			entries++;
		}
	}
	
	titleMemArea = MEMAllocFromDefaultHeap(ma);
	if(titleMemArea == NULL)
	{
		cJSON_Delete(json);
		clearRamBuf();
		clearTitles();
		MEMFreeToDefaultHeap(raw);
		return false;
	}
	
	titleEntry = MEMAllocFromDefaultHeap(sizeof(TitleEntry) * entries);
	if(titleEntry == NULL)
	{
		cJSON_Delete(json);
		clearRamBuf();
		clearTitles();
		MEMFreeToDefaultHeap(raw);
		return false;
	}
	
	char *ptr = titleMemArea;
	entries = 0;
	uint64_t tid;
	uint32_t cat;
	uint32_t j;
	char sj[32];
	sj[0] = '0';
	char *sjm = sj + 1;
	cJSON_ArrayForEach(curr[0], json)
	{
		if(curr[0]->string[0] == 's')
			continue; // TODO
		
		i = atoi(curr[0]->string);
		if(i > TRANSFORMED_TID_HIGH_UPDATE)
			continue;
		
		switch(i)
		{
			case TRANSFORMED_TID_HIGH_GAME:
				cat = 0;
				break;
			case TRANSFORMED_TID_HIGH_UPDATE:
				cat = 1;
				break;
			case TRANSFORMED_TID_HIGH_DLC:
				cat = 2;
				break;
			case TRANSFORMED_TID_HIGH_DEMO:
				cat = 3;
				break;
			default:
				cat = 4;
				break;
		}
		
		if(cat != 4)
			filteredEntry[cat] = titleEntry + entries;
		
		cJSON_ArrayForEach(curr[1], curr[0])
		{
			size = strlen(curr[1]->string);
			if(size != 7)
				continue;
			
			curr[2] = cJSON_GetArrayItem(curr[1], 0);
			size = strlen(curr[2]->valuestring) + 1;
			if(size > 256)
				continue;
			
			strcpy(sjm, curr[1]->string);
			hexToByte(sj, (uint8_t *)&j);
			
			strcpy(ptr, curr[2]->valuestring);
//			debugPrintf("titleNames[%d][0x%08X] = %s", i, j, ptr);
			titleEntry[entries].name = ptr;
			ptr += size;
			
			titleEntry[entries].region = cJSON_GetArrayItem(curr[1], 1)->valueint;
			titleEntry[entries].isDLC = i == TRANSFORMED_TID_HIGH_DLC;
			titleEntry[entries].isUpdate = i == TRANSFORMED_TID_HIGH_UPDATE;
			titleEntry[entries].key = cJSON_GetArrayItem(curr[1], 2)->valueint;
			
			tid = retransformTidHigh(i);
			tid <<= 32;
			tid |= 0x0000000010000000;
			tid |= j;
//			debugPrintf("%s / 0x%016X / 0x%016X", sj, j, tid);
			titleEntry[entries++].tid = tid;
			titleEntries[cat]++;
		}
	}
	
	cJSON_Delete(json);
	clearRamBuf();
	MEMFreeToDefaultHeap(raw);
	addToScreenLog("title database parsed!");
	debugPrintf("%d titles parsed", getTitleEntriesSize(TITLE_CATEGORY_ALL));
	return true;
}

void clearTitles()
{	
	if(titleMemArea != NULL)
	{
		MEMFreeToDefaultHeap(titleMemArea);
		titleMemArea = NULL;
	}
	
	if(titleEntry != NULL)
	{
		MEMFreeToDefaultHeap(titleEntry);
		titleEntry = NULL;
	}
	
	for(int i = 0; i < 5; i++)
		titleEntries[i] = 0;
	
	for(int i = 0; i < 4; i++)
		filteredEntry[i] = NULL;
}
