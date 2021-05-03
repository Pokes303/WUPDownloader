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
#include <titleDB_json.h>
#include <titles.h>
#include <utils.h>
#include <menu/utils.h>

#include <cJSON.h>

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#define TITLE_DB NAPI_URL "titles.php"

char *titleMemArea = NULL;
char **titleNames[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
size_t titleSizes[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
size_t titleEntries = 0;
TitleEntry *titleEntry = NULL;

int transformTidHigh(TID_HIGH tidHigh)
{
	switch(tidHigh)
	{
		case TID_HIGH_GAME:
		case TID_HIGH_DLC:
		case TID_HIGH_UPDATE:
			return 0;
		case TID_HIGH_DEMO:
			return 1;
		case TID_HIGH_SYSTEM_APP:
			return 2;
		case TID_HIGH_SYSTEM_DATA:
			return 3;
		case TID_HIGH_SYSTEM_APPLET:
			return 4;
		case TID_HIGH_VWII_IOS:
			return 5;
		case TID_HIGH_VWII_SYSTEM_APP:
			return 6;
		case TID_HIGH_VWII_SYSTEM:
			return 7;
		default:
			return -1;
	}
}

TID_HIGH retransformTidHigh(int transformedTidHigh)
{
	switch(transformedTidHigh)
	{
		case 0:
			return TID_HIGH_GAME;
		case 1:
			return TID_HIGH_DEMO;
		case 2:
			return TID_HIGH_SYSTEM_APP;
		case 3:
			return TID_HIGH_SYSTEM_DATA;
		case 4:
			return TID_HIGH_SYSTEM_APPLET;
		case 5:
			return TID_HIGH_VWII_IOS;
		case 6:
			return TID_HIGH_VWII_SYSTEM_APP;
		case 7:
			return TID_HIGH_VWII_SYSTEM;
		default:
			return -1;
	}
}


TitleEntry *getTitleEntries()
{
	return titleEntry;
}

size_t getTitleEntriesSize()
{
	return titleEntries;
}

char *tid2name(const char *tid)
{
	if(titleMemArea == NULL)
		return NULL;
	
	char tidHigh[9];
	OSBlockMove(tidHigh, tid, 8, false);
	tidHigh[8] = '\0';
	
	uint32_t tl;
	uint8_t *tlp = (uint8_t *)&tl;
	hexToByte(tidHigh, tlp);
	int tt = transformTidHigh(tl);
	if(tt == -1)
		return NULL;
	
	hexToByte(tid + 8, tlp);
	tl &= 0x00FFFFFF;
	return tl > titleSizes[tt] ? NULL : titleNames[tt][tl];
}

char *name2tid(const char *name)
{
	size_t lower = 0;
	size_t upper = titleEntries;
	size_t current = upper / 2;
	int strret;
	
	while(lower != upper)
	{
		strret =  strcmp(titleEntry[current].name, name);
		if(strret == 0)
			return hex(titleEntry[current].tid, 16);
		
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
	return NULL;
}

bool initTitles()
{
	bool useOnline = useOnlineTitleDB();
	if(useOnline)
	{
		startNewFrame();
		textToFrame(0, 0, "Prepairing download");
		writeScreenLog();
		drawFrame();
		showFrame();
		
		if(downloadFile(TITLE_DB, "JSON", FILE_TYPE_JSON | FILE_TYPE_TORAM, false) != 0)
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
	
	cJSON *json = cJSON_ParseWithLength(useOnline ? ramBuf : (char *)titleDB_json, useOnline ? ramBufSize : titleDB_json_size);
	if(json == NULL)
	{
		clearRamBuf();
		debugPrintf("json == NULL");
		return false;
	}
	
	cJSON *curr[3];
	uint32_t i;
	size_t ma = 0;
	size_t size;
	char sj[9];
	sj[0] = sj[1] = '0';
	size_t j;
	titleEntries = 0;
	cJSON_ArrayForEach(curr[0], json)
	{
		i = atoi(curr[0]->string);
		if(i > 7)
			continue;
		
		cJSON_ArrayForEach(curr[1], curr[0])
		{
			size = strlen(curr[1]->string);
			if(size != 6)
				continue;
			
			curr[2] = cJSON_GetArrayItem(curr[1], 0);
			size = strlen(curr[2]->valuestring) + 1;
			if(size > 128)
			{
				debugPrintf("Too long title name detected!");
				continue;
			}
			
			j = 0;
			strcpy(&sj[2], curr[1]->string);
			hexToByte(sj, (uint8_t *)&j);
			
			if(j > titleSizes[i])
				titleSizes[i] = j;
			
			ma += size;
			titleEntries++;
		}
		
		titleNames[i] = MEMAllocFromDefaultHeap(++titleSizes[i]);
		if(titleNames[i] == NULL)
		{
			cJSON_Delete(json);
			clearRamBuf();
			clearTitles();
			return false;
		}
		
		OSBlockSet(titleNames[i], 0, titleSizes[i]);
	}
	
	titleMemArea = MEMAllocFromDefaultHeap(ma);
	if(titleMemArea == NULL)
	{
		cJSON_Delete(json);
		clearRamBuf();
		clearTitles();
		return false;
	}
	
	titleEntry = MEMAllocFromDefaultHeap(sizeof(TitleEntry) * titleEntries);
	if(titleEntry == NULL)
	{
		cJSON_Delete(json);
		clearRamBuf();
		clearTitles();
		return false;
	}
	
	char *ptr = titleMemArea;
	titleEntries = 0;
	uint64_t tid;
	char *rgn;
	bool cont;
	cJSON_ArrayForEach(curr[0], json)
	{
		i = atoi(curr[0]->string);
		if(i > 7)
			continue;
		
		cJSON_ArrayForEach(curr[1], curr[0])
		{
			size = strlen(curr[1]->string);
			if(size != 6)
				continue;
			
			curr[2] = cJSON_GetArrayItem(curr[1], 0);
			size = strlen(curr[2]->valuestring) + 1;
			if(size > 128)
				continue;
			
			strcpy(&sj[2], curr[1]->string);
			hexToByte(sj, (uint8_t *)&j);
			strcpy(ptr, curr[2]->valuestring);
//			debugPrintf("titleNames[%d][0x%08X] = %s", i, j, ptr);
			titleNames[i][j] = titleEntry[titleEntries].name = ptr;
			ptr += size;
			
			rgn = cJSON_GetArrayItem(curr[1], 1)->valuestring;
			if(strcmp(rgn, "ALL") == 0)
				titleEntry[titleEntries].region = TITLE_REGION_ALL;
			else
			{
				titleEntry[titleEntries].region = TITLE_REGION_UNKNOWN;
				
				cont = true;
				while(strlen(rgn) > 2)
				{
					if(rgn[3] != '\0')
						rgn[3] = '\0';
					else
						cont = false;
					
					if(strcmp(rgn, "EUR") == 0)
						titleEntry[titleEntries].region |= TITLE_REGION_EUR;
					if(strcmp(rgn, "USA") == 0)
						titleEntry[titleEntries].region |= TITLE_REGION_USA;
					if(strcmp(rgn, "JPN") == 0)
						titleEntry[titleEntries].region |= TITLE_REGION_JAP;
					
					if(cont)
						rgn += 4;
					else
						break;
				}
			}
			
			tid = retransformTidHigh(i);
			tid <<= 32;
			tid |= 0x0000000010000000;
			tid |= j;
			titleEntry[titleEntries++].tid = tid;
		}
	}
	
	cJSON_Delete(json);
	clearRamBuf();
	addToScreenLog("title database parsed!");
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
	titleEntries = 0;
	
	for(int i = 0; i < 8; i++)
	{
		if(titleNames[i] != NULL)
		{
			MEMFreeToDefaultHeap(titleNames[i]);
			titleNames[i] = NULL;
		}
		titleSizes[i] = 0;
	}
}
