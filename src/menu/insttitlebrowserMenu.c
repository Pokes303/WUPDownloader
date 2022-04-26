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
#include <renderer.h>
#include <status.h>
#include <titles.h>
#include <utils.h>
#include <menu/insttitlebrowser.h>
#include <menu/utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

#include <mxml.h>

#define MAX_ITITLEBROWSER_LINES (MAX_LINES - 3)

static MCPTitleListType *ititleEntries;
static size_t ititleEntrySize;

typedef struct
{
	char name[MAX_TITLENAME_LENGTH];
	TITLE_REGION region;
	bool isDlc;
	bool isUpdate;
	DEVICE_TYPE dt;
} INST_META;

static INST_META getInstalledMeta(MCPTitleListType *entry)
{
	INST_META ret;

	switch(entry->indexedDevice[0])
	{
		case 'u':
			ret.dt = DEVICE_TYPE_USB;
			break;
		case 'm':
			ret.dt = DEVICE_TYPE_NAND;
			break;
		default: // TODO: bt. drh, slc
			ret.dt = DEVICE_TYPE_UNKNOWN;
	}

	TitleEntry *e = getTitleEntryByTid(entry->titleId);
	if(e)
	{
		strcpy(ret.name, e->name);
		ret.region = e->region;
		ret.isDlc = e->isDLC;
		ret.isUpdate = e->isUpdate;
		return ret;
	}

	switch(getTidHighFromTid(entry->titleId))
	{
		case TID_HIGH_UPDATE:
			ret.isDlc = false;
			ret.isUpdate = true;
			break;
		case TID_HIGH_DLC:
			ret.isDlc = true;
			ret.isUpdate = false;
			break;
		default:
			ret.isDlc = ret.isUpdate = false;
	}

	ret.region = TITLE_REGION_UNKNOWN;
	char *buf = getStaticPathBuffer(0);
	char tid[17];
	switch(ret.dt)
	{
		case DEVICE_TYPE_USB:
			mountUSB();
			strcpy(buf, "usb:/");
			break;
		case DEVICE_TYPE_NAND:
			mountMLC();
			strcpy(buf, "mlc:/");
			break;
		default: // DEVICE_TYPE_UNKNOWN
			hex(entry->titleId, 16, tid);
			strcpy(ret.name, tid);
			return ret;
	}

	strcpy(buf + 5, entry->path + 19);
	strcat(buf, "/meta/meta.xml");
	const char *name = NULL;
	mxml_node_t *xt = NULL;
	FILE *f = fopen(buf, "rb");
	buf = NULL;
	if(f != NULL)
	{
		// mxmls file parsing is slow, so we load everything to RAM
		uint32_t fs = getFilesize(f);
		buf = MEMAllocFromDefaultHeap(fs);
		if(buf != NULL)
		{
			if(fread(buf, fs, 1, f) == 1)
			{
				xt = mxmlLoadString(NULL, buf, MXML_OPAQUE_CALLBACK);
				if(xt != NULL)
				{
					mxml_node_t *xm = mxmlGetFirstChild(xt);
					if(xm != NULL)
					{
						mxml_node_t *xn = mxmlFindElement(xm, xt, "region", "type", "hexBinary", MXML_DESCEND);
						if(xn != NULL)
						{
							name = mxmlGetOpaque(xn);
							if(ret.name != NULL)
							{
								MCPRegion xr;
								hexToByte(name, (uint8_t *)&xr);
								name = NULL;
								if(xr & MCP_REGION_EUROPE)
									ret.region |= TITLE_REGION_EUR;
								if(xr & MCP_REGION_USA)
									ret.region |= TITLE_REGION_USA;
								if(xr & MCP_REGION_JAPAN)
									ret.region |= TITLE_REGION_JAP;
							}
						}

						xn = mxmlFindElement(xm, xt, "longname_en", "type", "string", MXML_DESCEND);
						if(xn != NULL)
						{
							name = mxmlGetOpaque(xn);
							if(strcmp(name, "Long Title Name (EN)") == 0)
								name = NULL;
						}
					}
				}
			}
		}

		fclose(f);
	}

	if(name == NULL)
	{
		hex(entry->titleId, 16, tid);
		name = tid;
	}

	strncpy(ret.name, name, MAX_TITLENAME_LENGTH - 1);
	ret.name[MAX_TITLENAME_LENGTH - 1] = '\0';

	if(xt)
		mxmlDelete(xt);
	if(buf)
		MEMFreeToDefaultHeap(buf);

	for(buf = ret.name; *buf != '\0'; ++buf)
		if(*buf == '\n')
			*buf = ' ';

	return ret;
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
		im = getInstalledMeta(ititleEntries + pos + i);
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
	uint32_t oldHold = 0;
	size_t frameCount = 0;
	bool dpadAction;
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
				frameCount = 30;
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
				frameCount = 30;
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
					frameCount = 30;
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
					frameCount = 30;
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
	if(!AppRunning())
	{
		MEMFreeToDefaultHeap(ititleEntries);
		return;
	}

	deinstall(*entry, false);
	MEMFreeToDefaultHeap(ititleEntries);
}
