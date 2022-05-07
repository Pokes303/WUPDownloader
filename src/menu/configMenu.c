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
#include <input.h>
#include <renderer.h>
#include <state.h>
#include <titles.h>
#include <menu/download.h>
#include <menu/main.h>
#include <menu/utils.h>

#include <string.h>

#include <coreinit/mcp.h>

#define ENTRY_COUNT 3

static int cursorPos = 0;

static void drawConfigMenu()
{
	startNewFrame();
	char *toScreen = getToFrameBuffer();

	strcpy(toScreen, "Online updates: ");
	strcpy(toScreen + 16, updateCheckEnabled() ? "Enabled" : "Disabled");
	textToFrame(0, 4, toScreen);

	strcpy(toScreen, "Auto resume failed downloads: ");
	strcpy(toScreen + 30, autoResumeEnabled() ? "Enabled" : "Disabled");
	textToFrame(1, 4, toScreen);

	strcpy(toScreen, "Notification method: ");
	strcpy(toScreen + 21, getNotificationString(getNotificationMethod()));
	textToFrame(2, 4, toScreen);

	strcpy(toScreen, "Region: ");
	strcpy(toScreen + 8, getFormattedRegion(getRegion()));
	textToFrame(3, 4, toScreen);
	
	lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);
	textToFrame(MAX_LINES - 1, 0, "Press " BUTTON_B " to go back");

	arrowToFrame(cursorPos, 0);

	drawFrame();
}

static inline void switchNotificationMethod()
{
	NOTIF_METHOD m;

	if(vpad.trigger & VPAD_BUTTON_LEFT)
	{
		switch((int)getNotificationMethod())
		{
			case NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED:
				m = NOTIF_METHOD_NONE;
				break;
			case NOTIF_METHOD_NONE:
				m = NOTIF_METHOD_RUMBLE;
				break;
			case NOTIF_METHOD_RUMBLE:
				m = NOTIF_METHOD_LED;
				break;
			case NOTIF_METHOD_LED:
				m = NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED;
		}
	}
	else
	{
		switch((int)getNotificationMethod())
		{
			case NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED:
				m = NOTIF_METHOD_LED;
				break;
			case NOTIF_METHOD_LED:
				m = NOTIF_METHOD_RUMBLE;
				break;
			case NOTIF_METHOD_RUMBLE:
				m = NOTIF_METHOD_NONE;
				break;
			case NOTIF_METHOD_NONE:
				m = NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED;
		}
	}

	setNotificationMethod(m);
}

static inline void switchRegion()
{
	MCPRegion reg:

	if(vpad.trigger & VPAD_BUTTON_LEFT)
	{
		switch((int)getRegion())
		{
			case MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN:
				reg = MCP_REGION_JAPAN;
				break;
			case MCP_REGION_JAPAN:
				reg = MCP_REGION_USA;
				break;
			case MCP_REGION_USA:
				reg = MCP_REGION_EUROPE;
				break;
			case MCP_REGION_EUROPE:
				reg = MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN);
		}
	}
	else
	{
		switch((int)getRegion())
		{
			case MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN:
				reg = MCP_REGION_EUROPE;
				break;
			case MCP_REGION_EUROPE:
				reg = MCP_REGION_USA;
				break;
			case MCP_REGION_USA:
				reg = MCP_REGION_JAPAN;
				break;
			case MCP_REGION_JAPAN:
				reg = MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN;
		}
	}

	setRegion(reg);
}

void configMenu()
{
	drawConfigMenu();
	
	bool redraw = false;
	while(AppRunning())
	{
		if(app == APP_STATE_BACKGROUND)
			continue;
		if(app == APP_STATE_RETURNING)
			drawConfigMenu();
		
		showFrame();
		if(vpad.trigger & VPAD_BUTTON_B)
		{
			saveConfig(false);
			return;
		}

		if(vpad.trigger & (VPAD_BUTTON_RIGHT | VPAD_BUTTON_LEFT | VPAD_BUTTON_A))
		{
			switch(cursorPos)
			{
				case 0:
					setUpdateCheck(!updateCheckEnabled());
					break;
				case 1:
					setAutoResume(!autoResumeEnabled());
					break;
				case 2:
					switchNotificationMethod();
					break;
				case 3:
					switchRegion();
					break;
			}

			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_UP)
		{
			--cursorPos;
			if(cursorPos < 0)
				cursorPos = ENTRY_COUNT;
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_DOWN)
		{
			++cursorPos;
			if(cursorPos > ENTRY_COUNT)
				cursorPos = 0;
			redraw = true;
		}

		if(redraw)
		{
			drawConfigMenu();
			redraw = false;
		}
	}
}
