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

#include <file.h>
#include <status.h>
#include <usb.h>
#include <utils.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/title.h>


static const uint8_t drcTexMd5[16] = { 0x33, 0xe1, 0xd6, 0x7f, 0x62, 0xc9, 0xf4, 0x48, 0xc5, 0xc0, 0x95, 0xe5, 0x95, 0xaa, 0x6f, 0xc6 };

bool sanityCheck()
{
	if(isChannel())
	{
		MCPTitleListType title;
		if(MCP_GetTitleInfo(mcpHandle, OSGetTitleID(), &title))
			return false;

		if(title.path == NULL || strlen(title.path) != 45)
			return false;

		title.path[16] = '\0';
		bool isUsb;
		if(strcmp(title.path, "/vol/storage_usb") == 0)
			isUsb = true;
		else if(strcmp(title.path, "/vol/storage_mlc") == 0)
			isUsb = false;
		else
			return false;

		char newPath[1024];
		bool ret = false;
		if(isUsb)
		{
			strcpy(newPath, "usb:");
			strcat(newPath, title.path + 17);
			strcat(newPath, "/meta/bootDrcTex.tga");

			mountUSB();
		}

		if(fileExists(newPath))
		{
			FILE *f = fopen(newPath, "rb+");
			if(f != NULL)
			{
				uint32_t s = getFilesize(f);
				void *buf = MEMAllocFromDefaultHeap(s);
				if(buf != NULL)
				{
					if(fread(buf, s, 1, f) == 1)
					{
						MD5_CTX md5c;
						uint8_t m[16];
						MD5_Init(&md5c);
						MD5_Update(&md5c, buf, s);
						MD5_Final(&m[0], &md5c);

						ret = true;
						for(int i = 0; i < 16; i++)
						{
							if(m[i] != drcTexMd5[i])
							{
								ret = false;
								break;
							}
						}
					}
					MEMFreeToDefaultHeap(buf);
				}
				fclose(f);
			}
		}

		if(isUsb)
			unmountUSB();

		return ret;
	}

	// else / isAroma()
	//TODO
	return true;
}

#endif
