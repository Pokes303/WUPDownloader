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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <cJSON.h>
#include <config.h>
#include <crypto.h>
#include <file.h>
#include <input.h>
#include <osdefs.h>
#include <renderer.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/time.h>

#define CONFIG_VERSION 1

#define LANG_JAP	"Japanese"
#define LANG_ENG	"English"
#define LANG_FRE	"French"
#define LANG_GER	"German"
#define LANG_ITA	"Italian"
#define LANG_SPA	"Spanish"
#define LANG_CHI	"Chinese"
#define LANG_KOR	"Korean"
#define LANG_DUT	"Dutch"
#define LANG_POR	"Potuguese"
#define LANG_RUS	"Russian"
#define LANG_TCH	"Traditional chinese"
#define LANG_SYS	"System settings"

#define SET_EUR		"Europe"
#define SET_USA		"USA"
#define SET_JPN		"Japan"
#define SET_ALL		"All"

bool changed = false;
bool useTitleDB = true;
bool checkForUpdates = true;
bool autoResume = true;
Swkbd_LanguageType lang = Swkbd_LanguageType__Invalid;
Swkbd_LanguageType sysLang;
int configInitTries = 0;
bool dlToUSB = true;
reg regionSetting = regAll;

bool initConfig()
{
	if(++configInitTries == 10)
	{
		debugPrintf("Giving up!");
		return false;
	}
	
	debugPrintf("Initializing config file...");
	
	if(!fileExists(CONFIG_PATH))
	{
		debugPrintf("\tFile not found!");
		if(!saveConfig(true))
			return false;
	}
	
	OSTime t = OSGetTime();
	FILE *fp = fopen(CONFIG_PATH, "rb");
	if(fp == NULL)
		return false;
	
	long fileSize = getFilesize(fp);
	bool ret = fileSize > 0;
	if(!ret)
		fileSize = 1;
	char fileContent[fileSize];
	ret = ret && fread(fileContent, fileSize, 1, fp) == 1;
	fclose(fp);
	t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));
	if(!ret)
		return false;
	
	cJSON *json = cJSON_ParseWithLength(fileContent, fileSize);
	if(json == NULL)
		return false;
	
	cJSON *configEntry = cJSON_GetObjectItemCaseSensitive(json, "Use online title DB");
	if(configEntry != NULL && cJSON_IsBool(configEntry))
		useTitleDB = cJSON_IsTrue(configEntry);
	else
		changed = true;
	
	configEntry = cJSON_GetObjectItemCaseSensitive(json, "Check for updates");
	if(configEntry != NULL && cJSON_IsBool(configEntry))
		checkForUpdates = cJSON_IsTrue(configEntry);
	else
		changed = true;
	
	configEntry = cJSON_GetObjectItemCaseSensitive(json, "Auto resume failed downloads");
	if(configEntry != NULL && cJSON_IsBool(configEntry))
		autoResume = cJSON_IsTrue(configEntry);
	else
		changed = true;
	
	configEntry = cJSON_GetObjectItemCaseSensitive(json, "Language");
	if(configEntry != NULL && cJSON_IsString(configEntry))
	{
		if(strcmp(configEntry->valuestring, LANG_JAP) == 0)
			lang = Swkbd_LanguageType__Japanese;
		else if(strcmp(configEntry->valuestring, LANG_ENG) == 0)
			lang = Swkbd_LanguageType__English;
		else if(strcmp(configEntry->valuestring, LANG_GER) == 0)
			lang = Swkbd_LanguageType__French;
		else if(strcmp(configEntry->valuestring, LANG_ITA) == 0)
			lang = Swkbd_LanguageType__Italian;
		else if(strcmp(configEntry->valuestring, LANG_SPA) == 0)
			lang = Swkbd_LanguageType__Spanish;
		else if(strcmp(configEntry->valuestring, LANG_CHI) == 0)
			lang = Swkbd_LanguageType__Chinese1;
		else if(strcmp(configEntry->valuestring, LANG_KOR) == 0)
			lang = Swkbd_LanguageType__Korean;
		else if(strcmp(configEntry->valuestring, LANG_DUT) == 0)
			lang = Swkbd_LanguageType__Dutch;
		else if(strcmp(configEntry->valuestring, LANG_POR) == 0)
			lang = Swkbd_LanguageType__Potuguese;
		else if(strcmp(configEntry->valuestring, LANG_RUS) == 0)
			lang = Swkbd_LanguageType__Russian;
		else if(strcmp(configEntry->valuestring, LANG_TCH) == 0)
			lang = Swkbd_LanguageType__Chinese2;
		else
			lang = Swkbd_LanguageType__Invalid;
	}
	else
		changed = true;

	configEntry = cJSON_GetObjectItemCaseSensitive(json, "Region");
	if(configEntry != NULL && cJSON_IsString(configEntry))
	{
		if(strcmp(configEntry->valuestring, SET_ALL) == 0)
			regionSetting = regAll;
		else if(strcmp(configEntry->valuestring, SET_EUR) == 0)
			regionSetting = regEUR;
		else if(strcmp(configEntry->valuestring, SET_USA) == 0)
			regionSetting = regUSA;
		else if(strcmp(configEntry->valuestring, SET_JPN) == 0)
			regionSetting = regJPN;
		else
			regionSetting = regAll;
	}
	else
		changed = true;
	
	configEntry = cJSON_GetObjectItemCaseSensitive(json, "Download to USB");
	if(configEntry != NULL && cJSON_IsBool(configEntry))
		dlToUSB = cJSON_IsTrue(configEntry);
	else
		changed = true;

	configEntry = cJSON_GetObjectItemCaseSensitive(json, "Seed");
	if(configEntry != NULL && cJSON_IsNumber(configEntry))
		addEntropy(&(configEntry->valueint), 4);
	
	cJSON_Delete(json);
	
	if(changed)
		saveConfig(false);
	
	IOSHandle handle = UCOpen();
	if(handle < 1)
	{
		debugPrintf("Error opening UC: %d", handle);
		sysLang = Swkbd_LanguageType__English;
		return true;
	}
	
	UCSysConfig *settings = MEMAllocFromDefaultHeapEx(sizeof(UCSysConfig), 0x40);
	if(settings == NULL)
	{
		UCClose(handle);
		debugPrintf("OUT OF MEMORY!");
		sysLang = Swkbd_LanguageType__English;
		return true;
	}
	
	strcpy(settings->name, "cafe.language");
	settings->access = 0;
	settings->dataType = UCDataType_UnsignedInt;
	settings->error = IOS_ERROR_OK;
	settings->dataSize = sizeof(Swkbd_LanguageType);
	settings->data = &sysLang;
	
	IOSError err = UCReadSysConfig(handle, 1, settings);
	if(err != IOS_ERROR_OK)
	{
		UCClose(handle);
		MEMFreeToDefaultHeap(settings);
		debugPrintf("Error reading UC: %d!", err);
		sysLang = Swkbd_LanguageType__English;
		return true;
	}
	
	UCClose(handle);
	MEMFreeToDefaultHeap(settings);
	debugPrintf("System language found: %s", getLanguageString(sysLang));
	
	addToScreenLog("Config file loaded!");
	startNewFrame();
	textToFrame(0, 0, "Loading...");
	writeScreenLog();
	drawFrame();
	showFrame();
	return true;
}

char *getLanguageString(Swkbd_LanguageType language)
{
	switch(language)
	{
		case Swkbd_LanguageType__Japanese:
			return LANG_JAP;
		case Swkbd_LanguageType__English:
			return LANG_ENG;
		case Swkbd_LanguageType__French:
			return LANG_FRE;
		case Swkbd_LanguageType__German:
			return LANG_GER;
		case Swkbd_LanguageType__Italian:
			return LANG_ITA;
		case Swkbd_LanguageType__Spanish:
			return LANG_SPA;
		case Swkbd_LanguageType__Chinese1:
			return LANG_CHI;
		case Swkbd_LanguageType__Korean:
			return LANG_KOR;
		case Swkbd_LanguageType__Dutch:
			return LANG_DUT;
		case Swkbd_LanguageType__Potuguese:
			return LANG_POR;
		case Swkbd_LanguageType__Russian:
			return LANG_RUS;
		case Swkbd_LanguageType__Chinese2:
			return LANG_TCH;
			break;
		default:
			return LANG_SYS;
	}
}

bool saveConfig(bool force)
{
	debugPrintf("saveConfig()");
	if(!changed && !force)
		return true;
	
	cJSON *config = cJSON_CreateObject();
	if(config == NULL )
		return false;
	
	cJSON *entry = cJSON_CreateNumber(CONFIG_VERSION);
	if(entry == NULL)
	{
		cJSON_Delete(config);
		return false;;
	}
	cJSON_AddItemToObject(config, "File Version", entry);
	
	entry = cJSON_CreateBool(useTitleDB);
	if(entry == NULL)
	{
		cJSON_Delete(config);
		return false;
	}
	cJSON_AddItemToObject(config, "Use online title DB", entry);
	
	entry = cJSON_CreateBool(checkForUpdates);
	if(entry == NULL)
	{
		cJSON_Delete(config);
		return false;
	}
	cJSON_AddItemToObject(config, "Check for updates", entry);
	
	entry = cJSON_CreateBool(autoResume);
	if(entry == NULL)
	{
		cJSON_Delete(config);
		return false;
	}
	cJSON_AddItemToObject(config, "Auto resume failed downloads", entry);
	
	entry = cJSON_CreateString(getLanguageString(lang));
	if(entry == NULL)
	{
		cJSON_Delete(config);
		return false;
	}
	cJSON_AddItemToObject(config, "Language", entry);

	entry = cJSON_CreateString(getFormattedRegion(getRegion()));
	if(entry == NULL)
	{
		cJSON_Delete(config);
		return false;
	}
	cJSON_AddItemToObject(config, "Region", entry);
	
	entry = cJSON_CreateBool(dlToUSB);
	if(entry == NULL)
	{
		cJSON_Delete(config);
		return false;
	}
	cJSON_AddItemToObject(config, "Download to USB", entry);

	uint32_t entropy;
	osslBytes((unsigned char *)&entropy, 4);
	entry = cJSON_CreateNumber(entropy);
		cJSON_AddItemToObject(config, "Seed", entry);
	
	char *configString = cJSON_Print(config);
	cJSON_Delete(config);
	if(configString == NULL)
		return false;
	
	OSTime t = OSGetTime();
	FILE *fp = fopen(CONFIG_PATH, "w");
	if(fp == NULL)
		return false;
	
	fwrite(configString, strlen(configString), 1, fp);
	debugPrintf("Config written!");
	fclose(fp);
	t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));
	
	changed = false;
	return true;
}

bool useOnlineTitleDB()
{
	return useTitleDB;
}

void setUseOnlineTitleDB(bool use)
{
	if(useTitleDB == use)
		return;
	
	useTitleDB = use;
	changed = true;
}

bool updateCheckEnabled()
{
	return checkForUpdates;
}

void setUpdateCheck(bool enabled)
{
	if(checkForUpdates == enabled)
		return;
	
	checkForUpdates = enabled;
	changed = true;
}

bool autoResumeEnabled()
{
	return autoResume;
}

void setAutoResume(bool enabled)
{
	if(autoResume == enabled)
		return;
	
	autoResume = enabled;
	changed = true;
}

bool dlToUSBenabled()
{
	return dlToUSB;
}

void setDlToUSB(bool toUSB)
{
	if(dlToUSB == toUSB)
		return;
	
	dlToUSB = toUSB;
	changed = true;
}

Swkbd_LanguageType getKeyboardLanguage()
{
	return lang == Swkbd_LanguageType__Invalid ? sysLang : lang;
}

Swkbd_LanguageType getUnfilteredLanguage()
{
	return lang;
}

reg getRegion()
{
	return regionSetting;
}

char *getFormattedRegion(reg region)
{
	switch(region)
	{
		case regEUR:
			return SET_EUR;
		case regUSA:
			return SET_USA;
		case regJPN:
			return SET_JPN;
		default:
			return SET_ALL;
	}
}

void setRegion(reg region)
{
	if(region == regionSetting)
		return;

	regionSetting = region;
	changed = true;
}

void setKeyboardLanguage(Swkbd_LanguageType language)
{
	if(lang == language)
		return;
	
	lang = language;
	changed = true;
	
	SWKBD_Shutdown();
	debugPrintf("CA");
	pauseRenderer();
	debugPrintf("CB");
	resumeRenderer();
	debugPrintf("CC");
//	SWKBD_Init();
	debugPrintf("CD");
}
