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

#include <wut-fixups.h>

#include <openssl/md5.h>

#include <aes.h>
#include <otp.h>
#include <pbkdf2.h>

#include <titles.h>
#include <utils.h>

#include <string.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#define KEYGEN_SECRET  "fd040105060b111c2d49"

static inline const char *transformPassword(TITLE_KEY in)
{
	switch(in)
	{
		case TITLE_KEY_mypass:
			return "mypass";
		case TITLE_KEY_nintendo:
			return "nintendo";
		case TITLE_KEY_test:
			return "test";
		case TITLE_KEY_1234567890:
			return "1234567890";
		case TITLE_KEY_Lucy131211:
			return "Lucy131211";
		case TITLE_KEY_fbf10:
			return "fbf10";
		case TITLE_KEY_5678:
			return "5678";
		case TITLE_KEY_1234:
			return "1234";
		case TITLE_KEY_:
			return "";
		default:
			debugPrintf("Unknown password!");
			return "mypass"; // TODO
	}
}

char *generateKey(const TitleEntry *te)
{
	char *ret = MEMAllocFromDefaultHeap(33);
	if(ret == NULL)
		return NULL;
	ret[32] = '\0';
	
	char *tid = hex(te->tid, 16);
	if(tid == NULL)
		return NULL;
	
	char *tmp = tid;
	while(tmp[0] == '0' && tmp[1] == '0')
		tmp += 2;
	
	char h[1024];
	strcpy(h, KEYGEN_SECRET);
	strcat(h, tmp);
	MEMFreeToDefaultHeap(tid);
	
	size_t bhl = strlen(h) >> 1;
	uint8_t bh[bhl];
	hexToByte(h, bh);
	
	MD5_CTX md5c;
	uint8_t ct[16];
	MD5_Init(&md5c);
	MD5_Update(&md5c, bh, bhl);
	MD5_Final(&ct[0], &md5c);
	
	uint8_t key[16];
	const char *pw = transformPassword(te->key);
	debugPrintf("Using password \"%s\"", pw);
	pbkdf2_hmac_sha1((uint8_t *)pw, strlen(pw), ct, 16, 20, key, 16);
	
	hexToByte(tid, ct);
	
	OSBlockSet(ct + 8, 0, 8);
	struct AES_ctx aesc;
	AES_init_ctx_iv(&aesc, getCommonKey(), ct);
	AES_CBC_encrypt_buffer(&aesc, key, 16);
	
	char *t = ret;
	for(int i = 0; i < 16; i++, t += 2)
		sprintf(t, "%02x", key[i]);
	
	debugPrintf("Key: 0x%s", ret);
	return ret;
}
