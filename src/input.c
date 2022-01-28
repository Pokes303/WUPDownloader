/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
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

#include <config.h>
#include <crypto.h>
#include <input.h>
#include <messages.h>
#include <renderer.h>
#include <status.h>
#include <swkbd_wrapper.h>
#include <thread.h>
#include <utils.h>
#include <menu/utils.h>

#include <stdio.h>
#include <uchar.h>

#include <vpad/input.h>
#include <coreinit/dynload.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <coreinit/messagequeue.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <padscore/kpad.h>
#include <padscore/wpad.h>

#define CT_STACK_SIZE (256 * 1024)
#define SWKBD_QUEUE_SIZE MIN_STACKSIZE

//WIP. This need a better implementation

VPADStatus vpad;
KPADStatus kpad[4];
ControllerType lastUsedController;

int io = -1;

Swkbd_CreateArg createArg;
FSClient swkbd_fsc;

OSMessageQueue swkbd_queue;
OSMessage swkbd_msg[SWKBD_QUEUE_SIZE];

static OSTime lastButtonPress = 0;

typedef struct
{
	int globalMaxlength;
	bool globalLimit;
	bool okButtonEnabled;
	OSThread *calcThread;
} SWKBD_Args;

static bool isUrl(char c)
{
	return isNumber(c) || isLowercase(c) || isUppercase(c) || c == '.' || c == '/' || c == ':' || c == '%' || c == '-' || c == '_'; //TODO
}

typedef bool (*checkingFunction)(char);

static int calcThreadMain(int argc, const char **argv)
{
	OSMessage msg;
	do
	{
		OSReceiveMessage(&swkbd_queue, &msg, OS_MESSAGE_FLAGS_BLOCKING);
		if(msg.message == NUSSPLI_MESSAGE_NONE)
			Swkbd_CalcSubThreadFont();
		checkStacks("SWKBD font");
	}
	while(msg.message != NUSSPLI_MESSAGE_EXIT);
	
	return 0;
}

static bool isH(char c)
{
    return isHexa(c);
}

static bool isA(char c)
{
    return isAllowedInFilename(c);
}

static void SWKBD_Render(SWKBD_Args *args, KeyboardChecks check)
{
	if(args->globalMaxlength != -1)
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
						cf = &isH;
						break;
					case CHECK_ALPHANUMERICAL:
						cf = &isA;
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
				
				for(len = 0; inputFormString[len] != '\0'; ++len)
					if(!cf(inputFormString[len]))
					{
						inputFormString[len] = '\0';
						Swkbd_SetInputFormString(inputFormString);
						break;
					}
			}
			
			MEMFreeToDefaultHeap(inputFormString);
			args->okButtonEnabled = args->globalLimit ? len == args->globalMaxlength : len <= args->globalMaxlength;
		}
		else
			args->okButtonEnabled = false;
		
		Swkbd_SetEnableOkButton(args->okButtonEnabled);
	}
	
	Swkbd_ControllerInfo controllerInfo = { .vpad = &vpad };
	KPADStatus *ksp = kpad;
	--ksp;
	KPADStatus **ciksp = controllerInfo.kpad;
	--ciksp;
	for(int i = 4; i; --i)
		*++ciksp = ++ksp;
	
	Swkbd_Calc(controllerInfo);

	if(Swkbd_IsNeedCalcSubThreadFont())
	{
		OSMessage msg = { .message = NUSSPLI_MESSAGE_NONE };
		OSSendMessage(&swkbd_queue, &msg, OS_MESSAGE_FLAGS_NONE);
	}

	drawKeyboard(lastUsedController != CT_VPAD_0);
}

static bool SWKBD_Show(SWKBD_Args *args, KeyboardLayout layout, KeyboardType type, int maxlength, bool limit, const char *okStr) {
	debugPrintf("SWKBD_Show()");
	if(!Swkbd_IsHidden())
	{
		debugPrintf("...while visible!!!");
		return false;
	}

    args->calcThread = startThread("NUSspli SWKBD font calculator", THREAD_PRIORITY_MEDIUM, CT_STACK_SIZE, calcThreadMain, 0, NULL, OS_THREAD_ATTRIB_AFFINITY_ANY);
	if(args->calcThread == NULL)
    {
		debugPrintf("SWKBD: Can't spawn calc thread!");
		return false;
    }
	
	char16_t *okStrL;
	if(okStr == NULL)
		okStrL = NULL;
	else
	{
		size_t strLen = strlen(okStr);
		okStrL = MEMAllocFromDefaultHeap(++strLen);
		if(okStrL)
			for(size_t i = 0; i < strLen; ++i)
				okStrL[i] = okStr[i];
	}
	
	// Show the keyboard
	Swkbd_AppearArg appearArg;
	
	OSBlockSet(&appearArg, 0, sizeof(Swkbd_AppearArg));
	
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
	args->globalMaxlength = appearArg.inputFormArg.maxTextLength = maxlength;
	
	bool kbdVisible = Swkbd_AppearInputForm(appearArg);
	debugPrintf("Swkbd_AppearInputForm(): %s", kbdVisible ? "true" : "false");
	
	if(okStrL != NULL)
		MEMFreeToDefaultHeap(okStrL);
	
	if(!kbdVisible)
		return false;
	
	args->globalLimit = limit;
	args->okButtonEnabled = limit || maxlength == -1;
	Swkbd_SetEnableOkButton(args->okButtonEnabled);
	
	VPADSetSensorBar(VPAD_CHAN_0, true);
	
	debugPrintf("nn::swkbd::AppearInputForm success");
	return true;
}

static void SWKBD_Hide(SWKBD_Args *args)
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
		SWKBD_Render(args, CHECK_NONE);

    OSMessage msg = { .message = NUSSPLI_MESSAGE_EXIT };
	OSSendMessage(&swkbd_queue, &msg, OS_MESSAGE_FLAGS_BLOCKING);
	stopThread(args->calcThread, NULL);
}

bool SWKBD_Init()
{
	debugPrintf("SWKBD_Init()");
	
	createArg.workMemory = MEMAllocFromDefaultHeap(Swkbd_GetWorkMemorySize(0));
	if(createArg.workMemory == NULL)
	{
		debugPrintf("SWKBD: Can't allocate memory!");
		return false;
	}

	OSBlockSet(swkbd_msg, 0, sizeof(OSMessage) * SWKBD_QUEUE_SIZE);
	OSInitMessageQueueEx(&swkbd_queue, swkbd_msg, SWKBD_QUEUE_SIZE, "NUSspli SWKBD calc queue");
	
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
	
	createArg.unk_0x08 = 0;
	createArg.fsClient = &swkbd_fsc;
	FSAddClient(createArg.fsClient, 0);
	OSDynLoadAllocFn oAlloc;
	OSDynLoadFreeFn oFree;
	OSDynLoad_GetAllocator(&oAlloc, &oFree);
	bool ret = Swkbd_Create(createArg);
	OSDynLoad_SetAllocator(oAlloc, oFree);
	
	return ret;
}

void SWKBD_Shutdown()
{
	if(createArg.workMemory)
	{
		Swkbd_Destroy();
		MEMFreeToDefaultHeap(createArg.workMemory);
		createArg.workMemory = NULL;
		FSDelClient(createArg.fsClient, 0);
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
	{
		lastUsedController = CT_VPAD_0;
		debugPrintf("INPUT: DRC: %d", vpad.trigger);
	}
	
	bool altCon = false;
	uint32_t controllerType;
	int32_t controllerProbe;
	uint32_t oldV;
	uint32_t tv;
	KPADStatus *kps = kpad + 4;
	int i = 4;
	while(i)
	{
		--kps;
		controllerProbe = WPADProbe(--i, &controllerType);
		if(controllerProbe == 0)
		{
			altCon = true;
			KPADRead(i, kps, 1);
		}
		else
		{
			if(controllerProbe != -1)
				altCon = true;

			OSBlockSet(kps, 0, sizeof(KPADStatus));
			continue;
		}
		
		if(controllerType == WPAD_EXT_PRO_CONTROLLER || // With a simple input like ours we're able to handle Wii u pro as Wii classic controllers.
				controllerType == WPAD_EXT_CLASSIC ||
				controllerType == WPAD_EXT_MPLUS_CLASSIC)
		{
			oldV = vpad.trigger;
			tv = kps->classic.trigger;
			if(tv)
				debugPrintf("INPUT: WPAD_CLASSIC: %d", tv);

			if(tv & WPAD_CLASSIC_BUTTON_A)
				vpad.trigger |= VPAD_BUTTON_A;
			if(tv & WPAD_CLASSIC_BUTTON_B)
				vpad.trigger |= VPAD_BUTTON_B;
			if(tv & WPAD_CLASSIC_BUTTON_X)
				vpad.trigger |= VPAD_BUTTON_X;
			if(tv & WPAD_CLASSIC_BUTTON_Y)
				vpad.trigger |= VPAD_BUTTON_Y;
			if(tv & WPAD_CLASSIC_BUTTON_UP)
				vpad.trigger |= VPAD_BUTTON_UP;
			if(tv & WPAD_CLASSIC_BUTTON_DOWN)
				vpad.trigger |= VPAD_BUTTON_DOWN;
			if(tv & WPAD_CLASSIC_BUTTON_LEFT)
				vpad.trigger |= VPAD_BUTTON_LEFT;
			if(tv & WPAD_CLASSIC_BUTTON_RIGHT)
				vpad.trigger |= VPAD_BUTTON_RIGHT;
			if(tv & WPAD_CLASSIC_BUTTON_PLUS)
				vpad.trigger |= VPAD_BUTTON_PLUS;
			if(tv & WPAD_CLASSIC_BUTTON_MINUS)
				vpad.trigger |= VPAD_BUTTON_MINUS;
			if(tv & WPAD_CLASSIC_BUTTON_HOME)
				vpad.trigger |= VPAD_BUTTON_HOME;
			if(tv & WPAD_CLASSIC_BUTTON_R)
				vpad.trigger |= VPAD_BUTTON_R;
			if(tv & WPAD_CLASSIC_BUTTON_L)
				vpad.trigger |= VPAD_BUTTON_L;
			if(tv & WPAD_CLASSIC_BUTTON_ZR)
				vpad.trigger |= VPAD_BUTTON_ZR;
			if(tv & WPAD_CLASSIC_BUTTON_ZL)
				vpad.trigger |= VPAD_BUTTON_ZL;
			
			if(vpad.trigger != 0)
			{
				if(kbdHidden && vpad.trigger != oldV)
					lastUsedController = i;
				
				continue;
			}
		}

		tv = kps->trigger;
		if(tv)
			debugPrintf("INPUT: WPAD: %d", tv);

		if(tv & WPAD_BUTTON_A)
			vpad.trigger |= VPAD_BUTTON_A;
		if(tv & WPAD_BUTTON_B)
			vpad.trigger |= VPAD_BUTTON_B;
		if(tv & WPAD_BUTTON_1)
			vpad.trigger |= VPAD_BUTTON_X;
		if(tv & WPAD_BUTTON_2)
			vpad.trigger |= VPAD_BUTTON_Y;
		if(tv & WPAD_BUTTON_UP)
			vpad.trigger |= VPAD_BUTTON_UP;
		if(tv & WPAD_BUTTON_DOWN)
			vpad.trigger |= VPAD_BUTTON_DOWN;
		if(tv & WPAD_BUTTON_LEFT)
			vpad.trigger |= VPAD_BUTTON_LEFT;
		if(tv & WPAD_BUTTON_RIGHT)
			vpad.trigger |= VPAD_BUTTON_RIGHT;
		if(tv & WPAD_BUTTON_PLUS)
			vpad.trigger |= VPAD_BUTTON_PLUS;
		if(tv & WPAD_BUTTON_MINUS)
			vpad.trigger |= VPAD_BUTTON_MINUS;
		if(tv & WPAD_BUTTON_HOME)
			vpad.trigger |= VPAD_BUTTON_HOME;
		
		if(vpad.trigger != 0 && kbdHidden && vpad.trigger != oldV)
			lastUsedController = i;
	}

	if(vpad.trigger != 0)
	{
		OSTime t = OSGetSystemTime() - lastButtonPress;
		addEntropy(&t, sizeof(OSTime));
		lastButtonPress = t;

	}
	
	if(!altCon && vError == VPAD_READ_INVALID_CONTROLLER)
	{
		if(io < 0)
			io = addErrorOverlay("No Controller connected!");
	}
	else if(io >= 0)
	{
		removeErrorOverlay(io);
		io = -1;
	};
}

bool showKeyboard(KeyboardLayout layout, KeyboardType type, char *output, KeyboardChecks check, int maxlength, bool limit, const char *input, const char *okStr) {
	debugPrintf("Initialising SWKBD");

	SWKBD_Args args;

	if(!SWKBD_Show(&args, layout, type, maxlength, limit, okStr))
	{
		drawErrorFrame("Error showing SWKBD:\nnn::swkbd::AppearInputForm failed", ANY_RETURN);
		
		while(true)
		{
			showFrame();							
			if(vpad.trigger)
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
	OSTime t = OSGetSystemTime();
	while(true)
	{
		VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad.tpFiltered1, &vpad.tpNormal);
		vpad.tpFiltered2 = vpad.tpNormal = vpad.tpFiltered1;
		SWKBD_Render(&args, check);
//		sleepTillFrameEnd();
		
		if(args.okButtonEnabled && (Swkbd_IsDecideOkButton(&dummy) || (lastUsedController == CT_VPAD_0 && vpad.trigger & VPAD_BUTTON_A) || vpad.trigger & VPAD_BUTTON_PLUS))
		{
			debugPrintf("SWKBD Ok button pressed");
			if(layout == KEYBOARD_LAYOUT_NORMAL)
			{
				const char16_t *outputStr = Swkbd_GetInputFormString16();
				size_t i = 0;
				size_t j = -1;
				do
				{
					output[++j] = (char)(outputStr[i] >> 8);
					output[++j] = (char)(outputStr[i]);
				}
				while(outputStr[i++]);
			}
			else
			{
				char *outputStr = Swkbd_GetInputFormString();
				strcpy(output, outputStr);
				MEMFreeToDefaultHeap(outputStr);
			}
			SWKBD_Hide(&args);
            t = OSGetSystemTime() - t;
			addEntropy(&t, sizeof(OSTime));
			return true;
		}
		
		bool close = vpad.trigger & VPAD_BUTTON_B || vpad.trigger & VPAD_BUTTON_MINUS;
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
			SWKBD_Hide(&args);
            t = OSGetSystemTime() - t;
			addEntropy(&t, sizeof(OSTime));
			return false;
		}
	}
}
