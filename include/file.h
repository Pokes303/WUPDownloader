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

#ifndef NUSSPLI_FILE_H
#define NUSSPLI_FILE_H

#include <wut-fixups.h>

#include <stdbool.h>
#include <stdio.h>

#define INSTALL_DIR_SD "/vol/external01/install/"
#define INSTALL_DIR_USB "usb:/install/"

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum
{
	// Real file types, specify exactly one!
	FILE_TYPE_TMD = 1,		// 00000001
	FILE_TYPE_TIK = 2,		// 00000010
	FILE_TYPE_CERT = 4,		// 00000100
	FILE_TYPE_APP = 8,		// 00001000
	FILE_TYPE_H3 = 16,		// 00010000
	FILE_TYPE_JSON = 32,	// 00100000
	
	// Extra flags, OR them to the real file type (except FILE_TYPE_TODISC which exists for beauty reasons only / is the default)
	FILE_TYPE_TODISC = 0,	// 00000000
	FILE_TYPE_TORAM = 64,	// 01000000
} FileType;

uint8_t readUInt8(const char *file, uint32_t pos);
uint16_t readUInt16(const char *file, uint32_t pos);
uint32_t readUInt32(const char *file, uint32_t pos);
uint64_t readUInt64(const char *file, uint32_t pos);

void writeVoidBytes(FILE *fp, uint32_t length);
uint8_t charToByte(char c);
void writeCustomBytes(FILE *fp, const char *str);
void writeRandomBytes(FILE *fp, uint32_t length);
void writeHeader(FILE *fp, FileType type);

bool fileExists(const char *path);
bool dirExists(const char *path);
void removeDirectory(const char *path);
long getFilesize(FILE *fp);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_FILE_H
