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

#include <file.h>
#include <filesystem.h>
#include <iosuhaxx.h>
#include <status.h>
#include <utils.h>

#include <stdbool.h>
#include <stdio.h>

#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>

int fsaHandle = -1;
bool usbMounted = false;
bool mlcMounted = false;
bool usb01;

bool isUSB01()
{
	return usb01;
}

static bool initFsa()
{
	if(fsaHandle >= 0)
		return true;

	if(!openIOSUhax())
        return false;

	fsaHandle = IOSUHAX_FSA_Open();
	if(fsaHandle < 0)
	{
		closeIOSUhax();
		debugPrintf("IOSUHAX: Error opening FSA!");
		return false;
	}
	return true;
}

bool mountUSB()
{
	if(usbMounted)
		return true;

	if(!initFsa())
		return false;

	int ret = mount_fs("usb", fsaHandle, NULL, "/vol/storage_usb01");
	if(ret != 0 || !dirExists("usb:/"))
	{
		debugPrintf("IOSUHAX: error mounting USB drive 1: %#010x", ret);
		ret = mount_fs("usb", fsaHandle, NULL, "/vol/storage_usb02");
		if(ret != 0 || !dirExists("usb:/"))
		{
			IOSUHAX_FSA_Close(fsaHandle);
			closeIOSUhax();
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

	int ret = mount_fs("mlc", fsaHandle, NULL, "/vol/storage_mlc01");
	if(ret || !dirExists("mlc:/"))
	{
		debugPrintf("IOSUHAX: error mounting MLC: %#010x", ret);
		return false;
	}

	debugPrintf("IOSUHAX: MLC mounted!");
	mlcMounted = true;
	return true;
}

static void closeFsa()
{
	IOSUHAX_FSA_Close(fsaHandle);
	closeIOSUhax();
	fsaHandle = -1;
}

void unmountUSB()
{
	if(!usbMounted)
		return;

	unmount_fs("usb");
	usbMounted = false;
	if(!mlcMounted)
		closeFsa();

	debugPrintf("IOSUHAX: USB drive unmounted!");
}

void unmountMLC()
{
	if(!mlcMounted)
		return;

	unmount_fs("mlc");
	mlcMounted = false;
	if(!usbMounted)
		closeFsa();

	debugPrintf("IOSUHAX: MLC unmounted!");
}
