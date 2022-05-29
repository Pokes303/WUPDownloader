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

#include <file.h>
#include <filesystem.h>
#include <iosuhaxx.h>
#include <state.h>
#include <utils.h>

#include <stdbool.h>
#include <stdio.h>

#include <coreinit/filesystem.h>
#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>

static bool usbMounted = false;
static bool mlcMounted = false;
static bool usb01;

extern FSClient *__wut_devoptab_fs_client;

bool isUSB01()
{
	return usb01;
}

static bool initFsa()
{
	if(!openIOSUhax())
        return false;

	return IOSUHAX_UnlockFSClient(__wut_devoptab_fs_client) >= 0;
}

bool mountUSB()
{
	if(usbMounted)
		return true;

	if(!initFsa())
		return false;

	int ret = IOSUHAX_FSMount(__wut_devoptab_fs_client, "/dev/usb01", "/vol/usb");

	if(ret < 0 || !dirExists("fs:/vol/usb"))
	{
		debugPrintf("IOSUHAX: error mounting USB drive 1: %#010x", ret);
		ret = IOSUHAX_FSMount(__wut_devoptab_fs_client, "/dev/usb02", "/vol/usb");
		if(ret < 0 || !dirExists("fs:/vol/usb"))
		{
			debugPrintf("IOSUHAX: error mounting USB drive 2: %#010x", ret);
			return false;
		}
		usb01 = false;
	}
	else
		usb01 = true;

	debugPrintf("IOSUHAX: USB drive %s mounted!", usb01 ? "1" : "2");
	usbMounted = true;
	return true;
}

bool mountMLC()
{
	if(mlcMounted)
		return true;

	if(!initFsa())
		return false;

	int ret = IOSUHAX_FSMount(__wut_devoptab_fs_client, "/dev/mlc01", "/vol/mlc");
	if(ret < 0 || !dirExists("fs:/vol/mlc"))
	{
		debugPrintf("IOSUHAX: error mounting MLC: %#010x", ret);
		return false;
	}

	debugPrintf("IOSUHAX: MLC mounted!");
	mlcMounted = true;
	return true;
}

void unmountUSB()
{
	if(!usbMounted)
		return;

	// TODO
//	usbMounted = false;

//	debugPrintf("IOSUHAX: USB drive unmounted!");
}

void unmountMLC()
{
	if(!mlcMounted)
		return;

	// TODO
//	mlcMounted = false;

//	debugPrintf("IOSUHAX: MLC unmounted!");
}
