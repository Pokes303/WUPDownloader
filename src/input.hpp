#include "main.hpp"

enum KeyboardChecks {
	CHECK_NONE = 0,			//No check
	CHECK_NUMERICAL = 1,	//Only numbers
	CHECK_HEXADECIMAL = 2,	//Only hex
	CHECK_NOSPECIAL = 3		//Only letters or numbers
};

bool SWKBD_Init();
bool SWKBD_Show(int maxlength, bool limit);
void SWKBD_Render(VPADStatus* vpad);
void SWKBD_Hide(VPADStatus* vpad);
bool SWKBD_IsOkButton();
bool SWKBD_IsCancelButton();
const char* SWKBD_GetError(KeyboardChecks check);
std::string SWKBD_GetText();
void SWKBD_Shutdown();
bool SWKBD_IsShowing();

bool showKeyboard(std::string* output, KeyboardChecks check, int maxlength, bool limit);