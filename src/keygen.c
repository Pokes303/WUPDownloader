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

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

#include <otp.h>
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

	if(PKCS5_PBKDF2_HMAC_SHA1(pw, strlen(pw), ct, 16, 20, 16, key) == 0)
		return NULL;
	
	hexToByte(tid, ct);
	OSBlockSet(ct + 8, 0, 8);

	AES_KEY aesk;
	AES_set_encrypt_key(getCommonKey(), 128, &aesk);
	AES_cbc_encrypt(key, h, 16, &aesk, ct, AES_ENCRYPT);

	char *ret = MEMAllocFromDefaultHeap(33);
	if(ret == NULL)
		return NULL;
	ret[32] = '\0';

	char *t = ret;
	for(int i = 0; i < 16; i++, t += 2)
		sprintf(t, "%02x", h[i]);
	
	debugPrintf("Key: 0x%s", ret);
	return ret;
}
