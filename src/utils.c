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

#include <wut-fixups.h>

#include <utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>

int mcpHandle;

char* hex(uint64_t i, int digits) {
	char h[33];
	sprintf(h, "%lx%lx", (long unsigned int)((i & 0xFFFF0000) >> 16), (long unsigned int)(i & 0x0000FFFF));
	size_t hexDigits = strlen(h);
	if (hexDigits > digits)
		return "too few digits error";
	
	char *result = MEMAllocFromDefaultHeap(sizeof(char) * (digits + 1));
	if(result != NULL)
	{
		int n = digits - hexDigits;
		if(n > 0)
		{
			OSBlockSet(result, '0', n);
			result[n] = '\0';
			strcat(result, h);
		}
		else
			strcpy(result, h);
	}
	
	return result;
}

bool isNumber(char c)
{
	return c >= '0' && c <= '9';
}

bool isLowercase(char c)
{
	return c >= 'a' && c <= 'z';
}

bool isUppercase(char c)
{
	return c >= 'A' && c <= 'Z';
}

bool isAlphanumerical(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || isNumber(c);
}

bool isLowercaseHexa(char c)
{
	return isNumber(c) || (c >= 'a' && c <= 'f');
}

bool isUppercaseHexa(char c)
{
	return isNumber(c) || (c >= 'A' && c <= 'F');
}

bool isHexa(char c)
{
	return isLowercaseHexa(c) || (c >= 'A' && c <= 'F');
}

void toLowercase(char *inOut)
{
	if(inOut == NULL)
		return;
	
	for(int i = 0; i < strlen(inOut); i++)
		if(isUppercase(inOut[i]))
			inOut[i] += 32;
}

// This extreme fast RNG is based on George Marsaglias paper "Xorshift RNGs" from https://www.jstatsoft.org/article/view/v008i14
// TODO: Tune the wordsize to the Wii Us CPU.
uint32_t rngState;
uint32_t lastSeed = 0;
uint32_t getRandom()
{
	rngState ^= rngState << 13;
	rngState ^= rngState >> 17;
	rngState ^= rngState << 5;
	if(rngState == 0)
	{
		initRandom();
		return 0;
	}
	return rngState;
}

void initRandom()
{
	uint32_t seed = OSGetTick();
	if(lastSeed == seed)
	{
		OSSleepTicks(1);
		seed++;
	}
	rngState = lastSeed = seed;
}

void getSpeedString(float bytePerSecond, char *out)
{
	float bitPerSecond = bytePerSecond * 8.0f;
	
	if(bitPerSecond < 1024.0f)
		sprintf(out, "%.2f b/s (", bitPerSecond);
	else if(bitPerSecond < 1024.0f * 1024.0f)
		sprintf(out, "%.2f Kb/s (", bitPerSecond / 1024.0f);
	else
		sprintf(out, "%.2f Mb/s (", bitPerSecond / (1024.0f * 1024.0f));
	
	out += strlen(out);
	
	if(bytePerSecond < 1024.0f)
		sprintf(out, "%.2f B/s)", bytePerSecond);
	else if(bytePerSecond < 1024.0f * 1024.0f)
		sprintf(out, "%.2f KB/s)", bytePerSecond / 1024.0f);
	else
		sprintf(out, "%.2f MB/s)", bytePerSecond / (1024.0f * 1024.0f));
}

uint8_t charToByte(char c)
{
	if(isNumber(c))
		return c - '0';
	if(isLowercaseHexa(c))
		return c - 'a' + 0xA;
	if(isUppercaseHexa(c))
		return c - 'A' + 0xA;
	return 0xFF;
}

void hexToByte(const char *hex, uint8_t *out)
{
	for(int i = 0; *hex != '\0'; out[i++] |= charToByte(*hex++))
		out[i] = charToByte(*hex++) << 4;
}

#ifdef NUSSPLI_DEBUG

#include <stdarg.h>

#include <coreinit/fastmutex.h>
#include <coreinit/time.h>
#include <whb/log.h>

char days[7][4] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
};

char months[12][4] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Nov",
	"Dez",
};

OSFastMutex debugMutex;

void debugInit()
{
	OSFastMutex_Init(&debugMutex, "NUSspli debug");
	OSFastMutex_Unlock(&debugMutex);
	WHBLogUdpInit();
}

void debugPrintf(const char *str, ...)
{
	va_list va;
	va_start(va, str);
	char newStr[2048];
	
	OSCalendarTime now;
    OSTicksToCalendarTime(OSGetTime(), &now);
	sprintf(newStr, "%s %02d %s %d %02d:%02d:%02d.%03d\t", days[now.tm_wday], now.tm_mday, months[now.tm_mon], now.tm_year, now.tm_hour, now.tm_min, now.tm_sec, now.tm_msec);

	size_t tss = strlen(newStr);
	
	vsnprintf(newStr + tss, 2048 - tss, str, va);
	va_end(va);
	
	while(!OSFastMutex_TryLock(&debugMutex))
		;
	WHBLogPrint(newStr);
	OSFastMutex_Unlock(&debugMutex);
}

#endif // ifdef NUSSPLI_DEBUG
