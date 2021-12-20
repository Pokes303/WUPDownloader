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

#ifndef NUSSPLI_UTILS_H
#define NUSSPLI_UTILS_H

#include <wut-fixups.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <uchar.h>

#include <coreinit/mcp.h>
#include <coreinit/thread.h>

#define NAPI_URL		"https://napi.nbg01.v10lator.de/v2/"
#define NUSSPLI_COPYRIGHT	"© 2020-2021 V10lator <v10lator@myway.de>"

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
	
	template<class I, class E, class S>
	struct codecvt : std::codecvt<I, E, S>
	{
		~codecvt()
		{}
	};

	extern "C" {
#endif

typedef struct
{
	bool processing;
	MCPError err;
} McpData;

/*
 * Default Wii U thread priority is 16.
 * SDL changes its audio thread priority one lower, so to 15.
 * We want the SDL audio thread to be at THREAD_PRIORITY_LOW,
 * so that has to be 15, too
 */
typedef enum
{
    THREAD_PRIORITY_HIGH = 13,
    THREAD_PRIORITY_MEDIUM = 14,
    THREAD_PRIORITY_LOW = 15,
} THREAD_PRIORITY;

extern int mcpHandle;

#define isNumber(x) (x >= '0' && x <= '9')

#define isLowercase(x) (x >= 'a' && x <= 'z')
#define isUppercase(x) (x >= 'A' && x <= 'Z')
#define isAlphanumerical(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || isNumber(x))
// Keep it to ASCII for FTPiiU compat.
#define isAllowedInFilename(x) (x >= ' ' && x <= '~' && x != '/' && x != '\\' && x != '"' && x != '*' && x != ':' && x != '<' && x != '>' && x != '?' && x != '|')
#define isLowercaseHexa(x) (isNumber(x) || (x >= 'a' && x <= 'f'))
#define isUppercaseHexa(x) (isNumber(x) || (x >= 'A' && x <= 'F'))
#define isHexa(x) (isLowercaseHexa(x) || isUppercaseHexa(x))

#define toLowercase(x) for(int y = 0; y < strlen(x); y++) if(isUppercase(x[y])) x[y] += 32;

bool startThread(OSThread *thread, const char *name, THREAD_PRIORITY priority, void **stackPtr, size_t stacksize, OSThreadEntryPointFn mainfunc, OSThreadAttributes attribs);
char* b_tostring(bool b);
void hex(uint64_t i, int digits, char *out); //ex: 000050D1
void getSpeedString(double bytePerSecond, char *out);
void hexToByte(const char *hex, uint8_t *out);
void glueMcpData(MCPInstallTitleInfo *info, McpData *data);
void showMcpProgress(McpData *data, const char *game, const bool inst);
size_t strlen16(char16_t *str);
char16_t *str16str(char16_t *haystack, char16_t *needle);
char16_t tolower16(char16_t in);
char16_t *str16cpy(char16_t *target, const char16_t *source);
int str16cmp(const char16_t *s1, const char16_t *s2);
#ifdef NUSSPLI_DEBUG
void debugInit();
void debugPrintf(const char *str, ...);
void checkStacks(const char *src);
#endif

#ifdef __cplusplus
	}
#endif

#endif // ifndef NUSSPLI_UTILS_H
