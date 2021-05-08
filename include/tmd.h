/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2021 V10lator <v10lator@myway.de>                         *
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

#ifndef NUSSPLI_TMD_H
#define NUSSPLI_TMD_H

#include <wut-fixups.h>

#include <stdint.h>

#include <wut_structsize.h>

#ifdef __cplusplus
extern "C" {
#endif

// From: https://wiiubrew.org/wiki/Title_metadata

typedef struct WUT_PACKED {
	uint32_t cid;
	uint16_t index;
	uint16_t type;
	uint64_t size;
} TMD_CONTENT;
WUT_CHECK_OFFSET(TMD_CONTENT, 0x00, cid);
WUT_CHECK_OFFSET(TMD_CONTENT, 0x04, index);
WUT_CHECK_OFFSET(TMD_CONTENT, 0x06, type);
WUT_CHECK_OFFSET(TMD_CONTENT, 0x08, size);

typedef struct WUT_PACKED {
	WUT_UNKNOWN_BYTES(0x018C);
	uint64_t tid;
	WUT_UNKNOWN_BYTES(0x01DE - 0x018C - 8);
	uint16_t num_contents;
	WUT_UNKNOWN_BYTES(0x0B04 - 0x01E0);
	void *contents;
} TMD;
WUT_CHECK_OFFSET(TMD, 0x018C, tid);
WUT_CHECK_OFFSET(TMD, 0x01DE, num_contents);
WUT_CHECK_OFFSET(TMD, 0x0B04, contents);

TMD_CONTENT *tmdGetContent(const TMD *tmd, uint16_t i);

#ifdef __cplusplus
}
#endif

#endif // ifndef NUSSPLI_TMD_H
