#include "input.h"
#include "utils.h"
#include "swkbd_wrapper.h"

//WIP. This need a better implementation

FSClient* swkbdCli;
Swkbd_CreateArg createArg;

int globalMaxlength;
bool globalLimit;

char* outputStr = NULL;

bool showed;

bool SWKBD_Init() {
	swkbdCli = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
	FSAddClient(swkbdCli, 0);
	
	WHBGfxInit();
	createArg.regionType = Swkbd_RegionType__Europe;
	createArg.workMemory = MEMAllocFromDefaultHeap(Swkbd_GetWorkMemorySize(0));
	createArg.unk_0x08 = 0;
	createArg.fsClient = swkbdCli;
	if (!Swkbd_Create(createArg)) {
		WHBLogPrintf("nn::swkbd::Create failed");
		WHBGfxShutdown();
		return false;
	}
	WHBLogPrintf("nn::swkbd::Create success");
	WHBGfxShutdown();
	return true;
}
bool SWKBD_Show(int maxlength, bool limit) {
	outputStr = NULL;
	showed = true;
	
	WHBLogPrintf("WHBGfxInit(): %d", WHBGfxInit());
	// Show the keyboard
	Swkbd_AppearArg appearArg;
	appearArg.keyboardArg.configArg = (Swkbd_ConfigArg){ Swkbd_LanguageType__English, 4, 0, 0x7FFFF, 19, -1,  1, 1 };
	appearArg.keyboardArg.receiverArg = (Swkbd_ReceiverArg){ 0, 0, 0, -1, 0, -1 };
	appearArg.inputFormArg = (Swkbd_InputFormArg){ -1, -1, 0, 0, maxlength, 0, 0, false, false, false };
	globalMaxlength = maxlength;
	if (!Swkbd_AppearInputForm(appearArg)) {
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
			free(inputFormString);
			Swkbd_SetEnableOkButton((globalLimit) ? (len == (uint32_t)globalMaxlength) : (len <= (uint32_t)globalMaxlength));
		}
	}
	
	Swkbd_ControllerInfo controllerInfo;
	controllerInfo.vpad = vpad;
	controllerInfo.kpad[0] =
		controllerInfo.kpad[1] =
		controllerInfo.kpad[2] =
		controllerInfo.kpad[3] = NULL;
	Swkbd_Calc(controllerInfo);

	if (Swkbd_IsNeedCalcSubThreadFont()) {
		Swkbd_CalcSubThreadFont();
		WHBLogPrintf("SWKBD nn::swkbd::IsNeedCalcSubThreadFont()");
	}

	if (Swkbd_IsNeedCalcSubThreadPredict()) {
		Swkbd_CalcSubThreadPredict();
		WHBLogPrintf("SWKBD nn::swkbd::CalcSubThreadPredict()");
	}

	WHBGfxBeginRender();

	WHBGfxBeginRenderTV();
	WHBGfxClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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

	if (globalMaxlength != -1 && globalLimit && strlen(output) != (uint32_t)globalMaxlength)
	{
		char ret[1024];
		sprintf(ret, "Invalid input size (%d/%d)", strlen(output), globalMaxlength);
		free(output);
		return ret;
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
		free(outputStr);
		outputStr = NULL;
	}
}

void SWKBD_Shutdown() {
	SWKBD_CleanupText();
	Swkbd_Destroy();
	MEMFreeToDefaultHeap(createArg.workMemory);
	
	if (showed) { //Shutdown libraries properly
		WHBGfxInit();
		WHBGfxShutdown();
	}
}

bool showKeyboard(char** output, KeyboardChecks check, int maxlength, bool limit) {
	WHBLogPrintf("Initialising SWKBD");
	bool kError = SWKBD_Show(maxlength, limit);
	if (!kError) {
		while(true) {
			readInput();

			startRefresh();
			colorStartRefresh(0xFF800000);
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

					colorStartRefresh(0xFF800000);
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
			*output = SWKBD_GetText();
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