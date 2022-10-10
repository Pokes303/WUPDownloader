/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2021-2022 V10lator <v10lator@myway.de>                    *
 * Copyright (c) 2022 Xpl0itU <DaThinkingChair@protonmail.com>             *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#pragma once

#include <wut-fixups.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define deinitCrypto()

    bool initCrypto();
    void addEntropy(void *e, size_t len) __attribute__((__hot__));
    int NUSrng(void *data, unsigned char *out, size_t outlen);
    bool encryptAES(void *data, int data_len, const unsigned char *key, unsigned char *iv, void *encrypted);

#define osslBytes(buf, num) NUSrng(NULL, (unsigned char *)buf, num)

#ifdef __cplusplus
}
#endif
