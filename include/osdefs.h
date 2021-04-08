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

#include <stdbool.h>

#include <coreinit/mcp.h>

#ifdef __cplusplus
	extern "C" {
#endif

// KPAD
extern void KPADShutdown();

// MCP
extern MCPError MCP_DeleteTitleAsync(int handle, char *path, MCPInstallTitleInfo *out);
extern bool MCP_DeleteTitleDoneAsync(int handle, bool *out);

//WPAD:
extern void WPADControlMotor(int controller, int onOff);

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_OSDEFS_H
