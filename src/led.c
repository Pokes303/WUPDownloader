/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2022 V10lator <v10lator@myway.de>                         *
 * Copyright (c) 2022 Xpl0itU <DaThinkingChair@protonmail.com>             *
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

#include <stdint.h>

#include <nn/acp/drcled_c.h>

#include <config.h>
#include <osdefs.h>

static uint32_t pId;

void initLED()
{
	pId = GetPersistentId();
}

void startLED()
{
	if(getNewsMethod() & NEWS_METHOD_LED)
		ACPTurnOnDrcLed(pId, 1);
}

void stopLED()
{
	if(getNewsMethod() & NEWS_METHOD_LED)
		ACPTurnOnDrcLed(pId, 0);
}
