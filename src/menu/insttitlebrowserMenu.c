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

#include <wut-fixups.h>

#include <deinstaller.h>
#include <file.h>
#include <filesystem.h>
#include <input.h>
#include <osdefs.h>
#include <renderer.h>
#include <state.h>
#include <staticMem.h>
#include <titles.h>
#include <utils.h>
#include <menu/insttitlebrowser.h>
#include <menu/utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <nn/acp/title.h>

#define MAX_ITITLEBROWSER_LINES			(MAX_LINES - 3)
#define MAX_ITITLEBROWSER_TITLE_LENGTH	(MAX_TITLENAME_LENGTH >> 1)
#define DPAD_COOLDOWN_FRAMES			30 // half a second at 60 FPS

static MCPTitleListType *ititleEntries;
static size_t ititleEntrySize;

typedef struct
{
	char name[MAX_ITITLEBROWSER_TITLE_LENGTH];
	MCPRegion region;
	bool isDlc;
	bool isUpdate;
	DEVICE_TYPE dt;
} INST_META;

static void getInstalledMeta(MCPTitleListType *entry, INST_META *out)
{
	switch(entry->indexedDevice[0])
	{
		case 'u':
			out->dt = DEVICE_TYPE_USB;
			break;
		case 'm':
			out->dt = DEVICE_TYPE_NAND;
			break;
		default: // TODO: bt. drh, slc
			out->dt = DEVICE_TYPE_UNKNOWN;
	}

	const TitleEntry *e = getTitleEntryByTid(entry->titleId);
	if(e)
	{
		strncpy(out->name, e->name, MAX_ITITLEBROWSER_TITLE_LENGTH - 1);
		out->name[MAX_ITITLEBROWSER_TITLE_LENGTH - 1] = '\0';

		out->region = e->region;
		out->isDlc = isDLC(e);
		out->isUpdate = isUpdate(e);
		return;
	}

	switch(getTidHighFromTid(entry->titleId))
	{
		case TID_HIGH_UPDATE:
			out->isDlc = false;
			out->isUpdate = true;
			break;
		case TID_HIGH_DLC:
			out->isDlc = true;
			out->isUpdate = false;
			break;
		default:
			out->isDlc = out->isUpdate = false;
	}

	ACPMetaXml *meta = getStaticMetaXmlBuffer();
	if(ACPGetTitleMetaXmlByTitleListType(entry, meta) == ACP_RESULT_SUCCESS)
	{
		strncpy(out->name, meta->longname_en, MAX_ITITLEBROWSER_TITLE_LENGTH - 1);
		out->name[MAX_ITITLEBROWSER_TITLE_LENGTH - 1] = '\0';

		if(strcmp(out->name, "Long Title Name (EN)"))
		{
			for(char *buf = out->name; *buf != '\0'; ++buf)
				if(*buf == '\n')
					*buf = ' ';

			out->region = meta->region;
			return;
		}
	}
	char tid[17];
	hex(entry->titleId, 16, tid);
	strcpy(out->name, tid);
	out->region = MCP_REGION_UNKNOWN;
}

static void drawITBMenuFrame(const size_t pos, const size_t cursor)
{
	startNewFrame();
	boxToFrame(0, MAX_LINES - 2);
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, "Press " BUTTON_A " to delete || " BUTTON_B " to return");
	
	size_t max = ititleEntrySize - pos;
	if(max > MAX_ITITLEBROWSER_LINES)
		max = MAX_ITITLEBROWSER_LINES;

	INST_META im;
	char *toFrame = getToFrameBuffer();
	for(size_t i = 0, l = 1; i < max; ++i, ++l)
	{
		getInstalledMeta(ititleEntries + pos + i, &im);
		if(im.isDlc)
			strcpy(toFrame, "[DLC] ");
		else if(im.isUpdate)
			strcpy(toFrame, "[UPD] ");
		else
			toFrame[0] = '\0';

		if(cursor == i)
			arrowToFrame(l, 1);

		deviceToFrame(l, 4, im.dt);
		flagToFrame(l, 7, im.region);
		strcat(toFrame, im.name);
		textToFrameCut(l, 10, toFrame, (1280 - (FONT_SIZE << 1)) - (getSpaceWidth() * 11));
	}

	drawFrame();
}

void ititleBrowserMenu()
{
	int32_t r = MCP_TitleCount(mcpHandle);
	if(r < 0)
	{
		// TODO
		return;
	}

	uint32_t s = sizeof(MCPTitleListType) * (uint32_t)r;
	ititleEntries = (MCPTitleListType *)MEMAllocFromDefaultHeap(s);
	if(ititleEntries == NULL)
	{
		debugPrintf("Insttitlebrowser: OUT OF MEMORY!");
		return;
	}

	r = MCP_TitleList(mcpHandle, &s, ititleEntries, s);
	if(r < 0)
	{
		// TODO
		MEMFreeToDefaultHeap(ititleEntries);
		return;
	}

	ititleEntrySize = s;
	size_t cursor = 0;
	size_t pos = 0;
	
	drawITBMenuFrame(pos, cursor);
	
	bool mov = ititleEntrySize >= MAX_ITITLEBROWSER_LINES;
	bool redraw = false;
	MCPTitleListType *entry;
	uint32_t oldHold = VPAD_BUTTON_RIGHT;
	size_t frameCount = DPAD_COOLDOWN_FRAMES;
	bool dpadAction;

loopEntry:
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawITBMenuFrame(pos, cursor);
		
		showFrame();
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			entry = ititleEntries + cursor + pos;
			break;
		}
		
		if(vpad.trigger & VPAD_BUTTON_B)
		{
			MEMFreeToDefaultHeap(ititleEntries);
			return;
		}

		if(vpad.hold & VPAD_BUTTON_UP)
		{
			if(oldHold != VPAD_BUTTON_UP)
			{
				oldHold = VPAD_BUTTON_UP;
				frameCount = DPAD_COOLDOWN_FRAMES;
				dpadAction = true;
			}
			else if(frameCount == 0)
				dpadAction = true;
			else
			{
				--frameCount;
				dpadAction = false;
			}

			if(dpadAction)
			{
				if(cursor)
					cursor--;
				else
				{
					if(mov)
					{
						if(pos)
							pos--;
						else
						{
							cursor = MAX_ITITLEBROWSER_LINES - 1;
							pos = ititleEntrySize - MAX_ITITLEBROWSER_LINES;
						}
					}
					else
						cursor = ititleEntrySize - 1;
				}

				redraw = true;
			}
		}
		else if(vpad.hold & VPAD_BUTTON_DOWN)
		{
			if(oldHold != VPAD_BUTTON_DOWN)
			{
				oldHold = VPAD_BUTTON_DOWN;
				frameCount = DPAD_COOLDOWN_FRAMES;
				dpadAction = true;
			}
			else if(frameCount == 0)
				dpadAction = true;
			else
			{
				--frameCount;
				dpadAction = false;
			}

			if(dpadAction)
			{
				if(cursor + pos >= ititleEntrySize - 1 || cursor >= MAX_ITITLEBROWSER_LINES - 1)
				{
					if(!mov || ++pos + cursor >= ititleEntrySize)
						cursor = pos = 0;
				}
				else
					++cursor;

				redraw = true;
			}
		}
		else if(mov)
		{
			if(vpad.hold & VPAD_BUTTON_RIGHT)
			{
				if(oldHold != VPAD_BUTTON_RIGHT)
				{
					oldHold = VPAD_BUTTON_RIGHT;
					frameCount = DPAD_COOLDOWN_FRAMES;
					dpadAction = true;
				}
				else if(frameCount == 0)
					dpadAction = true;
				else
				{
					--frameCount;
					dpadAction = false;
				}

				if(dpadAction)
				{
					pos += MAX_ITITLEBROWSER_LINES;
					if(pos >= ititleEntrySize)
						pos = 0;
					cursor = 0;
					redraw = true;
				}
			}
			else if(vpad.hold & VPAD_BUTTON_LEFT)
			{
				if(oldHold != VPAD_BUTTON_LEFT)
				{
					oldHold = VPAD_BUTTON_LEFT;
					frameCount = DPAD_COOLDOWN_FRAMES;
					dpadAction = true;
				}
				else if(frameCount == 0)
					dpadAction = true;
				else
				{
					--frameCount;
					dpadAction = false;
				}

				if(dpadAction)
				{
					if(pos >= MAX_ITITLEBROWSER_LINES)
						pos -= MAX_ITITLEBROWSER_LINES;
					else
						pos = ititleEntrySize - MAX_ITITLEBROWSER_LINES;
					cursor = 0;
					redraw = true;
				}
			}
		}

		if(oldHold && !(vpad.hold & (VPAD_BUTTON_UP | VPAD_BUTTON_DOWN | VPAD_BUTTON_LEFT | VPAD_BUTTON_RIGHT)))
			oldHold = 0;
		
		if(redraw)
		{
			drawITBMenuFrame(pos, cursor);
			mov = ititleEntrySize > MAX_ITITLEBROWSER_LINES;
			redraw = false;
		}
	}

	if(AppRunning())
	{
		INST_META im;
		getInstalledMeta(entry, &im);
		char *toFrame = getToFrameBuffer();
		strcpy(toFrame, "Do you really want to uninstall\n");
		strcat(toFrame, im.name);
		strcat(toFrame, "\nfrom your ");
		strcat(toFrame, im.dt == DEVICE_TYPE_USB ? "USB" : im.dt == DEVICE_TYPE_NAND ? "NAND" : "unknown");
		strcat(toFrame, " drive?\n\n" BUTTON_A " Yes || " BUTTON_B " No");
		r = addErrorOverlay(toFrame);

		while(AppRunning())
		{
			showFrame();

			if(vpad.trigger & VPAD_BUTTON_B)
			{
				removeErrorOverlay(r);
				goto loopEntry;
			}
			if(vpad.trigger & VPAD_BUTTON_A)
				break;
		}

		removeErrorOverlay(r);

		if(checkSystemTitle(entry->titleId, im.region) && AppRunning())
			deinstall(entry, im.name, false);
		else
			goto loopEntry;
	}

	MEMFreeToDefaultHeap(ititleEntries);
}
