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
#include <utils.h>
#include <menu/insttitlebrowser.h>
#include <menu/utils.h>

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>

#include <mxml.h>

#define MAX_ITITLEBROWSER_LINES (MAX_LINES - 4)

static MCPTitleListType *ititleEntries;
static size_t ititleEntrySize;

static void drawITBMenuFrame(const size_t pos, const size_t cursor)
{
	startNewFrame();
	boxToFrame(1, MAX_LINES - 2);
	textToFrame(MAX_LINES - 1, ALIGNED_CENTER, "Press " BUTTON_A " to delete || " BUTTON_B " to return");
	
	size_t max = MAX_ITITLEBROWSER_LINES < ititleEntrySize ? MAX_ITITLEBROWSER_LINES : ititleEntrySize;
	size_t j, l;
	char *toFrame = getToFrameBuffer();
	TitleEntry *e;
	char tid[17];
	const char *name;
	DEVICE_TYPE dt;
	char *xmlPath = getStaticPathBuffer(0);
	FILE *f;
	uint32_t fs;
	char *buf;
	mxml_node_t *xt, *xm, *xn;
	MCPRegion xr;
	TITLE_REGION region;
	bool isDlc, isUpd;
	for(size_t i = 0; i < max; ++i)
	{
		l = i + 2;
		if(cursor == i)
			arrowToFrame(l, 1);
		
		j = i + pos;
		switch(ititleEntries[j].indexedDevice[0])
		{
			case 'u':
				dt = DEVICE_TYPE_USB;
				break;
			case 'm':
				dt = DEVICE_TYPE_NAND;
				break;
			default: // TODO: bt. drh, slc
				dt = DEVICE_TYPE_UNKNOWN;
		}

		deviceToFrame(l, 4, dt);
		buf = NULL;
		xt = NULL;

		e = getTitleEntryByTid(ititleEntries[j].titleId);
		if(e == NULL)
		{
			region = TITLE_REGION_UNKNOWN;

			switch(getTidHighFromTid(ititleEntries[j].titleId))
			{
				case TID_HIGH_UPDATE:
					isDlc = false;
					isUpd = true;
					break;
				case TID_HIGH_DLC:
					isDlc = true;
					isUpd = false;
					break;
				default:
					isDlc = isUpd = false;
			}

			switch(dt)
			{
				case DEVICE_TYPE_USB:
					mountUSB();
					strcpy(xmlPath, "usb:/");
					break;
				case DEVICE_TYPE_NAND:
					mountMLC();
					strcpy(xmlPath, "mlc:/");
					break;
				default: // DEVICE_TYPE_UNKNOWN
					hex(ititleEntries[j].titleId, 16, tid);
					name = tid;
					goto nameSet;
			}

			name = NULL;
			strcpy(xmlPath + 5, ititleEntries[j].path + 19);
			strcat(xmlPath, "/meta/meta.xml");
			f = fopen(xmlPath, "rb");
			if(f != NULL)
			{
				// mxmls file parsing is slow, so we load everything to RAM
				fs = getFilesize(f);
				buf = MEMAllocFromDefaultHeap(fs);
				if(buf != NULL)
				{
					if(fread(buf, fs, 1, f) == 1)
					{
						xt = mxmlLoadString(NULL, buf, MXML_OPAQUE_CALLBACK);
						if(xt != NULL)
						{
							xm = mxmlGetFirstChild(xt);
							if(xm != NULL)
							{
								xn = mxmlFindElement(xm, xt, "region", "type", "hexBinary", MXML_DESCEND);
								if(xn != NULL)
								{
									name = mxmlGetOpaque(xn);
									if(name != NULL)
									{
										hexToByte(name, (uint8_t *)&xr);
										name = NULL;
										if(xr & MCP_REGION_EUROPE)
											region |= TITLE_REGION_EUR;
										if(xr & MCP_REGION_USA)
											region |= TITLE_REGION_USA;
										if(xr & MCP_REGION_JAPAN)
											region |= TITLE_REGION_JAP;
									}
								}

								xn = mxmlFindElement(xm, xt, "shortname_en", "type", "string", MXML_DESCEND);
								if(xn != NULL)
									name = mxmlGetOpaque(xn);
							}
						}
					}
				}
				fclose(f);
			}

			if(name == NULL)
			{
				hex(ititleEntries[j].titleId, 16, tid);
				name = tid;
			}
		}
		else
		{
			name = e->name;
			region = e->region;
			isDlc = e->isDLC;
			isUpd = e->isUpdate;
		}

nameSet:
		if(isDlc)
			strcpy(toFrame, "[DLC] ");
		else if(isUpd)
			strcpy(toFrame, "[UPD] ");
		else
			toFrame[0] = '\0';

		strcat(toFrame, name);

		if(xt)
			mxmlDelete(xt);
		if(buf)
			MEMFreeToDefaultHeap(buf);

		textToFrameCut(l, 10, toFrame, (1280 - (FONT_SIZE << 1)) - (getSpaceWidth() * 11));
		flagToFrame(l, 7, region);

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
		
		if(vpad.trigger & VPAD_BUTTON_UP)
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
		else if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			if(cursor >= ititleEntrySize - 1 || cursor >= MAX_ITITLEBROWSER_LINES - 1)
			{
				if(mov && pos < ititleEntrySize - MAX_ITITLEBROWSER_LINES)
					++pos;
				else
					cursor = pos = 0;
			}
			else
				++cursor;
			
			redraw = true;
		}
		else if(mov)
		{
			if(vpad.trigger & VPAD_BUTTON_RIGHT)
			{
				pos += MAX_ITITLEBROWSER_LINES;
				if(pos >= ititleEntrySize - MAX_ITITLEBROWSER_LINES)
					pos = 0;
				cursor = 0;
				redraw = true;
			}
			else if(vpad.trigger & VPAD_BUTTON_LEFT)
			{
				if(pos >= MAX_ITITLEBROWSER_LINES)
					pos -= MAX_ITITLEBROWSER_LINES;
				else
					pos = ititleEntrySize - MAX_ITITLEBROWSER_LINES;
				cursor = 0;
				redraw = true;
			}
		}
		
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
