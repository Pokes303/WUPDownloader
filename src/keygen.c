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

#include <openssl/evp.h>
#include <openssl/md5.h>

#include <otp.h>
#include <titles.h>
#include <utils.h>

#include <stdint.h>
#include <string.h>

#include <coreinit/memory.h>

static const uint8_t KEYGEN_SECRET[10] = { 0xfd, 0x04, 0x01, 0x05, 0x06, 0x0b, 0x11, 0x1c, 0x2d, 0x49 };

static int encryptAES(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    if(!(ctx = EVP_CIPHER_CTX_new()))
        return 0;

    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        return 0;

    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        return 0;
    ciphertext_len = len;

    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        return 0;
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

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
			return "mypass"; // Seems to work so far even for newest releases
	}
}

bool generateKey(const TitleEntry *te, char *out)
{
	const uint8_t *ti = (const uint8_t *)&(te->tid);
	size_t i;
	size_t j;
	switch(getTidHighFromTid(te->tid))
	{
		case TID_HIGH_VWII_IOS:
			ti += 2;
			i = 8 - 3;
			j = 8 - 3 + 10;
			break;
		default:
			i = 8 - 1;
			j = 8 - 1 + 10;
	}

	uint8_t bh[j];
	OSBlockMove(bh, KEYGEN_SECRET, 10, false);
	OSBlockMove(bh + 10, ++ti, i, false);
	MD5(bh, j, bh);

	const char *pw = transformPassword(te->key);
	debugPrintf("Using password \"%s\"", pw);
	if(PKCS5_PBKDF2_HMAC_SHA1(pw, strlen(pw), bh, 16, 20, 16, bh) == 0)
		return false;

	uint8_t ct[16];
	OSBlockMove(ct, &(te->tid), 8, false);
	OSBlockSet(ct + 8, 0, 8);

	unsigned char tmp[17];
	encryptAES(bh, 16, getCommonKey(), ct, tmp);

	unsigned char *tmpc = tmp;
	--tmpc;
	i = 17;
	while(--i)
	{
		sprintf(out, "%02x", *++tmpc);
		out += 2;
	}
	
#ifdef NUSSPLI_DEBUG
	out -= 32;
	debugPrintf("Key: 0x%s", out);
#endif
	return true;
}
