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

#ifndef NUSSPLI_HBL

#include <wut-fixups.h>

#include <openssl/md5.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <crypto.h>
#include <file.h>
#include <filesystem.h>
#include <status.h>
#include <utils.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/time.h>
#include <coreinit/title.h>

#define MD5_FILES 6

static const uint8_t md5[MD5_FILES][16] = {
	{ 0x88, 0xbd, 0xe1, 0xac, 0xdd, 0xf1, 0x9e, 0x08, 0xfb, 0x1a, 0xe9, 0xf6, 0x6f, 0x97, 0x03, 0xe7 },
	{ 0x33, 0xe1, 0xd6, 0x7f, 0x62, 0xc9, 0xf4, 0x48, 0xc5, 0xc0, 0x95, 0xe5, 0x95, 0xaa, 0x6f, 0xc6 },
	{ 0xa7, 0xac, 0xd1, 0x88, 0x3e, 0x81, 0x6f, 0xab, 0x90, 0xff, 0x7e, 0x21, 0xa2, 0xae, 0xbb, 0xd2 },
	{ 0x70, 0x56, 0xac, 0x62, 0x63, 0x45, 0x6a, 0x2e, 0x1d, 0x2b, 0x99, 0x2a, 0xee, 0x82, 0x22, 0x71 },
	{ 0xc8, 0x85, 0xa1, 0x65, 0x3d, 0x16, 0x71, 0xb0, 0xaa, 0xd3, 0xf4, 0xf6, 0xeb, 0xdd, 0xab, 0x38 },
	{ 0x0b, 0x77, 0xc7, 0x96, 0x6d, 0x1a, 0xbb, 0xc3, 0xc9, 0x15, 0x1a, 0xe4, 0xfa, 0x4a, 0x6d, 0x53 },
};

static const char *md5File[MD5_FILES] = {
	"iconTex.tga",
	"bootDrcTex.tga",
	"BootTvTex.tga",
	"bootLogoTex.tga",
	"bootMovie.h264",
	"bootSound.btsnd",
};

bool sanityCheck()
{
	if(isChannel())
	{
		MCPTitleListType title;
		OSTime t = OSGetSystemTime();
		if(MCP_GetTitleInfo(mcpHandle, OSGetTitleID(), &title))
		{
			debugPrintf("Sanity error: Can't get MCPTitleListType");
			return false;
		}

		if(title.path == NULL || strlen(title.path) != 46)
		{
			debugPrintf("Sanity error: Incorrect length of path");
			return false;
		}

		title.path[16] = '\0';
		bool isUsb;
		if(strcmp(title.path, "/vol/storage_usb") == 0)
			isUsb = true;
		else if(strcmp(title.path, "/vol/storage_mlc") == 0)
			isUsb = false;
		else
		{
			debugPrintf("Can't determine storage device (%s)", title.path);
			return false;
		}

		char newPath[64];
		FILE *f;
		uint32_t s;
		void *buf;
		uint8_t m[16];
		bool ret = true;
		bool br = false;

		if(isUsb)
		{
			mountUSB();
			strcpy(newPath, "usb:");
		}
		else
		{
			mountMLC();
			strcpy(newPath, "mlc:");
		}

		strcpy(newPath + 4, title.path + 18);
		strcat(newPath, "/meta/");

		for(int i = 0; !br && i < MD5_FILES; i++)
		{
			strcpy(newPath + 38, md5File[i]);
			f = fopen(newPath, "rb+");
			if(f == NULL)
			{
				ret = false;
				break;
			}

			s = getFilesize(f);
			buf = MEMAllocFromDefaultHeap(s);
			if(buf == NULL)
			{
				ret = false;
				break;
			}

			if(fread(buf, s, 1, f) != 1)
			{
				MEMFreeToDefaultHeap(buf);
				fclose(f);
				ret = false;
				break;
			}

			MD5(buf, s, m);
			for(int j = 0; j < 16; j++)
			{
				if(m[j] != md5[i][j])
				{
					debugPrintf("Sanity error: Data integrity error");
					ret = false;
					br = true;
					break;
				}
			}

			MEMFreeToDefaultHeap(buf);
			fclose(f);
		}

		if(isUsb)
			unmountUSB();
		else
			unmountMLC();

        t = OSGetSystemTime() - t;
		addEntropy(&t, sizeof(OSTime));
		return ret;
	}

	// else / isAroma()
	//TODO
	return true;
}

#endif
