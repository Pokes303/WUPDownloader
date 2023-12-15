/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
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

#include <wut-fixups.h>

#include <stdbool.h>

#include <coreinit/title.h>
#include <mocha/mocha.h>
#include <rpxloader/rpxloader.h>

#include <state.h>
#include <utils.h>

#define VALUE_A 0xE3A00000 // mov r0, #0
#define VALUE_B 0xE12FFF1E // bx lr

static bool mochaReady = false;
static const uint32_t addys[6] = {
    // Cached cert check
    0x05054D6C,
    0x05054D70,
    // Cert verification
    0x05052A90,
    0x05052A94,
    // IOSC_VerifyPubkeySign
    0x05052C44,
    0x05052C48,
};
static uint32_t origValues[6];

bool cfwValid()
{
    MochaUtilsStatus s = Mocha_InitLibrary();
    mochaReady = s == MOCHA_RESULT_SUCCESS;
    bool ret = mochaReady;
    if(ret)
    {
        WiiUConsoleOTP otp;
        s = Mocha_ReadOTP(&otp);
        ret = s == MOCHA_RESULT_SUCCESS;
        if(ret)
        {
            MochaRPXLoadInfo info = {
                .target = 0xDEADBEEF,
                .filesize = 0,
                .fileoffset = 0,
                .path = "dummy"
            };

            s = Mocha_LaunchRPX(&info);
            ret = s != MOCHA_RESULT_UNSUPPORTED_API_VERSION && s != MOCHA_RESULT_UNSUPPORTED_COMMAND;
            if(ret)
            {
                if(isAroma())
                {
                    char path[FS_MAX_PATH];
                    RPXLoaderStatus rs = RPXLoader_GetPathOfRunningExecutable(path, FS_MAX_PATH);
                    ret = rs == RPX_LOADER_RESULT_SUCCESS;
                    if(!ret)
                        debugPrintf("RPXLoader error: %s", RPXLoader_GetStatusStr(rs));
                }

                if(ret)
                {
                    for(int i = 0; i < 6; ++i)
                    {
                        s = Mocha_IOSUKernelRead32(addys[i], origValues + i);
                        if(s != MOCHA_RESULT_SUCCESS)
                            goto restoreIOSU;

                        s = Mocha_IOSUKernelWrite32(addys[i], i % 2 == 0 ? VALUE_A : VALUE_B);
                        if(s != MOCHA_RESULT_SUCCESS)
                            goto restoreIOSU;

                        continue;
                    restoreIOSU:
                        for(--i; i >= 0; --i)
                            Mocha_IOSUKernelWrite32(addys[i], origValues[i]);

                        debugPrintf("libmocha error: %s", Mocha_GetStatusStr(s));
                        return false;
                    }
                }
            }
            else
                debugPrintf("Can't dummy load RPX: %s", Mocha_GetStatusStr(s));
        }
        else
            debugPrintf("Can't acces OTP: %s", Mocha_GetStatusStr(s));
    }
    else
        debugPrintf("Can't init libmocha: 0x%8X", s);

    return ret;
}

void deinitCfw()
{
    if(mochaReady)
    {
        for(int i = 0; i < 6; ++i)
            Mocha_IOSUKernelWrite32(addys[i], origValues[i]);

        Mocha_DeInitLibrary();
    }
}
