/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
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

#pragma once

#include <wut-fixups.h>

#include <stdbool.h>

#include <file.h>
#include <titles.h>
#include <tmd.h>
#include <menu/download.h>

#include <coreinit/memdefaultheap.h>
#include <wut_structsize.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct WUT_PACKED
{
	uint16_t contents; //Contents count
	uint16_t dcontent; //Actual content number
	double dlnow;
	double dltotal;
	double dltmp;
	uint32_t eta;
	size_t cs;
} downloadData;

#define DOWNLOAD_URL "http://ccs.cdn.wup.shop.nintendo.net/ccs/download/"

bool initDownloader();
void deinitDownloader();
int downloadFile(const char *url, char *file, downloadData *data, FileType type, bool resume);
bool downloadTitle(const TMD *tmd, size_t tmdSize, const TitleEntry *titleEntry, const char *titleVer, char *folderName, bool inst, NUSDEV dlDev, bool toUSB, bool keepFiles);
char *getRamBuf();
size_t getRamBufSize();
void clearRamBuf();

#ifdef __cplusplus
	}
#endif
