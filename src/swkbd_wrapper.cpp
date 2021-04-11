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

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <coreinit/memdefaultheap.h>

#include <utils.h>
#include <swkbd_wrapper.h>

bool kbd_initialized = false;

uint32_t Swkbd_GetWorkMemorySize(uint32_t unk)
{
	return nn::swkbd::GetWorkMemorySize(unk);
}

bool Swkbd_AppearInputForm(const Swkbd_AppearArg args)
{
	return nn::swkbd::AppearInputForm(*(nn::swkbd::AppearArg*)&args);
}

bool Swkbd_Create(const Swkbd_CreateArg args)
{
	kbd_initialized = nn::swkbd::Create(*(nn::swkbd::CreateArg*)&args);
	return kbd_initialized;
}

void Swkbd_SetEnableOkButton(bool enable)
{
	nn::swkbd::SetEnableOkButton(enable);
}

void Swkbd_DeleteCppChar(const char *str)
{
	delete str;
}

char *Swkbd_GetInputFormString()
{
	const char16_t *cppRet = nn::swkbd::GetInputFormString();
	if(cppRet == NULL)
		return NULL;
	
	size_t i = 0;
	while(cppRet[i] != u'\0')
		i++;
	
	char *outputStr = (char*)MEMAllocFromDefaultHeap(sizeof(char) * ++i);
	if(outputStr == NULL)
		return NULL;
	
	i = 0;
	do
		outputStr[i] = cppRet[i] > 0x7F ? '?' : (char)cppRet[i];
	while(outputStr[i++] != '\0');
	
	return outputStr;
}

void Swkbd_SetInputFormString(const char *str)
{
	size_t len = strlen(str);
	if(str == NULL || len == 0)
	{
		nn::swkbd::SetInputFormString(u"");
		return;
	}
	
	char16_t cppStr[++len];
	for(size_t i = 0; i < len; i++)
		cppStr[i] = str[i];
		
	nn::swkbd::SetInputFormString(cppStr);
}

void Swkbd_Calc(const Swkbd_ControllerInfo controllerInfo)
{
	nn::swkbd::Calc(*(nn::swkbd::ControllerInfo*)&controllerInfo);
}

bool Swkbd_IsNeedCalcSubThreadFont()
{
	return nn::swkbd::IsNeedCalcSubThreadFont();
}

void Swkbd_CalcSubThreadFont()
{
	nn::swkbd::CalcSubThreadFont();
}

bool Swkbd_IsNeedCalcSubThreadPredict()
{
	return nn::swkbd::IsNeedCalcSubThreadPredict();
}

void Swkbd_CalcSubThreadPredict()
{
	nn::swkbd::CalcSubThreadPredict();
}

void Swkbd_DrawTV()
{
	nn::swkbd::DrawTV();
}

void Swkbd_DrawDRC()
{
	nn::swkbd::DrawDRC();
}

bool Swkbd_IsDecideOkButton(bool *outIsSelected)
{
	return nn::swkbd::IsDecideOkButton(outIsSelected);
}

bool Swkbd_IsDecideCancelButton(bool *outIsSelected)
{
	return nn::swkbd::IsDecideCancelButton(outIsSelected);
}

bool Swkbd_DisappearInputForm()
{
	return nn::swkbd::DisappearInputForm();
}

void Swkbd_Destroy()
{
	nn::swkbd::Destroy();
	kbd_initialized = false;
}

bool Swkbd_IsHidden()
{
	return nn::swkbd::GetStateInputForm() == nn::swkbd::State::Hidden;
}

bool Swkbd_IsReady()
{
	return kbd_initialized;
}
