/***************************************************************************
 * This file is part of NUSspli.                                           *
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <crypto.h>
#include <file.h>
#include <input.h>
#include <ioQueue.h>
#include <osdefs.h>
#include <renderer.h>
#include <swkbd_wrapper.h>
#include <utils.h>
#include <menu/utils.h>

#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/time.h>

#include <jansson.h>

#define CONFIG_VERSION 1

#define LANG_JAP		"Japanese"
#define LANG_ENG		"English"
#define LANG_FRE		"French"
#define LANG_GER		"German"
#define LANG_ITA		"Italian"
#define LANG_SPA		"Spanish"
#define LANG_CHI		"Chinese"
#define LANG_KOR		"Korean"
#define LANG_DUT		"Dutch"
#define LANG_POR		"Potuguese"
#define LANG_RUS		"Russian"
#define LANG_TCH		"Traditional chinese"
#define LANG_SYS		"System settings"

#define SET_EUR			"Europe"
#define SET_USA			"USA"
#define SET_JPN			"Japan"
#define SET_ALL			"All"

#define NOTIF_RUMBLE	"Rumble"
#define NOTIF_LED		"LED"
#define NOTIF_BOTH		"Rumble + LED"
#define NOTIF_NONE		"None"

static bool changed = false;
static bool saveFailed = false;
static bool checkForUpdates = true;
static bool autoResume = true;
static Swkbd_LanguageType lang = Swkbd_LanguageType__Invalid;
static Swkbd_LanguageType sysLang;
static bool dlToUSB = true;
static MCPRegion regionSetting = MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN;
static NOTIF_METHOD notifSetting = NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED;

bool initConfig()
{
	debugPrintf("Initializing config file...");

	if(!fileExists(CONFIG_PATH))
	{
		addToScreenLog("Config file not found, using defaults!");
		changed = true; // trigger a save on app exit
		return true;
	}

	OSTime t = OSGetTime();
#ifdef NUSSPLI_DEBUG
	json_error_t jerr;
	json_t *json = json_load_file(CONFIG_PATH, 0, &jerr);
#else
	json_t *json = json_load_file(CONFIG_PATH, 0, NULL);
#endif
	if(!json)
	{
		debugPrintf("json_load_file() failed: %s!", jerr.text);
		return false;
	}

	json_t *configEntry = json_object_get(json, "Check for updates");
	if(configEntry != NULL && json_is_boolean(configEntry))
		checkForUpdates = json_is_true(configEntry);
	else
		changed = true;

	configEntry = json_object_get(json, "Auto resume failed downloads");
	if(configEntry != NULL && json_is_boolean(configEntry))
		autoResume = json_is_true(configEntry);
	else
		changed = true;

	configEntry = json_object_get(json, "Language");
	if(configEntry != NULL && json_is_string(configEntry))
	{
		if(strcmp(json_string_value(configEntry), LANG_JAP) == 0)
			lang = Swkbd_LanguageType__Japanese;
		else if(strcmp(json_string_value(configEntry), LANG_ENG) == 0)
			lang = Swkbd_LanguageType__English;
		else if(strcmp(json_string_value(configEntry), LANG_GER) == 0)
			lang = Swkbd_LanguageType__French;
		else if(strcmp(json_string_value(configEntry), LANG_ITA) == 0)
			lang = Swkbd_LanguageType__Italian;
		else if(strcmp(json_string_value(configEntry), LANG_SPA) == 0)
			lang = Swkbd_LanguageType__Spanish;
		else if(strcmp(json_string_value(configEntry), LANG_CHI) == 0)
			lang = Swkbd_LanguageType__Chinese1;
		else if(strcmp(json_string_value(configEntry), LANG_KOR) == 0)
			lang = Swkbd_LanguageType__Korean;
		else if(strcmp(json_string_value(configEntry), LANG_DUT) == 0)
			lang = Swkbd_LanguageType__Dutch;
		else if(strcmp(json_string_value(configEntry), LANG_POR) == 0)
			lang = Swkbd_LanguageType__Potuguese;
		else if(strcmp(json_string_value(configEntry), LANG_RUS) == 0)
			lang = Swkbd_LanguageType__Russian;
		else if(strcmp(json_string_value(configEntry), LANG_TCH) == 0)
			lang = Swkbd_LanguageType__Chinese2;
		else
			lang = Swkbd_LanguageType__Invalid;
	}
	else
		changed = true;

	configEntry = json_object_get(json, "Region");
	if(configEntry != NULL && json_is_string(configEntry))
	{
		if(strcmp(json_string_value(configEntry), SET_EUR) == 0)
			regionSetting = MCP_REGION_EUROPE;
		else if(strcmp(json_string_value(configEntry), SET_USA) == 0)
			regionSetting = MCP_REGION_USA;
		else if(strcmp(json_string_value(configEntry), SET_JPN) == 0)
			regionSetting = MCP_REGION_JAPAN;
		else
			regionSetting = MCP_REGION_EUROPE | MCP_REGION_USA | MCP_REGION_JAPAN;
	}
	else
		changed = true;

	configEntry = json_object_get(json, "Download to USB");
	if(configEntry != NULL && json_is_boolean(configEntry))
		dlToUSB = json_is_true(configEntry);
	else
		changed = true;

	configEntry = json_object_get(json, "Notification method");
	if(configEntry != NULL && json_is_string(configEntry))
	{
		if(strcmp(json_string_value(configEntry), NOTIF_RUMBLE) == 0)
			notifSetting = NOTIF_METHOD_RUMBLE;
		else if(strcmp(json_string_value(configEntry), NOTIF_LED) == 0)
			notifSetting = NOTIF_METHOD_LED;
		else if(strcmp(json_string_value(configEntry), NOTIF_NONE) == 0)
			notifSetting = NOTIF_METHOD_NONE;
		else
			notifSetting = NOTIF_METHOD_RUMBLE | NOTIF_METHOD_LED;
	}
	else
		changed = true;

	configEntry = json_object_get(json, "Seed");
	if(configEntry != NULL && json_is_integer(configEntry))
	{
		int ent = (int)json_integer_value(configEntry);
		addEntropy(&ent, 4);
	}

	json_decref(json);
	t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));
	
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
	return true;
}

const char *getLanguageString(Swkbd_LanguageType language)
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
		default:
			return LANG_SYS;
	}
}

const char *getNotificationString(NOTIF_METHOD method)
{
	switch((int)method)
	{
		case NOTIF_METHOD_RUMBLE:
			return NOTIF_RUMBLE;
		case NOTIF_METHOD_LED:
			return NOTIF_LED;
		case NOTIF_METHOD_NONE:
			return NOTIF_NONE;
		default:
			return NOTIF_BOTH;
	}
}

bool saveConfig(bool force)
{
	debugPrintf("saveConfig()");
	if(saveFailed)
		return false;

	if(!changed && !force)
		return true;
	
	json_t *config = json_object();
	if(config == NULL )
		return false;
	
	json_t *value = json_integer(CONFIG_VERSION);
	if(value == NULL || json_object_set(config, "File Version", value))
	{
		json_decref(config);
		return false;
	}

	value = checkForUpdates ? json_true() : json_false();
	if(value == NULL || json_object_set(config, "Check for updates", value))
	{
		json_decref(config);
		return false;
	}
	
	value = autoResume ? json_true() : json_false();
	if(value == NULL || json_object_set(config, "Auto resume failed downloads", value))
	{
		json_decref(config);
		return false;
	}
	
	value = json_string(getLanguageString(lang));
	if(value == NULL || json_object_set(config, "Language", value))
	{
		json_decref(config);
		return false;
	}

	value = json_string(getFormattedRegion(getRegion()));
	if(value == NULL || json_object_set(config, "Region", value))
	{
		json_decref(config);
		return false;
	}
	
	value = dlToUSB ? json_true() : json_false();
	if(value == NULL || json_object_set(config, "Download to USB", value))
	{
		json_decref(config);
		return false;
	}

	value = json_string(getNotificationString(getNotificationMethod()));
	if(value == NULL || json_object_set(config, "Nofit method", value))
	{
		json_decref(config);
		return false;
	}

	uint32_t entropy;
	osslBytes((unsigned char *)&entropy, 4);
	value = json_integer(entropy);
	if(value != NULL)
		json_object_set(config, "Seed", value);

	char *json = json_dumps(config, JSON_INDENT(4));
	json_decref(config);
	if(json == NULL)
		return false;

	flushIOQueue();
	NUSFILE *f = openFile(CONFIG_PATH, "w");
	if(f == NULL)
	{
		MEMFreeToDefaultHeap(json);
		debugPrintf("Can't open " CONFIG_PATH);
		saveFailed = true;

		drawErrorFrame("Couldn't save config file!\nYour SD card might be write locked.", ANY_RETURN);
		showFrame();

		while(!(vpad.trigger))
			showFrame();

		return false;
	}

	addToIOQueue(json, 1, strlen(json), f);
	MEMFreeToDefaultHeap(json);
	addToIOQueue(NULL, 0, 0, f);
	changed = false;
	return true;
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

MCPRegion getRegion()
{
	return regionSetting;
}

const char *getFormattedRegion(MCPRegion region)
{
	switch(region)
	{
		case MCP_REGION_EUROPE:
			return SET_EUR;
		case MCP_REGION_USA:
			return SET_USA;
		case MCP_REGION_JAPAN:
			return SET_JPN;
		default:
			return SET_ALL;
	}
}

void setRegion(MCPRegion region)
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

NOTIF_METHOD getNotificationMethod()
{
	return notifSetting;
}

void setNotificationMethod(NOTIF_METHOD method)
{
	if(notifSetting == method)
		return;

	notifSetting = method;
	changed = true;
}
