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
#include <thread.h>

#include <coreinit/time.h>

#include <openssl/rand.h>
#include <openssl/rand_drbg.h>
#include <openssl/ssl.h>

static volatile uint32_t entropy;
static volatile spinlock entropyLock = SPINLOCK_FREE;

static int osslBytes(unsigned char *buf, int num)
{
	spinLock(entropyLock);

	for(unsigned char *tmp = buf + num - 1; tmp >= buf; --tmp)
		*tmp = rand_r((uint32_t *)&entropy);

	spinReleaseLock(entropyLock);
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

void reseed()
{
	srand(entropy);
}

void addEntropy(void *e, size_t l)
{
	while(!spinTryLock(entropyLock))
		OSSleepTicks(128);

	uint8_t *buf = (uint8_t *)e;
	uint32_t ee;
	int s;
	for(size_t i = 0; i < l; ++i)
	{
		ee = buf[i];
		s = rand_r((uint32_t *)&entropy) % 25;
		if(s)
			ee <<= s;
		entropy ^= ee;
	}

	spinReleaseLock(entropyLock);
}

bool initCrypto()
{
	entropy ^= OSGetTick();
	reseed();
	return OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, NULL) == 1 &&
		RAND_set_rand_method(&srm) == 1 &&
		RAND_DRBG_set_reseed_defaults(0, 0, 0, 0) == 1;
}

uint32_t getEntropy()
{
    return entropy;
}
