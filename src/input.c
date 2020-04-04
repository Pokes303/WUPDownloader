#include "input.h"
#include "main.h"
#include "screen.h"
#include "swkbd_wrapper.h"
#include "utils.h"

#include <stdio.h>
 
#include <vpad/input.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <coreinit/memdefaultheap.h>

//WIP. This need a better implementation

FSClient* swkbdCli;
Swkbd_CreateArg *createArg = NULL;
Swkbd_AppearArg *appearArg = NULL;
Swkbd_ControllerInfo *controllerInfo = NULL;

int globalMaxlength;
bool globalLimit;

char* outputStr = NULL;

bool showed;

bool SWKBD_Init() {
	swkbdCli = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
	FSAddClient(swkbdCli, 0);
	
	createArg = MEMAllocFromDefaultHeap(sizeof(Swkbd_CreateArg));
	if(createArg == NULL)
		return false;
	
	createArg->workMemory = MEMAllocFromDefaultHeap(Swkbd_GetWorkMemorySize(0));
	if(createArg->workMemory == NULL)
		return false;
	
	appearArg = MEMAllocFromDefaultHeap(sizeof(Swkbd_CreateArg));
	if(appearArg == NULL)
		return false;
	
	controllerInfo = MEMAllocFromDefaultHeap(sizeof(Swkbd_ControllerInfo));
	if(controllerInfo == NULL)
		return false;
	
	createArg->regionType = Swkbd_RegionType__Europe;
	createArg->unk_0x08 = 0;
	createArg->fsClient = swkbdCli;
	
	appearArg->keyboardArg.configArg.languageType = Swkbd_LanguageType__English;
	appearArg->keyboardArg.configArg.unk_0x04 = 4;
	appearArg->keyboardArg.configArg.unk_0x08 = 0;
	appearArg->keyboardArg.configArg.unk_0x0C = 0x7FFFF;
	appearArg->keyboardArg.configArg.unk_0x10 = 19;
	appearArg->keyboardArg.configArg.unk_0x14 = -1;
	appearArg->keyboardArg.configArg.unk_0x9C = 1;
	appearArg->keyboardArg.configArg.unk_0xA4 = 1;
	
	appearArg->keyboardArg.receiverArg.unk_0x00 = 0;
	appearArg->keyboardArg.receiverArg.unk_0x04 = 0;
	appearArg->keyboardArg.receiverArg.unk_0x08 = 0;
	appearArg->keyboardArg.receiverArg.unk_0x0C = -1;
	appearArg->keyboardArg.receiverArg.unk_0x10 = 0;
	appearArg->keyboardArg.receiverArg.unk_0x14 = -1;
	
	appearArg->inputFormArg.unk_0x00 = -1;
	appearArg->inputFormArg.unk_0x04 = -1;
	appearArg->inputFormArg.unk_0x08 = 0;
	appearArg->inputFormArg.unk_0x0C = 0;
	appearArg->inputFormArg.unk_0x14 = 0;
	appearArg->inputFormArg.unk_0x18 = 0;
	appearArg->inputFormArg.unk_0x1C = false;
	appearArg->inputFormArg.unk_0x1D = false;
	appearArg->inputFormArg.unk_0x1E = false;
	
	controllerInfo->kpad[0] =
		controllerInfo->kpad[1] = 
		controllerInfo->kpad[2] = 
		controllerInfo->kpad[3] = NULL; //TODO: Real kPad support?
	
	WHBGfxInit();

	if (!Swkbd_Create(*createArg)) {
		WHBLogPrintf("nn::swkbd::Create failed");
		WHBGfxShutdown();
		return false;
	}
	WHBLogPrintf("nn::swkbd::Create success");
	WHBGfxShutdown();
	return true;
}
bool SWKBD_Show(int maxlength, bool limit) {
	SWKBD_CleanupText();
	showed = true;
	
	WHBLogPrintf("WHBGfxInit(): %d", WHBGfxInit());
	// Show the keyboard
	globalMaxlength = appearArg->inputFormArg.maxTextLength = maxlength;
	if (!Swkbd_AppearInputForm(*appearArg)) {
		WHBLogPrintf("nn::swkbd::AppearInputForm failed");
		WHBGfxShutdown();
		return false;
	}
	globalLimit = limit;
	if (!limit && maxlength != -1)
		Swkbd_SetEnableOkButton(false);
	WHBLogPrintf("nn::swkbd::AppearInputForm success");
	return true;
}
void SWKBD_Render(VPADStatus* vpad) {
	if (globalMaxlength != -1) {
		char *inputFormString = Swkbd_GetInputFormString();
		if(inputFormString != NULL)
		{
			uint32_t len = strlen(inputFormString);
			MEMFreeToDefaultHeap(inputFormString);
			Swkbd_SetEnableOkButton((globalLimit) ? (len == (uint32_t)globalMaxlength) : (len <= (uint32_t)globalMaxlength));
		}
	}
	
	controllerInfo->vpad = vpad;
	Swkbd_Calc(*controllerInfo); //TODO: Do this in a new thread?

	if (Swkbd_IsNeedCalcSubThreadFont())
	{
		Swkbd_CalcSubThreadFont(); //TODO: Do this in a new thread?
		WHBLogPrintf("SWKBD nn::swkbd::IsNeedCalcSubThreadFont()");
	}

	WHBGfxBeginRender();

	WHBGfxBeginRenderTV();
	WHBGfxClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	Swkbd_DrawTV();
	WHBGfxFinishRenderTV();
	
	WHBGfxBeginRenderDRC();
	WHBGfxClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	Swkbd_DrawDRC();
	WHBGfxFinishRenderDRC();
	
	WHBGfxFinishRender();
}
void SWKBD_Hide(VPADStatus* vpad) {
	for (int i = 0; i < 14; i++) {
		SWKBD_Render(vpad);
	}
	WHBGfxShutdown();
}
bool SWKBD_IsOkButton() {
	bool ret = Swkbd_IsDecideOkButton(NULL);
	if(ret)
		Swkbd_DisappearInputForm();
	return ret;
}
bool SWKBD_IsCancelButton() {
	bool ret = Swkbd_IsDecideCancelButton(NULL);
	if(ret)
		Swkbd_DisappearInputForm();
	return ret;
}
const char* SWKBD_GetError(KeyboardChecks check) {
	char* output = Swkbd_GetInputFormString();
	if (!output)
		return "nn::swkbd::GetInputFormString returned NULL";
	
	size_t len = strlen(output);
	
	if (globalMaxlength != -1 && globalLimit && len != globalMaxlength)
	{
		MEMFreeToDefaultHeap(output);
		return "Input size > globalMaxlength";
	}
	
	if(check != CHECK_NONE)
	{
		for(int i = 0; i < len; i++)
		{
			switch(check)
			{
				case CHECK_NUMERICAL:
					if(!(output[i] >= '0' && output[i] <= '9'))
					{
						MEMFreeToDefaultHeap(output);
						return "The wrote string must be only numerical [0->9]";
					}
					break;
				case CHECK_HEXADECIMAL:
					if(!((output[i] >= '0' && output[i] <= '9') || (output[i] >= 'A' && output[i] <= 'F') || (output[i] >= 'a' && output[i] <= 'f')))
					{
						MEMFreeToDefaultHeap(output);
						return "The wrote string must be only hexadecimal [0->F]";
					}
					break;
				case CHECK_NOSPECIAL:
					if(!((output[i] >= '0' && output[i] <= '9') || (output[i] >= 'A' && output[i] <= 'Z') || (output[i] >= 'a' && output[i] <= 'z') || output[i] == ' '))
					{
						MEMFreeToDefaultHeap(output);
						return "The wrote string must not have special characters [A->Z->9->Space]";
					}
					break;
			}
		}
	}
	
	WHBLogPrintf("Input string: %s", output);
	SWKBD_CleanupText() ;
	outputStr = output;
	return NULL;
}
char* SWKBD_GetText() {
	return outputStr;
}

void SWKBD_CleanupText() {
	if(outputStr != NULL)
	{
		MEMFreeToDefaultHeap(outputStr);
		outputStr = NULL;
	}
}

void SWKBD_Shutdown() {
	SWKBD_CleanupText();
	Swkbd_Destroy();
	
	if(showed)
	{
		WHBGfxInit();
		WHBGfxShutdown();
	}
	
	if(createArg != NULL)
	{
		if(createArg->workMemory != NULL)
			MEMFreeToDefaultHeap(createArg->workMemory);
		MEMFreeToDefaultHeap(createArg);
		createArg = NULL;
	}
	
	if(appearArg != NULL)
	{
		MEMFreeToDefaultHeap(appearArg);
		appearArg = NULL;
	}
	
	if(controllerInfo != NULL)
	{
		MEMFreeToDefaultHeap(controllerInfo);
		controllerInfo = NULL;
	}
}

bool showKeyboard(char *output, KeyboardChecks check, int maxlength, bool limit) {
	WHBLogPrintf("Initialising SWKBD");
	bool kError = SWKBD_Show(maxlength, limit);
	if (!kError) {
		while(true) {
			readInput();

			startRefresh();
			colorStartRefresh(SCREEN_COLOR_RED);
			write(0, 0, "Error showing SWKBD:");
			write(0, 1, "nn::swkbd::AppearInputForm failed");
			errorScreen(2, B_RETURN);
			endRefresh();
							
			if (vpad.trigger == VPAD_BUTTON_B)
					return false;
		}
	}
	WHBLogPrintf("SWKBD initialised successfully");
	
	while (true) {
		readInput();
		SWKBD_Render(&vpad);

		if (SWKBD_IsOkButton()) {
			WHBLogPrintf("SWKBD Ok button pressed");
			SWKBD_Hide(&vpad);
			const char* kError = SWKBD_GetError(check);
			if (kError) {
				WHBLogPrintf("SWKBD Result string error: %s", kError);
				while(true) {
					readInput();

					colorStartRefresh(SCREEN_COLOR_RED);
					write(0, 0, "Error on the resulted string:");
					write(0, 1, kError);
					errorScreen(2, B_RETURN__Y_RETRY);
					endRefresh();

					if (vpad.trigger == VPAD_BUTTON_B)
						return false;
					else if (vpad.trigger == VPAD_BUTTON_Y)
						return showKeyboard(output, check, maxlength, limit);
				}
			}
			strcpy(output, SWKBD_GetText());
			return true;
		}

		if (SWKBD_IsCancelButton()) {
			WHBLogPrintf("SWKBD Cancel button pressed");
			SWKBD_Hide(&vpad);
			startRefresh();
			endRefresh();
			return false;
		}
	}
}