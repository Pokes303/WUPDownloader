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

#include <input.h>
#include <renderer.h>
#include <staticMem.h>
#include <state.h>
#include <titles.h>
#include <utils.h>
#include <menu/download.h>
#include <menu/predownload.h>
#include <menu/utils.h>

#include <stdint.h>

bool downloadMenu()
{
	char titleID[17];
	char titleVer[33];
	char folderName[FILENAME_MAX - 11];
	titleID[0] = titleVer[0] = folderName[0] = '\0';
	
	if(!showKeyboard(KEYBOARD_LAYOUT_TID, KEYBOARD_TYPE_RESTRICTED, titleID, CHECK_HEXADECIMAL, 16, true, "00050000101", NULL))
		return false;
	
	if(!AppRunning())
		return true;
	
	toLowercase(titleID);
	uint64_t tid;
	hexToByte(titleID, (uint8_t *)&tid);
	
	const TitleEntry *entry = getTitleEntryByTid(tid);
	const TitleEntry e = { .name = "UNKNOWN", .tid = tid, .region = MCP_REGION_UNKNOWN, .key = 99 };
	if(entry == NULL)
		entry = &e;
	
	predownloadMenu(entry);
	return true;
}
