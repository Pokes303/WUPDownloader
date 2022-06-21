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
#include <utils.h>

#include <coreinit/filesystem.h>
#include <iosuhax.h>

#include <stdbool.h>

extern FSClient *__wut_devoptab_fs_client;

static NUSDEV usb = NUSDEV_SD;

bool initFS()
{
	if(IOSUHAX_Open(NULL) >= 0)
	{
		if(IOSUHAX_UnlockFSClient(__wut_devoptab_fs_client) == FS_ERROR_NOERROR)
		{
			bool ret = IOSUHAX_FSAMount(__wut_devoptab_fs_client, "/vol/external01", "/vol/app_sd", 0x01, NULL, 0) == FS_ERROR_NOERROR;
			if(ret)
				return true;
		}

		IOSUHAX_Close();
	}

	return false;
}

void deinitFS()
{
#ifdef NUSSPLI_DEBUG
	FSError err =
#endif
	IOSUHAX_FSAUnmount(__wut_devoptab_fs_client, "/vol/app_sd");
	IOSUHAX_Close();
	usb = NUSDEV_SD;
#ifdef NUSSPLI_DEBUG
	if(err != FS_ERROR_NOERROR)
		debugPrintf("Error unmounting SD: 0x%08X", err);
#endif
}

NUSDEV getUSB()
{
	if(usb == NUSDEV_SD)
	{
		if(dirExists(NUSDIR_USB1 "usr"))
			usb = NUSDEV_USB01;
		else if(dirExists(NUSDIR_USB2 "usr"))
			usb = NUSDEV_USB02;
		else
			usb = NUSDEV_NONE;
	}

	return usb;
}
