/***************************************************************************
 * This file is part of NUSspli.                                           *
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

#ifndef NUSSPLI_OSDEFS_H
#define NUSSPLI_OSDEFS_H

#include <wut-fixups.h>

#include <stdbool.h>

#include <ft2build.h>
#include FT_TYPES_H

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct
{
	char *name;
	void *codeStart;
	uint32_t unk01;
	uint32_t codeSize;
	void *dataStart;
	uint32_t unk02;
	uint32_t dataSize;
	uint32_t unk03;
	uint32_t unk04;
	uint32_t unk05;
} RPX_Info;

extern int OSDynLoad_GetRPLInfo(uint32_t unk01, uint32_t size, RPX_Info *out);
extern bool OSGetSharedData(uint32_t a, uint32_t b, FT_Bytes *font, size_t *size);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_OSDEFS_H
