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

#include <renderer.h>
#include <status.h>
#include <utils.h>
#include <menu/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <uchar.h>

#include <coreinit/ios.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>

int mcpHandle;

char* hex(uint64_t i, int digits) {
	if(digits > 16)
		return NULL;
	
	char x[16];
	sprintf(x, "%%0%illx", digits);
	char *result = MEMAllocFromDefaultHeap(sizeof(char) * (digits + 1));
	if(result == NULL)
		return NULL;
	
	sprintf(result, x, i);
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

// Keep it to ASCII for FTPiiU compat.
bool isAllowedInFilename(char c)
{
	return c >= ' ' && c <= '~' && c != '/' && c != '\\' && c != '"' && c != '*' && c != ':' && c != '<' && c != '>' && c != '?' && c != '|';
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
	for(int i = 0; *hex != '\0' && i < 64; out[i++] |= charToByte(*hex++))
		out[i] = charToByte(*hex++) << 4;
}

static void mcpCallback(MCPError err, void *rawData)
{
	McpData *data = (McpData *)rawData;
	data->err = err;
	data->processing = false;
}

void glueMcpData(MCPInstallTitleInfo *info, McpData *data)
{
	data->processing = true;
	uint32_t *ptr = (uint32_t *)info;
	*ptr = (uint32_t)mcpCallback;
	ptr++;
	*ptr = (uint32_t)data;
}

void showMcpProgress(McpData *data, const char *game, const bool inst)
{
	MCPInstallProgress *progress = MEMAllocFromDefaultHeapEx(sizeof(MCPInstallProgress), 0x40);
	if(progress == NULL)
	{
		debugPrintf("Error allocating memory!");
		enableShutdown();
		return;
	}
	
	progress->inProgress = 0;
	char multiplierName[3];
	int multiplier = 0;
	char *toScreen = getToFrameBuffer();
	MCPError err;
	OSTime lastSpeedCalc = 0;
	OSTime now;
	uint64_t sc = 0;
	uint64_t lsp = 0;
	char speedBuf[32];
	speedBuf[0] = '\0';
	
	while(data->processing)
	{
		err = MCP_InstallGetProgress(mcpHandle, progress);
		if(err == IOS_ERROR_OK)
		{
			if(progress->inProgress == 1 && progress->sizeTotal != 0)
			{
				if(multiplier == 0)
				{
					if(progress->sizeTotal < 1 << 10)
					{
						multiplier = 1;
						strcpy(multiplierName, "B");
					}
					else if(progress->sizeTotal < 1 << 20)
					{
						multiplier = 1 << 10;
						strcpy(multiplierName, "KB");
					}
					else if(progress->sizeTotal < 1 << 30)
					{
						multiplier = 1 << 20;
						strcpy(multiplierName, "MB");
					}
					else
					{
						multiplier = 1 << 30;
						strcpy(multiplierName, "GB");
					}
				}
				now = OSGetSystemTime();
				if(OSTicksToMilliseconds(now - lastSpeedCalc) > 33)
				{
					startNewFrame();
					strcpy(toScreen, inst ? "Installing " : "Uninstalling ");
					strcat(toScreen, game);
					textToFrame(0, 0, toScreen);
					barToFrame(1, 0, 40, progress->sizeProgress * 100.0f / progress->sizeTotal);
					sprintf(toScreen, "%.2f / %.2f %s", ((float)progress->sizeProgress) / multiplier, ((float)progress->sizeTotal) / multiplier, multiplierName);
					textToFrame(1, 41, toScreen);
					
					if(progress->sizeProgress != 0)
					{
						if(sc-- == 0)
						{
							getSpeedString(progress->sizeProgress - lsp, speedBuf);
							lsp = progress->sizeProgress;
							sc = 10;
						}
					}
					textToFrame(1, ALIGNED_RIGHT, speedBuf);
					
					writeScreenLog();
					drawFrame();
					lastSpeedCalc = now;
				}
			}
		}
		else
			debugPrintf("MCP_InstallGetProgress() returned %#010x", err);
		
		showFrame();
	}
}

size_t strlen16(char16_t *str)
{
	size_t ret = 0;
	while(str[ret] != u'\0')
		ret++;
	return ret;
}

char16_t *str16str(char16_t *haystack, char16_t *needle)
{
	size_t needleL = strlen16(needle);
	if(needleL == 0)
		return haystack;
	
	size_t hayL = strlen16(haystack);
	if(needleL > hayL)
		return NULL;
	
	size_t i = 0;
	char16_t *ret;
	do
	{
		ret = haystack + i;
		for(size_t j = 0 ; j < needleL; j++)
		{
			if(haystack[i + j] != needle[j])
			{
				ret = NULL;
				break;
			}
		}
		i++;
	}
	while(ret == NULL && haystack + i <= haystack + (hayL - needleL));
	
	return ret;
}

char16_t tolower16(char16_t in)
{
	if(in >= u'A' && (in <= u'Z' || (in >= u'À' && in <= u'Þ')))
		in += 0x0020;
	return in;
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

void checkStacks(const char *src)
{
	debugPrintf("%s: Checking thread stacks...", src);
	OSCheckActiveThreads();
	debugPrintf("%s: Done!", src);
}

#endif // ifdef NUSSPLI_DEBUG
