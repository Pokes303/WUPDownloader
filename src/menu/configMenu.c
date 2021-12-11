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
//	textToFrame(0, 0, "That Title Key Site:");
//	textToFrame(1, 0, getTitleKeySite());
//	textToFrame(4, 0, "Press \uE000 to change");
	char *toScreen = getToFrameBuffer();
	int i = 0;
	strcpy(toScreen, "Press \uE000 to ");
	strcat(toScreen, useOnlineTitleDB() ? "disable" : "enable");
	strcat(toScreen, " the online title database");
	textToFrame(i++, 0, toScreen);
	
	strcpy(toScreen, "Press \uE002 to ");
	strcat(toScreen, updateCheckEnabled() ? "disable" : "enable");
	strcat(toScreen, " online updates");
	textToFrame(i++, 0, toScreen);
	
	strcpy(toScreen, "Press \uE003 to ");
	strcat(toScreen, autoResumeEnabled() ? "disable" : "enable");
	strcat(toScreen, " auto resuming of failed downloads");
	textToFrame(i++, 0, toScreen);
	/*
	strcpy(toScreen, "Press LEFT/RIGHT to change the language (curently ");
	strcat(toScreen, getLanguageString(getUnfilteredLanguage()));
	strcat(toScreen, ")");
	textToFrame(i++, 0, toScreen);
	*/
	textToFrame(++i, 0, "Press \uE001 to go back");
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
/*		if(vpad.trigger & VPAD_BUTTON_LEFT)
		{
			int l = getUnfilteredLanguage();
			if(--l < Swkbd_LanguageType__Japanese)
				l = Swkbd_LanguageType__Invalid;
			
			setKeyboardLanguage(l);
			redraw = true;
		}
		else if(vpad.trigger & VPAD_BUTTON_RIGHT)
		{
			int l = getUnfilteredLanguage();
			if(++l > Swkbd_LanguageType__Invalid)
				l = Swkbd_LanguageType__Japanese;
			
			setKeyboardLanguage(l);
			redraw = true;
		}*/
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
