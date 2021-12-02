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

#include <stdbool.h>
#include <stddef.h>

#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <iosuhax.h>

#include <iosuhaxx.h>
#include <utils.h>

bool iosuConnected = false;
bool haxchi;

static void iosuCallback(IOSError err, void *dummy)
{
	debugPrintf("IOSUHAX: Closing callback!");
	iosuConnected = false;
}

void closeIOSUhax()
{
	if(!iosuConnected)
		return;

	//close down wupserver, return control to mcp
	IOSUHAX_Close();
	//wait for mcp to return
	if(haxchi)
	{
		while(iosuConnected)
			OSSleepTicks(1024 << 10); //TODO: What's a good value here?
	}
	else
		iosuConnected = false;
}

bool openIOSUhax()
{
	if(iosuConnected)
		return true;

	if(IOSUHAX_Open(NULL) < 0)
	{
		// We're not on Mocha. So try the Haxchi method of taking over MCP
		IOSError err = IOS_IoctlAsync(mcpHandle, 0x62, NULL, 0, NULL, 0, iosuCallback, NULL);
		if(err != IOS_ERROR_OK)
		{
			debugPrintf("IOSUHAX: Error sending IOCTL: %d", err);
			return false;
		}

		//let wupserver start up
		OSSleepTicks(OSMillisecondsToTicks(50)); // TODO: Extensively test this in release builds
		if(IOSUHAX_Open("/dev/mcp") < 0)
		{
			debugPrintf("IOSUHAX: Error opening!");
			return false;
		}
		haxchi = true;
	}
	else
		haxchi = false;

	iosuConnected = true;
	return true;
}
