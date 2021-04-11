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

#include <aes.h>
#include <md5.h>
#include <otp.h>
#include <pbkdf2.h>

#include <utils.h>

#include <string.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#define KEYGEN_SECRET  "fd040105060b111c2d49"

//const uint8_t keygen_pw[] = { 0x6e, 0x690, 0x6e, 0x74, 0x65, 0x6e, 0x64, 0x6f };
const uint8_t keygen_pw[] = { 0x6d, 0x79, 0x70, 0x61, 0x73, 0x73 };


char *generateKey(const char *tid)
{
	char *ret = MEMAllocFromDefaultHeap(33);
	if(ret == NULL)
		return NULL;
	ret[32] = '\0';
	
	const char *tmp = tid;
	while(tmp[0] == '0' && tmp[1] == '0')
		tmp += 2;
	
	char h[1024];
	strcpy(h, KEYGEN_SECRET);
	strcat(h, tmp);
	
	size_t bhl = strlen(h) >> 1;
	uint8_t bh[bhl];
	hexToByte(h, bh);
	
	MD5_CTX md5c;
	uint8_t md5sum[16];
	MD5_Init(&md5c);
	MD5_Update(&md5c, bh, bhl);
	MD5_Final(&md5sum[0], &md5c);
	
	uint8_t key[16];
	pbkdf2_hmac_sha1(keygen_pw, sizeof(keygen_pw), md5sum, 16, 20, key, 16);
	
	uint8_t iv[16];
	hexToByte(tid, iv);
	
	OSBlockSet(&iv[8], 0, 8);
	struct AES_ctx aesc;
	AES_init_ctx_iv(&aesc, getCommonKey(), iv);
	AES_CBC_encrypt_buffer(&aesc, key, 16);
	
	char *t = ret;
	for(int i = 0; i < 16; i++, t += 2)
		sprintf(t, "%02x", key[i]);
	
	return ret;
}
