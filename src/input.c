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

#include <wut-fixups.h>

#include <config.h>
#include <input.h>
#include <renderer.h>
#include <status.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

#include <stdio.h>
#include <uchar.h>

#include <vpad/input.h>
#include <coreinit/dynload.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/thread.h>
#include <padscore/kpad.h>
#include <padscore/wpad.h>

#define CT_STACK_SIZE (256 * 1024)

//WIP. This need a better implementation

VPADStatus vpad;
KPADStatus kpad[4];
ControllerType lastUsedController;

Swkbd_CreateArg createArg;

int globalMaxlength;
bool globalLimit;

bool okButtonEnabled;

static OSThread calcThread;
static void *calcThreadStack;

bool isUrl(char c)
{
	return isNumber(c) || isLowercase(c) || isUppercase(c) || c == '.' || c == '/' || c == ':' || c == '%' || c == '-' || c == '_'; //TODO
}

typedef bool (*checkingFunction)(char);

int calcThreadMain(int argc, const char **argv)
{
	Swkbd_CalcSubThreadFont();
	checkStacks("SWKBD font");
	return 0;
}

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
					case CHECK_ALPHANUMERICAL:
						cf = &isAllowedInFilename;
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

	if(Swkbd_IsNeedCalcSubThreadFont())
	{
		calcThreadStack = MEMAllocFromDefaultHeapEx(CT_STACK_SIZE, 8);
		if(calcThreadStack == NULL)
			debugPrintf("OUT OF MEMORY!"); // TODO
		else
		{
			if(OSCreateThread(&calcThread, calcThreadMain, 0, NULL, calcThreadStack + CT_STACK_SIZE, CT_STACK_SIZE, 0, OS_THREAD_ATTRIB_AFFINITY_CPU1))
			{
				OSSetThreadName(&calcThread, "NUSspli SWKBD font calculator");
				OSResumeThread(&calcThread);
				int ret;
				OSJoinThread(&calcThread, &ret);
			}
		}
	}

	drawKeyboard(lastUsedController != CT_VPAD_0);
}

bool SWKBD_Show(KeyboardLayout layout, KeyboardType type, int maxlength, bool limit, const char *okStr) {
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
	
	appearArg.keyboardArg.configArg.languageType = getKeyboardLanguage();
	switch(appearArg.keyboardArg.configArg.languageType)
	{
		case Swkbd_LanguageType__Japanese:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__Japanese;
			break;
		case Swkbd_LanguageType__French:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__French;
			break;
		case Swkbd_LanguageType__German:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__German;
			break;
		case Swkbd_LanguageType__Italian:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__Italian;
			break;
		case Swkbd_LanguageType__Spanish:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__Spanish;
			break;
		case Swkbd_LanguageType__Dutch:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__Dutch;
			break;
		case Swkbd_LanguageType__Potuguese:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__Portuguese;
			break;
		case Swkbd_LanguageType__Russian:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__Russian;
			break;
		default:
			appearArg.keyboardArg.configArg.languageType2 = Swkbd_LanguageType2__English;
	}

	appearArg.keyboardArg.configArg.unk_0x04 = lastUsedController;
	appearArg.keyboardArg.configArg.unk_0x08 = layout;
	appearArg.keyboardArg.configArg.unk_0x0C = 0xFFFFFFFF;
	appearArg.keyboardArg.configArg.unk_0x14 = -1;
	appearArg.keyboardArg.configArg.str = okStrL;
	appearArg.keyboardArg.configArg.framerate = FRAMERATE_60FPS;
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
	
	OSDynLoadAllocFn oAlloc;
	OSDynLoadFreeFn oFree;
	OSDynLoad_GetAllocator(&oAlloc, &oFree);
	
	OSBlockSet(&createArg, 0, sizeof(Swkbd_CreateArg));
	createArg.fsClient = MEMAllocFromDefaultHeap(sizeof(FSClient));
	if(createArg.fsClient != NULL)
		FSAddClient(createArg.fsClient, 0);
	
	createArg.workMemory = MEMAllocFromDefaultHeap(Swkbd_GetWorkMemorySize(0));
	if(createArg.fsClient == NULL || createArg.workMemory == NULL)
	{
		debugPrintf("SWKBD: Can't allocate memory!");
		return false;
	}
	
	switch(getKeyboardLanguage())
	{
		case Swkbd_LanguageType__Japanese:
			createArg.regionType = Swkbd_RegionType__Japan;
			break;
		case Swkbd_LanguageType__English:
			createArg.regionType = Swkbd_RegionType__USA;
			break;
		case Swkbd_LanguageType__Chinese1:
			createArg.regionType = Swkbd_RegionType__China;
			break;
		case Swkbd_LanguageType__Korean:
			createArg.regionType = Swkbd_RegionType__Korea;
			break;
		case  Swkbd_LanguageType__Chinese2:
			createArg.regionType = Swkbd_RegionType__Taiwan;
			break;
		default:
			createArg.regionType = Swkbd_RegionType__Europe;
			break;
	}
	
	bool ret = Swkbd_Create(createArg);
	OSDynLoad_SetAllocator(oAlloc, oFree);
	return ret;
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
	bool kbdHidden = Swkbd_IsReady() ? Swkbd_IsHidden() : true;
	
	if(vError != VPAD_READ_SUCCESS)
		OSBlockSet(&vpad, 0, sizeof(VPADStatus));
	else if(vpad.trigger != 0 && kbdHidden)
		lastUsedController = CT_VPAD_0;
	
	bool altCon = false;
	uint32_t controllerType;
	int32_t controllerProbe;
	for(int i = 0; i < 4; i++)
	{
		controllerProbe = WPADProbe(i, &controllerType);
		if(controllerProbe == -1)
			continue;
		
		altCon = true;
		
		if(controllerProbe != 0)
			continue;
		
		OSBlockSet(&kpad[i], 0, sizeof(KPADStatus));
		KPADRead(i, &kpad[i], 1);
		
		if(controllerType == WPAD_EXT_PRO_CONTROLLER || // With a simple input like ours we're able to handle Wii u pro as Wii classic controllers.
				controllerType == WPAD_EXT_CLASSIC ||
				controllerType == WPAD_EXT_MPLUS_CLASSIC)
		{
			
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_A)
				vpad.trigger = VPAD_BUTTON_A;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_B)
				vpad.trigger |= VPAD_BUTTON_B;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_X)
				vpad.trigger |= VPAD_BUTTON_X;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_Y)
				vpad.trigger |= VPAD_BUTTON_Y;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_UP)
				vpad.trigger |= VPAD_BUTTON_UP;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_DOWN)
				vpad.trigger |= VPAD_BUTTON_DOWN;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_LEFT)
				vpad.trigger |= VPAD_BUTTON_LEFT;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_RIGHT)
				vpad.trigger |= VPAD_BUTTON_RIGHT;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_PLUS)
				vpad.trigger |= VPAD_BUTTON_PLUS;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_MINUS)
				vpad.trigger |= VPAD_BUTTON_MINUS;
			if(kpad[i].classic.trigger & WPAD_CLASSIC_BUTTON_HOME)
				vpad.trigger |= VPAD_BUTTON_HOME;
			
			if(vpad.trigger != 0)
			{
				if(kbdHidden)
					lastUsedController = i;
				
				continue;
			}
		}
			
		if(kpad[i].trigger & WPAD_BUTTON_A)
			vpad.trigger = VPAD_BUTTON_A;
		if(kpad[i].trigger & WPAD_BUTTON_B)
			vpad.trigger |= VPAD_BUTTON_B;
		if(kpad[i].trigger & WPAD_BUTTON_1)
			vpad.trigger |= VPAD_BUTTON_X;
		if(kpad[i].trigger & WPAD_BUTTON_2)
			vpad.trigger |= VPAD_BUTTON_Y;
		if(kpad[i].trigger & WPAD_BUTTON_UP)
			vpad.trigger |= VPAD_BUTTON_UP;
		if(kpad[i].trigger & WPAD_BUTTON_DOWN)
			vpad.trigger |= VPAD_BUTTON_DOWN;
		if(kpad[i].trigger & WPAD_BUTTON_LEFT)
			vpad.trigger |= VPAD_BUTTON_LEFT;
		if(kpad[i].trigger & WPAD_BUTTON_RIGHT)
			vpad.trigger |= VPAD_BUTTON_RIGHT;
		if(kpad[i].trigger & WPAD_BUTTON_PLUS)
			vpad.trigger |= VPAD_BUTTON_PLUS;
		if(kpad[i].trigger & WPAD_BUTTON_MINUS)
			vpad.trigger |= VPAD_BUTTON_MINUS;
		if(kpad[i].trigger & WPAD_BUTTON_HOME)
			vpad.trigger |= VPAD_BUTTON_HOME;
		
		if(vpad.trigger != 0 && kbdHidden)
			lastUsedController = i;
	}
	
	if(!altCon && vError == VPAD_READ_INVALID_CONTROLLER)
		addErrorOverlay("No Controller connected!");
	else
		removeErrorOverlay();
}

bool showKeyboard(KeyboardLayout layout, KeyboardType type, char *output, KeyboardChecks check, int maxlength, bool limit, const char *input, const char *okStr) {
	debugPrintf("Initialising SWKBD");
	if(!SWKBD_Show(layout, type, maxlength, limit, okStr))
	{
		drawErrorFrame("Error showing SWKBD:\nnn::swkbd::AppearInputForm failed", B_RETURN);
		
		while(true)
		{
			showFrame();							
			if(vpad.trigger & VPAD_BUTTON_B)
				return false;
		}
	}
	debugPrintf("SWKBD initialised successfully");
	
	if(input != NULL)
	{
		if(layout == KEYBOARD_LAYOUT_NORMAL)
			Swkbd_SetInputFormString16((char16_t *)input);
		else
			Swkbd_SetInputFormString(input);
	}
	
	bool dummy;
	while(true)
	{
		VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad.tpFiltered1, &vpad.tpNormal);
		vpad.tpFiltered2 = vpad.tpNormal = vpad.tpFiltered1;
		SWKBD_Render(check);
//		sleepTillFrameEnd();
		
		if(okButtonEnabled && (Swkbd_IsDecideOkButton(&dummy) || (lastUsedController == CT_VPAD_0 && vpad.trigger & VPAD_BUTTON_A)))
		{
			debugPrintf("SWKBD Ok button pressed");
			if(layout == KEYBOARD_LAYOUT_NORMAL)
			{
				const char16_t *outputStr = Swkbd_GetInputFormString16();
				size_t i = 0;
				size_t j = 0;
				do
				{
					output[j++] = (char)(outputStr[i] >> 8);
					output[j++] = (char)(outputStr[i]);
				}
				while(outputStr[i++] != u'\0');
			}
			else
			{
				char *outputStr = Swkbd_GetInputFormString();
				strcpy(output, outputStr);
				MEMFreeToDefaultHeap(outputStr);
			}
			SWKBD_Hide();
			return true;
		}
		
		bool close = vpad.trigger & VPAD_BUTTON_B;
		if(close)
		{
			if(layout == KEYBOARD_LAYOUT_NORMAL)
			{
				const char16_t *inputFormString = Swkbd_GetInputFormString16();
				if(inputFormString != NULL)
					close = strlen16((char16_t *)inputFormString) == 0;
			}
			else
			{
				char *inputFormString = Swkbd_GetInputFormString();
				if(inputFormString != NULL)
				{
					close = strlen(inputFormString) == 0;
					MEMFreeToDefaultHeap(inputFormString);
				}
			}
		}
		
		if(close || Swkbd_IsDecideCancelButton(&dummy)) {
			debugPrintf("SWKBD Cancel button pressed");
			SWKBD_Hide();
			return false;
		}
	}
}
