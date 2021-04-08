/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
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

#ifndef NUSSPLI_INPUT_H
#define NUSSPLI_INPUT_H

#include <wut-fixups.h>

#include <stdbool.h>

#include <padscore/wpad.h>
#include <vpad/input.h>

#ifdef __cplusplus
	extern "C" {
#endif

extern VPADStatus vpad;

typedef enum
{
	CHECK_NONE,			//No check
	CHECK_NUMERICAL,	//Only numbers
	CHECK_HEXADECIMAL,	//Only hex
	CHECK_NOSPECIAL,	//Only letters or numbers
	CHECK_URL,
} KeyboardChecks;

typedef enum
{
	KEYBOARD_TYPE_RESTRICTED,
	KEYBOARD_TYPE_NORMAL
} KeyboardType;

typedef enum
{
	CT_WPAD_0 = WPAD_CHAN_0,
	CT_WPAD_1 = WPAD_CHAN_1,
	CT_WPAD_2 = WPAD_CHAN_2,
	CT_WPAD_3 = WPAD_CHAN_3,
	CT_VPAD_0 = 4 + VPAD_CHAN_0,
} ControllerType;

bool SWKBD_Init();
void SWKBD_Shutdown();

void readInput();
bool showKeyboard(KeyboardType type, char *output, KeyboardChecks check, int maxlength, bool limit, const char *input, const char *okStr);

#ifdef __cplusplus
	}
#endif

#endif //ifndef NUSSPLI_INPUT_H