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

#ifndef NUSSPLI_CONFIG_H
#define NUSSPLI_CONFIG_H

#include <wut-fixups.h>

#include <stdbool.h>

#include <swkbd_wrapper.h>

#define CONFIG_PATH "sd:/NUSspli.txt"
#define TITLE_KEY_URL_MAX_SIZE 1024

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum
{
	regEUR = 1,
	regUSA = 1 << 1,
	regJPN = 1 << 2,
	regALL = regEUR | regUSA | regJPN,
} reg;

bool initConfig();
bool saveConfig(bool force);
bool useOnlineTitleDB();
void setUseOnlineTitleDB(bool use);
bool updateCheckEnabled();
void setUpdateCheck(bool enabled);
bool autoResumeEnabled();
void setAutoResume(bool enabled);
Swkbd_LanguageType getKeyboardLanguage();
Swkbd_LanguageType getUnfilteredLanguage();
reg getRegion();
void setRegion(reg region);
char *getFormattedRegion(reg region);
void setKeyboardLanguage(Swkbd_LanguageType language);
char *getLanguageString(Swkbd_LanguageType language);
bool dlToUSBenabled();
void setDlToUSB(bool toUSB);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_CONFIG_H
