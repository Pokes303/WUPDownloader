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

// This is a uchar.h implementation as required by C11.
// It's missing in newlib as newlib is ANSI C only.

#pragma once

#ifndef __cplusplus
	#include <wut-fixups.h>
	#include <stddef.h>
	#include <stdint.h>
	#include <wchar.h>
	
	typedef __uint_least16_t char16_t;
	typedef __uint_least32_t char32_t;
	
	// These function are a WIP. Don't use them yet.
	size_t mbrtoc16(char16_t *out, const char *in, size_t size, mbstate_t *mbs);
	size_t c16rtomb(char *out, char16_t in, mbstate_t *mbs);
	size_t mbrtoc32(char32_t *out, const char *in, size_t size, mbstate_t *mbs);
	size_t c32rtomb(char *out, char32_t in, mbstate_t *mbs);
#endif // ifndef __cplusplus
