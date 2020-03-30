#ifndef SWKBD_MAPPER_H
#define SWKBD_MAPPER_H

#include <stdbool.h>

#include <nn/swkbd.h>

#ifdef __cplusplus
	extern "C" {
#endif

// Enums copied from https://github.com/devkitPro/wut/blob/master/include/nn/swkbd/swkbd_cpp.h
typedef enum
{
   Swkbd_LanguageType__Japanese = 0,
   Swkbd_LanguageType__English  = 1,
} Swkbd_LanguageType;

typedef enum
{
	Swkbd_RegionType__Japan    = 0,
	Swkbd_RegionType__USA      = 1,
	Swkbd_RegionType__Europe   = 2,
} Swkbd_RegionType;

//Structs also from the link above
typedef struct
{
   Swkbd_LanguageType languageType;
   uint32_t unk_0x04;
   uint32_t unk_0x08;
   uint32_t unk_0x0C;
   uint32_t unk_0x10;
   int32_t unk_0x14;
   WUT_UNKNOWN_BYTES(0x9C - 0x18);
   uint32_t unk_0x9C;
   WUT_UNKNOWN_BYTES(4);
   int32_t unk_0xA4;
} Swkbd_ConfigArg;

typedef struct
{
	uint32_t unk_0x00;
	uint32_t unk_0x04;
	uint32_t unk_0x08;
	int32_t unk_0x0C;
	uint32_t unk_0x10;
	int32_t unk_0x14;
} Swkbd_ReceiverArg;

typedef struct
{
   Swkbd_ConfigArg configArg;
   Swkbd_ReceiverArg receiverArg;
} Swkbd_KeyboardArg;

typedef struct
{
   uint32_t unk_0x00;
   int32_t unk_0x04;
   uint32_t unk_0x08;
   uint32_t unk_0x0C;
   int32_t maxTextLength;
   uint32_t unk_0x14;
   uint32_t unk_0x18;
   bool unk_0x1C;
   bool unk_0x1D;
   bool unk_0x1E;
   WUT_PADDING_BYTES(1);
} Swkbd_InputFormArg;

typedef struct
{
   Swkbd_KeyboardArg keyboardArg;
   Swkbd_InputFormArg inputFormArg;
} Swkbd_AppearArg;

typedef struct
{
   void *workMemory;
   Swkbd_RegionType regionType;
   uint32_t unk_0x08;
   FSClient *fsClient;
} Swkbd_CreateArg;

typedef struct
{
   VPADStatus *vpad;
   KPADStatus *kpad[4];
} Swkbd_ControllerInfo;

//Wrapper functions
uint32_t Swkbd_GetWorkMemorySize(uint32_t unk);
bool Swkbd_AppearInputForm(const Swkbd_AppearArg args);
bool Swkbd_Create(const Swkbd_CreateArg args);
void Swkbd_SetEnableOkButton(bool enable);
char *Swkbd_GetInputFormString();
void Swkbd_Calc(const Swkbd_ControllerInfo controllerInfo);
bool Swkbd_IsNeedCalcSubThreadFont();
void Swkbd_CalcSubThreadFont();
bool Swkbd_IsNeedCalcSubThreadPredict();
void Swkbd_CalcSubThreadPredict();
void Swkbd_DrawTV();
void Swkbd_DrawDRC();
bool Swkbd_IsDecideOkButton(bool *outIsSelected);
bool Swkbd_IsDecideCancelButton(bool *outIsSelected);
bool Swkbd_DisappearInputForm();
void Swkbd_Destroy();

#ifdef __cplusplus
	}
#endif

#endif //ifndef HEADER_FILE