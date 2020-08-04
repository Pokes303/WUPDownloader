/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020 V10lator <v10lator@myway.de>                         *
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
#include <titles.h>
#include <utils.h>
#include <menu/utils.h>

#include <cJSON.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#define TITLE_DB "http://napi.nbg01.v10lator.de"

char *titleMemArea = NULL;
char **titleNames[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

int transformTidHigh(uint32_t tidHigh)
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
	return titleNames[tt][tl];
}

bool initTitles()
{
	if(!useOnlineTitleDB())
		return true;
	
	startNewFrame();
	textToFrame(0, 0, "Prepairing download");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	if(downloadFile(TITLE_DB, "JSON", FILE_TYPE_JSON | FILE_TYPE_TORAM) != 0)
	{
		clearRamBuf();
		debugPrintf("Error downloading %s", TITLE_DB);
		return false;
	}
	
	startNewFrame();
	textToFrame(0, 0, "Parsing JSON");
	writeScreenLog();
	drawFrame();
	showFrame();
	
	cJSON *json = cJSON_ParseWithLength(ramBuf, ramBufSize);
	if(json == NULL)
	{
		clearRamBuf();
		debugPrintf("json == NULL");
		return false;
	}
	
	cJSON *curr[2];
	int i;
	size_t ma = 0;
	size_t size;
	char sj[9];
	sj[0] = sj[1] = '0';
	uint32_t j;
	uint32_t maxJ;
	cJSON_ArrayForEach(curr[0], json)
	{
		i = atoi(curr[0]->string);
		if(i < 0 || i > 7)
			continue;
		
		maxJ = 0;
		cJSON_ArrayForEach(curr[1], curr[0])
		{
			size = strlen(curr[1]->string);
			if(size != 6)
				continue;
			
			size = strlen(curr[1]->valuestring) + 1;
			if(size > 128)
			{
				debugPrintf("Too long title name detected!");
				continue;
			}
			
			j = 0;
			strcpy(&sj[2], curr[1]->string);
			hexToByte(sj, (uint8_t *)&j);
			
			if(j > maxJ)
				maxJ = j;
			
			ma += size;
		}
		
		titleNames[i] = MEMAllocFromDefaultHeap(++maxJ);
		if(titleNames[i] == NULL)
		{
			cJSON_Delete(json);
			clearRamBuf();
			clearTitles();
			return false;
		}
		
		OSBlockSet(titleNames[i], 0, maxJ);
	}
	
	titleMemArea = MEMAllocFromDefaultHeap(ma);
	if(titleMemArea == NULL)
	{
		cJSON_Delete(json);
		clearRamBuf();
		clearTitles();
		return false;
	}
	
	char *ptr = titleMemArea;
	cJSON_ArrayForEach(curr[0], json)
	{
		i = atoi(curr[0]->string);
		if(i < 0 || i > 7)
			continue;
		
		cJSON_ArrayForEach(curr[1], curr[0])
		{
			size = strlen(curr[1]->string);
			if(size != 6)
				continue;
			
			size = strlen(curr[1]->valuestring) + 1;
			if(size > 128)
				continue;
			
			j = 0;
			strcpy(&sj[2], curr[1]->string);
			hexToByte(sj, (uint8_t *)&j);
			strcpy(ptr, curr[1]->valuestring);
//			debugPrintf("titleNames[%d][0x%08X] = %s", i, j, ptr);
			titleNames[i][j] = ptr;
			ptr += size;
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
	
	for(int i = 0; i < 8; i++)
		if(titleNames[i] != NULL)
		{
			MEMFreeToDefaultHeap(titleNames[i]);
			titleNames[i] = NULL;
		}
}
