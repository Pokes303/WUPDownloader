/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020 V10lator <v10lator@myway.de>                         *
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

#include <input.h>
#include <renderer.h>
#include <status.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

#include <stdio.h>
#include <uchar.h>

#include <vpad/input.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <padscore/kpad.h>
#include <padscore/wpad.h>

//WIP. This need a better implementation

VPADStatus vpad;
KPADStatus kpad[4];
ControllerType lastUsedController;

Swkbd_CreateArg createArg;

int globalMaxlength;
bool globalLimit;

bool okButtonEnabled;

bool isUrl(char c)
{
	return isNumber(c) || isLowercase(c) || isUppercase(c) || c == '.' || c == '/' || c == ':' || c == '%' || c == '-' || c == '_'; //TODO
}

typedef bool (*checkingFunction)(char);

void SWKBD_Render(KeyboardChecks check)
{
	if(globalMaxlength != -1)
	{
		char *inputFormString = Swkbd_GetInputFormString();
		if(inputFormString != NULL)
		{
			size_t len = strlen(inputFormString);
			if(len != 0 && check != CHECK_NONE && check != CHECK_NUMERICAL)
			{
				checkingFunction cf;
				switch(check)
				{
					case CHECK_HEXADECIMAL:
						cf = &isHexa;
						break;
					case CHECK_NOSPECIAL:
						cf = &isSpecial;
						break;
					case CHECK_URL:
						cf = &isUrl;
						break;
					default:
						// DEAD CODE
						debugPrintf("0xDEADC0DE: %d", check);
						MEMFreeToDefaultHeap(inputFormString);
						return;
				}
				
				for(len = 0; inputFormString[len] != '\0'; len++)
					if(!cf(inputFormString[len]))
					{
						inputFormString[len] = '\0';
						Swkbd_SetInputFormString(inputFormString);
						break;
					}
			}
			
			MEMFreeToDefaultHeap(inputFormString);
			okButtonEnabled = globalLimit ? len == globalMaxlength : len <= globalMaxlength;
		}
		else
			okButtonEnabled = false;
		
		Swkbd_SetEnableOkButton(okButtonEnabled);
	}
	
	Swkbd_ControllerInfo controllerInfo;
	controllerInfo.vpad = &vpad;
	for(int i = 0; i < 4; i++)
		controllerInfo.kpad[i] = &kpad[i];
	
	Swkbd_Calc(controllerInfo); //TODO: Do this in a new thread?

	if (Swkbd_IsNeedCalcSubThreadFont())
	{
		Swkbd_CalcSubThreadFont(); //TODO: Do this in a new thread?
		debugPrintf("SWKBD nn::swkbd::IsNeedCalcSubThreadFont()");
	}

	drawKeyboard(lastUsedController != CT_VPAD_0);
}

bool SWKBD_Show(KeyboardType type, int maxlength, bool limit, const char *okStr) {
	debugPrintf("SWKBD_Show()");
	if(!Swkbd_IsHidden())
	{
		debugPrintf("...while visible!!!");
		return false;
	}
	
	char16_t *okStrL;
	if(okStr == NULL)
		okStrL = NULL;
	else
	{
		size_t strLen = strlen(okStr) + 1;
		okStrL = MEMAllocFromDefaultHeap(strLen);
		if(okStrL != NULL)
			for(size_t i = 0; i < strLen; i++)
				okStrL[i] = okStr[i];
	}
	
	// Show the keyboard
	Swkbd_AppearArg appearArg;
	
	OSBlockSet(&appearArg.keyboardArg.configArg, 0, sizeof(Swkbd_ConfigArg));
	OSBlockSet(&appearArg.keyboardArg.receiverArg, 0, sizeof(Swkbd_ReceiverArg));
	
	appearArg.keyboardArg.configArg.languageType = Swkbd_LanguageType__English;
	appearArg.keyboardArg.configArg.unk_0x04 = lastUsedController;
	appearArg.keyboardArg.configArg.unk_0x08 = 2;
	appearArg.keyboardArg.configArg.unk_0x0C = 2;
	appearArg.keyboardArg.configArg.unk_0x10 = 2;
	appearArg.keyboardArg.configArg.unk_0x14 = -1;
	appearArg.keyboardArg.configArg.str = okStrL;
	appearArg.keyboardArg.configArg.framerate = FRAMERATE_30FPS;
	appearArg.keyboardArg.configArg.showCursor = true;
	appearArg.keyboardArg.configArg.unk_0xA4 = -1;
	
	appearArg.keyboardArg.receiverArg.unk_0x14 = 2;
	
	appearArg.inputFormArg.unk_0x00 = type;
	appearArg.inputFormArg.unk_0x04 = -1;
	appearArg.inputFormArg.unk_0x08 = 0;
	appearArg.inputFormArg.unk_0x0C = 0;
	appearArg.inputFormArg.unk_0x14 = 0;
	appearArg.inputFormArg.unk_0x18 = 0x00008000; // Monospace seperator after 16 chars (for 32 char keys)
	appearArg.inputFormArg.unk_0x1C = true;
	appearArg.inputFormArg.unk_0x1D = true;
	appearArg.inputFormArg.unk_0x1E = true;
	globalMaxlength = appearArg.inputFormArg.maxTextLength = maxlength;
	
	bool kbdVisible = Swkbd_AppearInputForm(appearArg);
	debugPrintf("Swkbd_AppearInputForm(): %s", kbdVisible ? "true" : "false");
	
	if(okStrL != NULL)
		MEMFreeToDefaultHeap(okStrL);
	
	if(!kbdVisible)
		return false;
	
	globalLimit = limit;
	okButtonEnabled = limit || maxlength == -1;
	Swkbd_SetEnableOkButton(okButtonEnabled);
	
	VPADSetSensorBar(VPAD_CHAN_0, true);
	
	debugPrintf("nn::swkbd::AppearInputForm success");
	return true;
}

void SWKBD_Hide()
{
	debugPrintf("SWKBD_Hide()");
	if(Swkbd_IsHidden())
	{
		debugPrintf("...while invisible!!!");
		return;
	}
	
	VPADSetSensorBar(VPAD_CHAN_0, false);
	Swkbd_DisappearInputForm();
	
	// DisappearInputForm() wants to render a fade out animation
	while(!Swkbd_IsHidden())
		SWKBD_Render(CHECK_NONE);
}

bool SWKBD_Init()
{
	debugPrintf("SWKBD_Init()");
	createArg.fsClient = MEMAllocFromDefaultHeap(sizeof(FSClient));
	createArg.workMemory = MEMAllocFromDefaultHeap(Swkbd_GetWorkMemorySize(0));
	if(createArg.fsClient == NULL || createArg.workMemory == NULL)
	{
		debugPrintf("SWKBD: Can't allocate memory!");
		return false;
	}
	
	FSAddClient(createArg.fsClient, 0);
	
	createArg.regionType = Swkbd_RegionType__Europe;
	createArg.unk_0x08 = 0;
	
	return Swkbd_Create(createArg);
}

void SWKBD_Shutdown()
{
	if(createArg.workMemory != NULL)
	{
		Swkbd_Destroy();
		MEMFreeToDefaultHeap(createArg.workMemory);
		createArg.workMemory = NULL;
	}
	
	if(createArg.fsClient != NULL)
	{
		FSDelClient(createArg.fsClient, 0);
		MEMFreeToDefaultHeap(createArg.fsClient);
		createArg.fsClient = NULL;
	}
}

void readInput()
{
	VPADReadError vError;
	VPADRead(VPAD_CHAN_0, &vpad, 1, &vError);
	if(vError != VPAD_READ_SUCCESS)
		OSBlockSet(&vpad, 0, sizeof(VPADStatus));
	else if(vpad.trigger != 0 && Swkbd_IsHidden())
		lastUsedController = CT_VPAD_0;
	
	bool altCon = false;
	uint32_t controllerType;
	for(int i = 0; i < 4; i++)
	{
		if(WPADProbe(i, &controllerType) != 0)
			continue;
		
		OSBlockSet(&kpad[i], 0, sizeof(KPADStatus));
		KPADRead(i, &kpad[i], 1);
		
		if(vpad.trigger == 0)
		{
			altCon = true;
			if(controllerType == WPAD_EXT_PRO_CONTROLLER || // With a simple input like ours we're able to handle Wii u pro as Wii classic controllers.
					controllerType == WPAD_EXT_CLASSIC ||
					controllerType == WPAD_EXT_MPLUS_CLASSIC)
			{
				switch(kpad[i].classic.trigger)
				{
					case WPAD_CLASSIC_BUTTON_A:
						vpad.trigger = VPAD_BUTTON_A;
						break;
					case WPAD_CLASSIC_BUTTON_B:
						vpad.trigger = VPAD_BUTTON_B;
						break;
					case WPAD_CLASSIC_BUTTON_X:
						vpad.trigger = VPAD_BUTTON_X;
						break;
					case WPAD_CLASSIC_BUTTON_Y:
						vpad.trigger = VPAD_BUTTON_Y;
						break;
					case WPAD_CLASSIC_BUTTON_UP:
						vpad.trigger = VPAD_BUTTON_UP;
						break;
					case WPAD_CLASSIC_BUTTON_DOWN:
						vpad.trigger = VPAD_BUTTON_DOWN;
						break;
					case WPAD_CLASSIC_BUTTON_LEFT:
						vpad.trigger = VPAD_BUTTON_LEFT;
						break;
					case WPAD_CLASSIC_BUTTON_RIGHT:
						vpad.trigger = VPAD_BUTTON_RIGHT;
						break;
					case WPAD_CLASSIC_BUTTON_PLUS:
						vpad.trigger = VPAD_BUTTON_PLUS;
						break;
					case WPAD_CLASSIC_BUTTON_MINUS:
						vpad.trigger = VPAD_BUTTON_MINUS;
						break;
					case WPAD_CLASSIC_BUTTON_HOME:
						vpad.trigger = VPAD_BUTTON_HOME;
						break;
				}
				
				if(vpad.trigger != 0)
				{
					if(Swkbd_IsHidden())
						lastUsedController = i;
					
					continue;
				}
			}
				
			switch(kpad[i].trigger)
			{
				case WPAD_BUTTON_A:
					vpad.trigger = VPAD_BUTTON_A;
					break;
				case WPAD_BUTTON_B:
					vpad.trigger = VPAD_BUTTON_B;
					break;
				case WPAD_BUTTON_1:
					vpad.trigger = VPAD_BUTTON_X;
					break;
				case WPAD_BUTTON_2:
					vpad.trigger = VPAD_BUTTON_Y;
					break;
				case WPAD_BUTTON_UP:
					vpad.trigger = VPAD_BUTTON_UP;
					break;
				case WPAD_BUTTON_DOWN:
					vpad.trigger = VPAD_BUTTON_DOWN;
					break;
				case WPAD_BUTTON_LEFT:
					vpad.trigger = VPAD_BUTTON_LEFT;
					break;
				case WPAD_BUTTON_RIGHT:
					vpad.trigger = VPAD_BUTTON_RIGHT;
					break;
				case WPAD_BUTTON_PLUS:
					vpad.trigger = VPAD_BUTTON_PLUS;
					break;
				case WPAD_BUTTON_MINUS:
					vpad.trigger = VPAD_BUTTON_MINUS;
					break;
				case WPAD_BUTTON_HOME:
					vpad.trigger = VPAD_BUTTON_HOME;
			}
			
			if(vpad.trigger != 0 && Swkbd_IsHidden())
				lastUsedController = i;
		}
	}
	
	if(!altCon && vError == VPAD_READ_INVALID_CONTROLLER)
		addErrorOverlay("No Controller connected!");
	else
		removeErrorOverlay();
}

bool showKeyboard(KeyboardType type, char *output, KeyboardChecks check, int maxlength, bool limit, const char *input, const char *okStr) {
	debugPrintf("Initialising SWKBD");
	if(!SWKBD_Show(type, maxlength, limit, okStr))
	{
		drawErrorFrame("Error showing SWKBD:\nnn::swkbd::AppearInputForm failed", B_RETURN);
		
		while(true)
		{
			showFrame();							
			if(vpad.trigger == VPAD_BUTTON_B)
				return false;
		}
	}
	debugPrintf("SWKBD initialised successfully");
	
	if(input != NULL)
		Swkbd_SetInputFormString(input);
	
	bool dummy;
	while(true)
	{
		VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad.tpFiltered1, &vpad.tpNormal);
		vpad.tpFiltered2 = vpad.tpNormal = vpad.tpFiltered1;
		SWKBD_Render(check);
//		sleepTillFrameEnd();
		
		if(okButtonEnabled && (Swkbd_IsDecideOkButton(&dummy) || vpad.trigger == VPAD_BUTTON_A))
		{
			debugPrintf("SWKBD Ok button pressed");
			char *outputStr = Swkbd_GetInputFormString();
			strcpy(output, outputStr);
			MEMFreeToDefaultHeap(outputStr);
			SWKBD_Hide();
			return true;
		}
		
		bool close = vpad.trigger == VPAD_BUTTON_B;
		if(close)
		{
			char *inputFormString = Swkbd_GetInputFormString();
			if(inputFormString != NULL)
			{
				close = strlen(inputFormString) == 0;
				MEMFreeToDefaultHeap(inputFormString);
			}
		}
		
		if(close || Swkbd_IsDecideCancelButton(&dummy)) {
			debugPrintf("SWKBD Cancel button pressed");
			SWKBD_Hide();
			return false;
		}
	}
}
