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

#ifndef NUSSPLI_MEMDEBUG_H
#define NUSSPLI_MEMDEBUG_H

#ifdef SOME_NEVER_DEFINED_THING // NUSSPLI_DEBUG

	#include <coreinit/memheap.h>

	#ifdef __cplusplus
		extern "C" {
	#endif
	void initASAN();
	void deinitASAN();
	#ifdef __cplusplus
		}
	#endif
#else
	#define initASAN(...)
	#define deinitASAN(void)
#endif

#endif // ifndef NUSSPLI_MEMDEBUG_H
