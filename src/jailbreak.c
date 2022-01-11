/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2022 V10lator <v10lator@myway.de>                         *
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

#ifdef NUSSPLI_HBL

#include <wut-fixups.h>

#include <stdbool.h>

#include <jailbreak.h>
#include <utils.h>

#include <coreinit/cache.h>
#include <coreinit/ios.h>
#include <coreinit/mcp.h>

#include <wut_structsize.h>

typedef struct WUT_PACKED
{
	uint32_t cmd;
	uint32_t tgt;
	uint32_t fs;
	uint32_t fo;
	char path[256];
} LOAD_REQUEST;
WUT_CHECK_OFFSET(LOAD_REQUEST, 0x00, cmd);
WUT_CHECK_OFFSET(LOAD_REQUEST, 0x04, tgt);
WUT_CHECK_OFFSET(LOAD_REQUEST, 0x08, fs);
WUT_CHECK_OFFSET(LOAD_REQUEST, 0x0C, fo);
WUT_CHECK_OFFSET(LOAD_REQUEST, 0x10, path);
WUT_CHECK_SIZE(LOAD_REQUEST, 0x110);

bool jailbreak()
{
	debugPrintf("Jailbreak: Init...");
	mcpHandle = MCP_Open();
	if(mcpHandle == 0)
	{
		debugPrintf("Error opening MCP!");
		return false;
	}

	LOAD_REQUEST request =
	{
		.cmd = 0xFC,
		.tgt = 0,
		.fs = 0,
		.fo = 0,
		.path = "wiiu/apps/NUSspli/NUSspli.rpx",
	};

	DCFlushRange(&request, sizeof(LOAD_REQUEST));
	int ret = IOS_Ioctl(mcpHandle, 100, &request, sizeof(LOAD_REQUEST), NULL, 0);
	MCP_Close(mcpHandle);

	if(ret == 0)
	{
		debugPrintf("Jailbreak: Done!");
		return true;
	}

	debugPrintf("IOCTL Error: %d", ret);
	return false;
}

#endif // ifdef NUSSPLI_HBL
