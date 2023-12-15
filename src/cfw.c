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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include <coreinit/title.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <mocha/mocha.h>
#include <rpxloader/rpxloader.h>

#include <state.h>
#include <utils.h>

#define VALUE_A 0xE3A00000 // mov r0, #0
#define VALUE_B 0xE12FFF1E // bx lr

#define CFW_ERR "Unsupported environment.\nEither you're not using Tiramisu/Aroma or your Tiramisu version is out of date.\n\n"

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
static char *cfwError = NULL;

static void printCfwError(const char *str, ...)
{
    cfwError = MEMAllocFromDefaultHeap(sizeof(char) * 1024);
    if(cfwError == NULL)
        return;

    size_t l = strlen(CFW_ERR);
    OSBlockMove(cfwError, CFW_ERR, l, false);

    va_list va;
    va_start(va, str);
    vsnprintf(cfwError + l, (1024 - 1) - l, str, va);
    va_end(va);
}

const char *cfwValid()
{
    MochaUtilsStatus s = Mocha_InitLibrary();
    mochaReady = s == MOCHA_RESULT_SUCCESS;
    if(!mochaReady)
    {
        printCfwError("Can't init libmocha: 0x%8X", s);
        return cfwError;
    }

    WiiUConsoleOTP otp;
    s = Mocha_ReadOTP(&otp);
    if(s != MOCHA_RESULT_SUCCESS)
    {
        printCfwError("Can't acces OTP: %s", Mocha_GetStatusStr(s));
        return cfwError;
    }

    MochaRPXLoadInfo info = {
        .target = 0xDEADBEEF,
        .filesize = 0,
        .fileoffset = 0,
        .path = "dummy"
    };

    s = Mocha_LaunchRPX(&info);
    if(s == MOCHA_RESULT_UNSUPPORTED_API_VERSION || s == MOCHA_RESULT_UNSUPPORTED_COMMAND)
    {
        printCfwError("Can't dummy load RPX: %s", Mocha_GetStatusStr(s));
        return cfwError;
    }

    if(isAroma())
    {
        char path[FS_MAX_PATH];
        RPXLoaderStatus rs = RPXLoader_GetPathOfRunningExecutable(path, FS_MAX_PATH);
        if(rs != RPX_LOADER_RESULT_SUCCESS)
        {
            printCfwError("RPXLoader error: %s", RPXLoader_GetStatusStr(rs));
            return cfwError;
        }
    }

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

        printCfwError("libmocha error: %s", Mocha_GetStatusStr(s));
        return cfwError;
    }

    return NULL;
}

void deinitCfw()
{
    if(mochaReady)
    {
        for(int i = 0; i < 6; ++i)
            Mocha_IOSUKernelWrite32(addys[i], origValues[i]);

        Mocha_DeInitLibrary();
    }

    if(cfwError != NULL)
        MEMFreeToDefaultHeap(cfwError);
}
