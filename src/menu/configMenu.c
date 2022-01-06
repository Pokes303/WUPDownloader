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
#include <input.h>
#include <renderer.h>
#include <status.h>
#include <titles.h>
#include <menu/download.h>
#include <menu/main.h>
#include <menu/utils.h>

#include <string.h>

void drawConfigMenu()
{
	startNewFrame();
	char *toScreen = getToFrameBuffer();
	int i = -1;
	strcpy(toScreen, "Press " BUTTON_A " to ");
	strcat(toScreen, useOnlineTitleDB() ? "disable" : "enable");
	strcat(toScreen, " the online title database");
	textToFrame(++i, 0, toScreen);
	
	strcpy(toScreen, "Press " BUTTON_X " to ");
	strcat(toScreen, updateCheckEnabled() ? "disable" : "enable");
	strcat(toScreen, " online updates");
	textToFrame(++i, 0, toScreen);
	
	strcpy(toScreen, "Press " BUTTON_Y " to ");
	strcat(toScreen, autoResumeEnabled() ? "disable" : "enable");
	strcat(toScreen, " auto resuming of failed downloads");
	textToFrame(++i, 0, toScreen);
	
	strcpy(toScreen, "Press LEFT/RIGHT to change the region (currently ");
	strcat(toScreen, getFormattedRegion(getRegion()));
	strcat(toScreen, ")");
	textToFrame(++i, 0, toScreen);
	
	textToFrame(i + 2, 0, "Press " BUTTON_B " to go back");
	drawFrame();
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
		
		if(vpad.trigger & VPAD_BUTTON_A)
		{
			setUseOnlineTitleDB(!useOnlineTitleDB());
			redraw = true;
		}
		if(vpad.trigger & VPAD_BUTTON_X)
		{
			setUpdateCheck(!updateCheckEnabled());
			redraw = true;
		}
		if(vpad.trigger & VPAD_BUTTON_Y)
		{
			setAutoResume(!autoResumeEnabled());
			redraw = true;
		}
		if(vpad.trigger & VPAD_BUTTON_LEFT)
		{
			char *l = getFormattedRegion(getRegion());
			if(l == "All") 
			{
				setRegion("Japan");
			} else if(l == "Japan") 
			{
				setRegion("USA");
			} else if(l == "USA") 
			{
				setRegion("Europe");
			} else if(l == "Europe") 
			{
				setRegion("All");
			}
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_RIGHT)
		{
			char *l = getFormattedRegion(getRegion());
			if(l == "All") 
			{
				setRegion("Europe");
			} else if(l == "Europe") 
			{
				setRegion("USA");
			} else if(l == "USA") 
			{
				setRegion("Japan");
			} else if(l == "Japan") 
			{
				setRegion("All");
			}
			redraw = true;
		}
		if(vpad.trigger & VPAD_BUTTON_B)
		{
			saveConfig(false);
			clearTitles();
			initTitles();
			return;
		}
		
		if(redraw)
		{
			drawConfigMenu();
			redraw = false;
		}
	}
}
