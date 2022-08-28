/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2022 Xpl0itU <DaThinkingChair@protonmail.com>             *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#pragma once

#include <titles.h>
#include <tmd.h>
#include <filesystem.h>
#include <coreinit/filesystem.h>
#include <wut-fixups.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TitleData {
    TMD *tmd;
    size_t ramBufSize;
    const TitleEntry *entry;
    const char titleVer[33];
    char *folderName;
    bool inst;
    NUSDEV dlDev;
    bool toUSB;
    bool keepFiles;
} TitleData;

void addToQueue(TitleData data);
void proccessQueue();

#ifdef __cplusplus
}
#endif