/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
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

#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <iosuhax.h>

#include <iosuhaxx.h>

static bool iosuConnected = false;

void closeIOSUhax()
{
	if(iosuConnected)
	{
		IOSUHAX_Close();
		iosuConnected = false;
	}
}

bool openIOSUhax()
{
	if(iosuConnected)
		return true;

	iosuConnected = IOSUHAX_Open(NULL) >= 0;
	return iosuConnected;
}
