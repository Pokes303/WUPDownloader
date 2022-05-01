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

#ifndef NUSSPLI_OSDEFS_H
#define NUSSPLI_OSDEFS_H

#include <wut-fixups.h>
#include <wut_structsize.h>

#include <stdbool.h>
#include <stdint.h>

#include <coreinit/ios.h>
#include <coreinit/mcp.h>
#include <nn/acp/title.h>

#ifdef __cplusplus
	extern "C" {
#endif

// ACP
extern void ACPInitialize();
extern ACPResult ACPGetTitleMetaXmlByTitleListType(MCPTitleListType *titleListType, ACPMetaXml *out);

// MCP
extern MCPError MCP_DeleteTitleAsync(int handle, char *path, MCPInstallTitleInfo *out);
extern MCPError MCP_DeleteTitleDoneAsync(int handle, MCPInstallTitleInfo *out);

// SO
extern int somemopt (int type, void *buf, size_t bufsize, int unk);

// GX2
extern void GX2SetDRCGamma(float gamma);
extern void GX2SetDRCGammaEx(float gamma);
extern void GX2SetTVGamma(float gamma);
extern void GX2SetTVGammaEx(float gamma);

// UC - From:
// https://github.com/decaf-emu/decaf-emu/blob/master/src/libdecaf/src/cafe/libraries/coreinit/coreinit_userconfig.h
// https://github.com/decaf-emu/decaf-emu/blob/master/src/libdecaf/src/cafe/libraries/coreinit/coreinit_userconfig.cpp
typedef enum
{
	UCDataType_Undefined		= 0x00,
	UCDataType_UnsignedByte		= 0x01,
	UCDataType_UnsignedShort	= 0x02,
	UCDataType_UnsignedInt		= 0x03,
	UCDataType_SignedInt		= 0x04,
	UCDataType_Float		= 0x05,
	UCDataType_String		= 0x06,
	UCDataType_HexBinary		= 0x07,
	UCDataType_Complex		= 0x08,
	UCDataType_Invalid		= 0xFF
} UCDataType;

typedef struct WUT_PACKED
{
	char name[64];
	uint32_t access;
	UCDataType dataType;
	IOSError error;
	size_t dataSize;
	void *data;
} UCSysConfig;
WUT_CHECK_OFFSET(UCSysConfig, 0x00, name);
WUT_CHECK_OFFSET(UCSysConfig, 0x40, access);
WUT_CHECK_OFFSET(UCSysConfig, 0x44, dataType);
WUT_CHECK_OFFSET(UCSysConfig, 0x48, error);
WUT_CHECK_OFFSET(UCSysConfig, 0x4C, dataSize);
WUT_CHECK_OFFSET(UCSysConfig, 0x50, data);
WUT_CHECK_SIZE(UCSysConfig, 0x54);

extern IOSHandle UCOpen();
extern void UCClose(IOSHandle handle);
extern IOSError UCReadSysConfig(IOSHandle handle, size_t count, UCSysConfig *settings);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_OSDEFS_H
