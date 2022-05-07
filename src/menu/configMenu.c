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

int cursorPos = 0;
const int entryCount = 3;

static void drawConfigMenu()
{
	startNewFrame();
	char *toScreen = getToFrameBuffer();
	int i = -1;

	strcpy(toScreen, "Online updates: ");
	strcat(toScreen, updateCheckEnabled() ? "< disabled >" : "< enabled >");
	textToFrame(++i, 4, toScreen);

	strcpy(toScreen, "Auto resume failed downloads: ");
	strcat(toScreen, autoResumeEnabled() ? "< disabled >" : "< enabled >");
	textToFrame(++i, 4, toScreen);

	strcpy(toScreen, "Notification method: ");
	strcat(toScreen, getNotificationString(getNotificationMethod()));
	textToFrame(++i, 4, toScreen);

	strcpy(toScreen, "Region: ");
	strcat(toScreen, getFormattedRegion(getRegion()));
	textToFrame(++i, 4, toScreen);

	textToFrame(i + 2, 0, "Press " BUTTON_B " to go back");

	arrowToFrame(cursorPos, 0);

	drawFrame();
}

static inline void switchNotificationMethod()
{
	if(vpad.trigger & VPAD_BUTTON_LEFT)
	{
		switch((int)getNotificationMethod())
		{
			case NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED:
				setNotificationMethod(NOTIF_METHOD_NONE);
				break;
			case NOTIF_METHOD_NONE:
				setNotificationMethod(NOTIF_METHOD_RUMBLE);
				break;
			case NOTIF_METHOD_RUMBLE:
				setNotificationMethod(NOTIF_METHOD_LED);
				break;
			case NOTIF_METHOD_LED:
				setNotificationMethod(NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED);
		}
	}
	else
	{
		switch((int)getNotificationMethod())
		{
			case NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED:
				setNotificationMethod(NOTIF_METHOD_LED);
				break;
			case NOTIF_METHOD_LED:
				setNotificationMethod(NOTIF_METHOD_RUMBLE);
				break;
			case NOTIF_METHOD_RUMBLE:
				setNotificationMethod(NOTIF_METHOD_NONE);
				break;
			case NOTIF_METHOD_NONE:
				setNotificationMethod(NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED);
		}
	}
}

static inline void switchRegion()
{
	if(vpad.trigger & VPAD_BUTTON_LEFT)
	{
		switch((int)getRegion())
		{
			case MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN:
				setRegion(MCP_REGION_JAPAN);
				break;
			case MCP_REGION_JAPAN:
				setRegion(MCP_REGION_USA);
				break;
			case MCP_REGION_USA:
				setRegion(MCP_REGION_EUROPE);
				break;
			case MCP_REGION_EUROPE:
				setRegion(MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN);
		}
	}
	else
	{
		switch((int)getRegion())
		{
			case MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN:
				setRegion(MCP_REGION_EUROPE);
				break;
			case MCP_REGION_EUROPE:
				setRegion(MCP_REGION_USA);
				break;
			case MCP_REGION_USA:
				setRegion(MCP_REGION_JAPAN);
				break;
			case MCP_REGION_JAPAN:
				setRegion(MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN);
		}
	}
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

		if(redraw)
		{
			drawConfigMenu();
			redraw = false;
		}
	}
}
