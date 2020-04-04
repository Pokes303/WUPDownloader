#include "main.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <coreinit/energysaver.h>
#include <coreinit/foreground.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/systeminfo.h>
#include <whb/log.h>

void enableShutdown() {
	if (!hbl)
		OSEnableHomeButtonMenu(true);
	IMEnableAPD();
}
void disableShutdown() {
	if (!hbl)
		OSEnableHomeButtonMenu(false);
	IMDisableAPD();
}

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

bool pathExists(char *path)
{
	struct stat fileStat;
	return stat(path, &fileStat) == 0;
}
