#include "input.hpp"
#include "utils.hpp"

#include <nn/swkbd.h>

//WIP. This need a better implementation

FSClient* swkbdCli;
nn::swkbd::CreateArg createArg;

int globalMaxlength;
bool globalLimit;

std::string outputStr;

bool showed;

bool SWKBD_Init() {
	swkbdCli = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
	FSAddClient(swkbdCli, 0);
	
	WHBGfxInit();
	//createArg = nn::swkbd::CreateArg();
	createArg.regionType = nn::swkbd::RegionType::Europe;
	createArg.workMemory = MEMAllocFromDefaultHeap(nn::swkbd::GetWorkMemorySize(0));
	createArg.fsClient = swkbdCli;
	if (!nn::swkbd::Create(createArg)) {
		WHBLogPrintf("nn::swkbd::Create failed");
		WHBGfxShutdown();
		return false;
	}
	WHBLogPrintf("nn::swkbd::Create success");
	WHBGfxShutdown();
	return true;
}
bool SWKBD_Show(int maxlength, bool limit) {
	outputStr = "none";
	showed = true;
	
	WHBLogPrintf("WHBGfxInit(): %d", WHBGfxInit());
	// Show the keyboard
	nn::swkbd::AppearArg appearArg;
	appearArg.keyboardArg.configArg.languageType = nn::swkbd::LanguageType::English;
	appearArg.inputFormArg.maxTextLength = maxlength; //-1 For unlimited size
	globalMaxlength = maxlength;
	if (!nn::swkbd::AppearInputForm(appearArg)) {
		WHBLogPrintf("nn::swkbd::AppearInputForm failed");
		WHBGfxShutdown();
		return false;
	}
	globalLimit = limit;
	if (!limit && maxlength != -1)
		nn::swkbd::SetEnableOkButton(false);
	WHBLogPrintf("nn::swkbd::AppearInputForm success");
	return true;
}
void SWKBD_Render(VPADStatus* vpad) {
	if (globalMaxlength != -1) {
		uint32_t len = std::u16string(nn::swkbd::GetInputFormString()).size();
		nn::swkbd::SetEnableOkButton((globalLimit) ? (len == (uint32_t)globalMaxlength) : (len <= (uint32_t)globalMaxlength));
	}
	
	nn::swkbd::ControllerInfo controllerInfo;
	controllerInfo.vpad = vpad;
	controllerInfo.kpad[0] = nullptr;
	controllerInfo.kpad[1] = nullptr;
	controllerInfo.kpad[2] = nullptr;
	controllerInfo.kpad[3] = nullptr;
	nn::swkbd::Calc(controllerInfo);

	if (nn::swkbd::IsNeedCalcSubThreadFont()) {
		nn::swkbd::CalcSubThreadFont();
		WHBLogPrintf("SWKBD nn::swkbd::IsNeedCalcSubThreadFont()");
	}

	if (nn::swkbd::IsNeedCalcSubThreadPredict()) {
		nn::swkbd::CalcSubThreadPredict();
		WHBLogPrintf("SWKBD nn::swkbd::CalcSubThreadPredict()");
	}

	WHBGfxBeginRender();

	WHBGfxBeginRenderTV();
	WHBGfxClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	nn::swkbd::DrawTV();
	WHBGfxFinishRenderTV();
	
	WHBGfxBeginRenderDRC();
	WHBGfxClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	nn::swkbd::DrawDRC();
	WHBGfxFinishRenderDRC();
	
	WHBGfxFinishRender();
}
void SWKBD_Hide(VPADStatus* vpad) {
	for (uint8_t i = 0; i < 14; i++) {
		SWKBD_Render(vpad);
	}
	WHBGfxShutdown();
}
bool SWKBD_IsOkButton() {
	if (nn::swkbd::IsDecideOkButton(nullptr)) {
		nn::swkbd::DisappearInputForm();
		return true;
	}
	return false;
}
bool SWKBD_IsCancelButton() {
	if (nn::swkbd::IsDecideCancelButton(nullptr)) {
		nn::swkbd::DisappearInputForm();
		return true;
	}
	return false;
}
const char* SWKBD_GetError(KeyboardChecks check) {
	if (!nn::swkbd::GetInputFormString())
		return "nn::swkbd::GetInputFormString returned NULL";
	
	std::u16string output(nn::swkbd::GetInputFormString());

	if (globalMaxlength != -1 && globalLimit && output.size() != (uint32_t)globalMaxlength)
		return (std::string("Invalid input size (") + std::to_string(output.size()) + std::string("/") + std::to_string(globalMaxlength) + std::string(")")).c_str();

	// Quick hack to get from a char16_t str to char for our log function
	outputStr = "";

	for (uint32_t i = 0; i < output.size(); ++i) {
		if (output[i] > 0x7F)
			outputStr += '?';
		else
			outputStr += output[i];
		
		switch (check) {
			case CHECK_NUMERICAL:
				if (!(output[i] >= '0' && output[i] <= '9'))
					return "The wrote string must be only numerical [0->9]";
				break;
			case CHECK_HEXADECIMAL:
				if (!((output[i] >= '0' && output[i] <= '9') || (output[i] >= 'A' && output[i] <= 'F') || (output[i] >= 'a' && output[i] <= 'f')))
					return "The wrote string must be only hexadecimal [0->F]";
				break;
			case CHECK_NOSPECIAL:
				if (!((output[i] >= '0' && output[i] <= '9') || (output[i] >= 'A' && output[i] <= 'Z') || (output[i] >= 'a' && output[i] <= 'z') || output[i] != ' '))
					return "The wrote string must not have special characters [A->Z->9->Space]";
				break;
		}
	}
	WHBLogPrintf("Input string: %s", outputStr.c_str());
	return NULL;
}
std::string SWKBD_GetText() {
	return outputStr;
}
void SWKBD_Shutdown() {
	nn::swkbd::Destroy();
	MEMFreeToDefaultHeap(createArg.workMemory);
	
	if (showed) { //Shutdown libraries properly
		WHBGfxInit();
		WHBGfxShutdown();
	}
}

bool showKeyboard(std::string* output, KeyboardChecks check, int maxlength, bool limit) {
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