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
#include <iosuhaxx.h>
#include <status.h>
#include <usb.h>
#include <utils.h>

#include <stdbool.h>
#include <stdio.h>

#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>

int fsaHandle = -1;
bool usb01;

bool isUSB01()
{
	return usb01;
}

bool mountUSB()
{
	if(fsaHandle >= 0)
		return true;
	
	// Try to open Aroma/Mocha iosuhax
	if(!openIOSUhax())
        return false;
	
	fsaHandle = IOSUHAX_FSA_Open();
	if(fsaHandle < 0)
	{
		closeIOSUhax();
		debugPrintf("IOSUHAX: Error opening FSA!");
		return false;
	}
	
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
	return true;
}

void unmountUSB()
{
	if(fsaHandle < 0)
		return;
	
	unmount_fs("usb");
	IOSUHAX_FSA_Close(fsaHandle);
	closeIOSUhax();
	
	debugPrintf("IOSUHAX: USB drive unmounted!");
}
