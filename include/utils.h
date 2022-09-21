/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
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

#include <wut-fixups.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <coreinit/mcp.h>
#include <coreinit/thread.h>

#define NAPI_URL                   "https://napi.nbg01.v10lator.de/v2/"
#define NUSSPLI_COPYRIGHT          "© 2020-2022 V10lator <v10lator@myway.de>"

#define CUSTOM_MCP_ERROR_EOM       0xDEAD0001
#define CUSTOM_MCP_ERROR_CANCELLED 0xDEAD0002

#ifdef NUSSPLI_DEBUG
#include <whb/log.h>
#include <whb/log_udp.h>
#define shutdownDebug WHBLogUdpDeinit
#else
#define debugPrintf(...)
#define checkStacks(...)
#define debugInit()
#define shutdownDebug()
#endif

#ifdef __cplusplus
#include <codecvt>

template <class I, class E, class S>
struct codecvt : std::codecvt<I, E, S>
{
    ~codecvt()
    {
    }
};

extern "C"
{
#endif

    typedef struct
    {
        bool processing;
        MCPError err;
    } McpData;

    extern int mcpHandle;

#define toLowercase(x)                      \
    for(int y = strlen(x) - 1; y >= 0; --y) \
        if(isUppercase(x[y]))               \
            x[y] += 32;

    bool isNumber(char c) __attribute__((__hot__));
    bool isLowercase(char c) __attribute__((__hot__));
    bool isUppercase(char c) __attribute__((__hot__));
    bool isAlphanumerical(char c) __attribute__((__hot__));
    bool isAllowedInFilename(char c) __attribute__((__hot__));
    bool isLowercaseHexa(char c) __attribute__((__hot__));
    bool isUppercaseHexa(char c) __attribute__((__hot__));
    bool isHexa(char c) __attribute__((__hot__));
    void hex(uint64_t i, int digits, char *out); // ex: 000050D1
    void secsToTime(uint32_t seconds, char *out);
    void getSpeedString(double bytePerSecond, char *out);
    void hexToByte(const char *hex, uint8_t *out);
    void glueMcpData(MCPInstallTitleInfo *info, McpData *data);
    void showMcpProgress(McpData *data, const char *game, const bool inst);
#ifdef NUSSPLI_DEBUG
    void debugInit();
    void debugPrintf(const char *str, ...);
    void checkStacks(const char *src);
#endif

#ifdef __cplusplus
}
#endif
