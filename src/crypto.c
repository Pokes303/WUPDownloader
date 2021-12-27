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

#include <wut-fixups.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <utils.h>

#include <crypto.h>

#include <openssl/rand.h>
#include <openssl/rand_drbg.h>
#include <openssl/ssl.h>

#include <coreinit/time.h>

static volatile uint32_t entropy;

#define reseed()					\
{									\
	entropy ^= OSGetSystemTick();	\
	entropy ^= OSGetTick();			\
}

// Based on George Marsaglias paper "Xorshift RNGs" from https://www.jstatsoft.org/article/view/v008i14
#define rngRun()					\
{									\
	if(entropy)						\
	{								\
		entropy ^= entropy << 13;	\
		entropy ^= entropy >> 17;	\
		entropy ^= entropy << 5;	\
	}								\
	else							\
		reseed();					\
}

int osslBytes(unsigned char *buf, int num)
{
	--buf;
	++num;
	while(--num)
	{
		rngRun();
		*++buf = entropy;
	}

	return 1;
}

static void osslCleanup()
{
	// STUB
}

static int osslStatus()
{
	return 1;
}

static const RAND_METHOD srm = {
	.seed = NULL,
	.bytes = osslBytes,
	.cleanup = osslCleanup,
	.add = NULL,
	.pseudorand = osslBytes,
	.status = osslStatus,
};

void addEntropy(void *e, size_t l)
{
	uint8_t *buf8 = (uint8_t *)e;
	uint32_t *buf32;
	size_t l32 = l >> 2;
	if(l32)
	{
		buf32 = (uint32_t *)e;
		--buf32;
		buf8 += l32 << 2;
		l %= 4;
	}

	++l;
	++l32;
	--buf8;

	while(--l32)
	{
		rngRun();
		entropy ^= *++buf32;
	}

	while(--l)
	{
		rngRun();
		entropy ^= *++buf8;
	}
}

bool initCrypto()
{
	reseed();
	return OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL) == 1 &&
		RAND_set_rand_method(&srm) == 1 &&
		RAND_DRBG_set_reseed_defaults(0, 0, 0, 0) == 1;
}
