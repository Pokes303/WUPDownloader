/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
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

#include <stdarg.h>
#include <string.h>

#include <coreinit/memdefaultheap.h>

#include <renderer.h>
#include <utils.h>
#include <menu/utils.h>

struct DownloadLogList;
typedef struct DownloadLogList DownloadLogList;
struct DownloadLogList
{
	char *line;
	DownloadLogList *nextEntry;
};

DownloadLogList *downloadLogList = NULL;
char toFrameBuffer[TO_FRAME_BUFFER_SIZE];

void addToScreenLog(const char *str, ...)
{
	va_list va;
	va_start(va, str);
	char newStr[MAX_CHARS];
	vsnprintf(newStr, MAX_CHARS, str, va);
	va_end(va);
	debugPrintf(newStr);
	
	DownloadLogList *newEntry = MEMAllocFromDefaultHeap(sizeof(DownloadLogList));
	if(newEntry ==  NULL)
		return;
	
	//TODO: We copy the string here for fast porting purposes
	newEntry->line = MEMAllocFromDefaultHeap(sizeof(char) * (strlen(newStr) + 1));
	if(newEntry->line == NULL)
	{
		MEMFreeToDefaultHeap(newEntry);
		return;
	}
	strcpy(newEntry->line, newStr);
	
	newEntry->nextEntry = NULL;
	
	if(downloadLogList == NULL)
	{
		downloadLogList = newEntry;
		return;
	}
	
	DownloadLogList *last;
	int i = 0;
	for(DownloadLogList *c = downloadLogList; c != NULL; c = c->nextEntry)
	{
		
		i++;
		last = c;
	}
	if(i == MAX_LINES - 3)
	{
		DownloadLogList *tmpList = downloadLogList;
		downloadLogList = tmpList->nextEntry;
		MEMFreeToDefaultHeap(tmpList->line);
		MEMFreeToDefaultHeap(tmpList);
	}
	
	last->nextEntry = newEntry;
}

void clearScreenLog()
{
	DownloadLogList *tmpList;
	while(downloadLogList != NULL)
	{
		tmpList = downloadLogList;
		downloadLogList = tmpList->nextEntry;
		MEMFreeToDefaultHeap(tmpList->line);
		MEMFreeToDefaultHeap(tmpList);
	}
}

void writeScreenLog()
{
	lineToFrame(2, SCREEN_COLOR_WHITE);
	int i = 3;
	for(DownloadLogList *entry = downloadLogList; entry != NULL; entry = entry->nextEntry)
		textToFrame(i++, 0, entry->line);
}

void drawErrorFrame(const char *text, ErrorOptions option)
{
	debugPrintf("Error frame");
	colorStartNewFrame(SCREEN_COLOR_RED);
	
	size_t size;
	int line = 0;
	for(; text != NULL; line++)
	{
		const char *l = strchr(text, '\n');
		size = l == NULL ? strlen(text) : (l - text);
		if(size == 0)
			continue;
		
		char tmp[size + 1];
		for(int i = 0; i < size; i++)
			tmp[i] = text[i];
		
		tmp[size] = '\0';
		textToFrame(line, 0, tmp);
		
		text = l == NULL ? NULL : (l + 1);
	}
	
	line = MAX_LINES - 1;
	if((option & B_RETURN) == B_RETURN)
		textToFrame(line--, 0, "Press \uE001 to return");
	if((option & Y_RETRY) == Y_RETRY)
		textToFrame(line--, 0, "Press \uE003 to retry");
	if((option & A_CONTINUE) == A_CONTINUE)
		textToFrame(line--, 0, "Press \uE000 to continue");
	lineToFrame(line, SCREEN_COLOR_WHITE);
	
	drawFrame();
}

char *getToFrameBuffer()
{
	return toFrameBuffer;
}
