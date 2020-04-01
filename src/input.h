#ifndef WUPD_INPUT_H
#define WUPD_INPUT_H

#include "main.h"
#include <stdbool.h>

#include <vpad/input.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum {
	CHECK_NONE = 0,			//No check
	CHECK_NUMERICAL = 1,	//Only numbers
	CHECK_HEXADECIMAL = 2,	//Only hex
	CHECK_NOSPECIAL = 3		//Only letters or numbers
} KeyboardChecks;

bool SWKBD_Init();
bool SWKBD_Show(int maxlength, bool limit);
void SWKBD_Render(VPADStatus* vpad);
void SWKBD_Hide(VPADStatus* vpad);
bool SWKBD_IsOkButton();
bool SWKBD_IsCancelButton();
const char* SWKBD_GetError(KeyboardChecks check);
char* SWKBD_GetText();
void SWKBD_CleanupText();
void SWKBD_Shutdown();

bool showKeyboard(char *output, KeyboardChecks check, int maxlength, bool limit);

#ifdef __cplusplus
	}
#endif

#endif //ifndef WUPD_INPUT_H