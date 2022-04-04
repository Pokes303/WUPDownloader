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

#ifndef NUSSPLI_FILE_H
#define NUSSPLI_FILE_H

#include <wut-fixups.h>

#include <ioThread.h>

#include <stdbool.h>
#include <stdio.h>

#define INSTALL_DIR_SD	"fs:/vol/external01/install/"
#define INSTALL_DIR_USB	"usb:/install/"
#define INSTALL_DIR_MLC	"mlc:/install/"
#define IO_BUFSIZE	(128 * 1024) // 128 KB

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum
{
	// Real file types, specify exactly one!
	FILE_TYPE_TMD = 1,		// 00000001
	FILE_TYPE_TIK = 1 << 1,		// 00000010
	FILE_TYPE_CERT = 1 << 2,	// 00000100
	FILE_TYPE_APP = 1 << 3,		// 00001000
	FILE_TYPE_H3 = 1 << 4,		// 00010000
	FILE_TYPE_JSON = 1 << 5,	// 00100000
	
	// Extra flags, OR them to the real file type.
	FILE_TYPE_TORAM = 1 << 6,	// 01000000
} FileType;

typedef enum
{
	NUSFS_ERR_NOERR,
	NUSFS_ERR_LOCKED,
	NUSFS_ERR_FULL,
	NUSFS_ERR_LIMITS,
	NUSFS_ERR_DONTEXIST
} NUSFS_ERR;

void writeVoidBytes(NUSFILE *fp, uint32_t length);
void writeCustomBytes(NUSFILE *fp, const char *str);
void writeRandomBytes(NUSFILE *fp, uint32_t length);
void writeHeader(NUSFILE *fp, FileType type);

bool fileExists(const char *path);
bool dirExists(const char *path);
void removeDirectory(const char *path);
NUSFS_ERR moveDirectory(const char *src, const char *dest);
NUSFS_ERR createDirectory(const char *path, mode_t mode);
const char *translateNusfsErr(NUSFS_ERR err);
off_t getFilesize(FILE *fp);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_FILE_H
