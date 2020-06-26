/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020 V10lator <v10lator@myway.de>                         *
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

#ifndef NUSSPLI_DOWNLOADER_H
#define NUSSPLI_DOWNLOADER_H

#include <wut-fixups.h>

#include <stdbool.h>

#include <file.h>
#include <menu/download.h>

#ifdef __cplusplus
	extern "C" {
#endif

extern uint16_t contents;
extern char *ramBuf;
extern size_t ramBufSize;

#define DOWNLOAD_URL "http://ccs.cdn.wup.shop.nintendo.net/ccs/download/"

int downloadFile(const char *url, char *file, FileType type);
bool downloadTitle(const char *tid, const char *titleVer, char *folderName, bool inst, bool dlToUSB, bool toUSB, bool keepFiles);
void clearRamBuf();

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_DOWNLOADER_H
