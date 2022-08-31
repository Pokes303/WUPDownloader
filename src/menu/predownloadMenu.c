/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
 * Copyright (c) 2022 Xpl0itU <DaThinkingChair@protonmail.com>             *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include <wut-fixups.h>

#include <config.h>
#include <deinstaller.h>
#include <downloader.h>
#include <filesystem.h>
#include <input.h>
#include <menu/predownload.h>
#include <menu/utils.h>
#include <queue.h>
#include <renderer.h>
#include <state.h>
#include <titles.h>
#include <tmd.h>
#include <utils.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <coreinit/filesystem.h>
#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#define PD_MENU_ENTRIES 5

typedef enum
{
    OPERATION_DOWNLOAD,
    OPERATION_INSTALL,
} OPERATION;

static int cursorPos = 17;

static inline bool isInstalled(const TitleEntry *entry, MCPTitleListType *out)
{
    if(out == NULL)
    {
        MCPTitleListType titleList;
        return MCP_GetTitleInfo(mcpHandle, entry->tid, &titleList) == 0;
    }
    return MCP_GetTitleInfo(mcpHandle, entry->tid, out) == 0;
}

static void drawPDMenuFrame(const TitleEntry *entry, const char *titleVer, uint64_t size, bool installed, const char *folderName, bool usbMounted, NUSDEV dlDev, bool keepFiles, OPERATION operation, NUSDEV instDev)
{
    startNewFrame();

    textToFrame(0, 0, gettext("Name:"));

    char *toFrame = getToFrameBuffer();
    strcpy(toFrame, entry->name);
    char tid[17];
    hex(entry->tid, 16, tid);
    strcat(toFrame, " [");
    strcat(toFrame, tid);
    strcat(toFrame, "]");
    int line = textToFrameMultiline(0, ALIGNED_CENTER, toFrame, MAX_CHARS - 33); // TODO

    char *bs;
    float fsize;
    if(size > 1024 * 1024 * 1024)
    {
        bs = "GB";
        fsize = ((float)(size / 1024 / 1024)) / 1024.0F;
    }
    else if(size > 1024 * 1924)
    {
        bs = "MB";
        fsize = ((float)(size / 1024)) / 1024.0F;
    }
    else if(size > 1024)
    {
        bs = "KB";
        fsize = ((float)size) / 1024.0F;
    }
    else
    {
        bs = "B";
        fsize = (float)size;
    }

    sprintf(toFrame, "%.02f %s", fsize, bs);
    textToFrame(++line, 0, gettext("Size:"));
    textToFrame(++line, 3, toFrame);

    strcpy(toFrame, gettext("Provided title version"));
    strcat(toFrame, " [");
    strcat(toFrame, gettext("Only numbers"));
    strcat(toFrame, "]:");
    textToFrame(++line, 0, toFrame);

    if(titleVer[0] == '\0')
    {
        toFrame[0] = '<';
        strcpy(toFrame + 1, gettext("LATEST"));
        strcat(toFrame, ">");
        textToFrame(++line, 3, toFrame);
    }
    else
        textToFrame(++line, 3, titleVer);

    strcpy(toFrame, gettext("Custom folder name"));
    strcat(toFrame, " [");
    strcat(toFrame, gettext("ASCII only"));
    strcat(toFrame, "]:");
    textToFrame(++line, 0, toFrame);
    textToFrame(++line, 3, folderName);

    line = MAX_LINES;

    arrowToFrame(cursorPos, 0);

    textToFrame(--line, 0, gettext("Press " BUTTON_B " to return || " BUTTON_PLUS " to start || " BUTTON_MINUS " to add to the queue"));
    lineToFrame(--line, SCREEN_COLOR_WHITE);

    textToFrame(--line, 5, gettext("Set custom name to the download folder"));
    textToFrame(--line, 5, gettext("Set title version"));

    strcpy(toFrame, gettext("Download to:"));
    strcat(toFrame, " ");
    switch((int)dlDev)
    {
        case NUSDEV_USB01:
        case NUSDEV_USB02:
            strcat(toFrame, " SD");
            break;
        case NUSDEV_SD:
            strcat(toFrame, " NAND");
            break;
        case NUSDEV_MLC:
            strcat(toFrame, usbMounted ? " USB" : " SD");
    }
    textToFrame(--line, 5, gettext(toFrame));

    strcpy(toFrame, gettext("Operation:"));
    strcat(toFrame, " ");
    switch((int)operation)
    {
        case OPERATION_DOWNLOAD:
            strcat(toFrame, " Download only");
            break;
        case OPERATION_INSTALL:
            strcat(toFrame, " Install");
            break;
    }
    textToFrame(--line, 5, gettext(toFrame));

    strcpy(toFrame, gettext("Install to:"));
    strcat(toFrame, " ");
    switch((int)instDev)
    {
        case NUSDEV_USB01:
        case NUSDEV_USB02:
            strcat(toFrame, " USB");
            break;
        case NUSDEV_MLC:
            strcat(toFrame, " NAND");
            break;
    }
    textToFrame(--line, 5, gettext(toFrame));

    lineToFrame(--line, SCREEN_COLOR_WHITE);

    drawFrame();
}

static int drawPDDemoFrame(const TitleEntry *entry, bool inst)
{
    char *toFrame = getToFrameBuffer();
    strcpy(toFrame, entry->name);
    strcat(toFrame, " ");
    strcat(toFrame, gettext("is a demo."));
    strcat(toFrame, "\n\n" BUTTON_A " ");

    strcat(toFrame, gettext(inst ? "Install main game" : "Download main game"));
    strcat(toFrame, " || " BUTTON_B " ");
    strcat(toFrame, gettext("Continue"));

    return addErrorOverlay(toFrame);
}

static inline void changeTitleVersion(char *buf)
{
    if(!showKeyboard(KEYBOARD_LAYOUT_TID, KEYBOARD_TYPE_RESTRICTED, buf, CHECK_NUMERICAL, 5, false, buf, NULL))
        buf[0] = '\0';
}

static inline void changeFolderName(char *buf)
{
    if(!showKeyboard(KEYBOARD_LAYOUT_TID, KEYBOARD_TYPE_NORMAL, buf, CHECK_ALPHANUMERICAL, FS_MAX_PATH - 11, false, buf, NULL))
        buf[0] = '\0';
}

static inline void switchInstallDevice(NUSDEV *dev)
{
    switch((int)*dev)
    {
        case NUSDEV_USB01:
        case NUSDEV_USB02:
            *dev = NUSDEV_MLC;
            break;
        case NUSDEV_MLC:
            *dev = getUSB();
            if(!*dev)
                *dev = NUSDEV_MLC;
            break;
    }
}

static inline void switchOperation(OPERATION *op)
{
    switch(*op)
    {
        case OPERATION_INSTALL:
            *op = OPERATION_DOWNLOAD;
            break;
        case OPERATION_DOWNLOAD:
            *op = OPERATION_INSTALL;
            break;
    }
}

static inline void switchDownloadDevice(NUSDEV *dev)
{
    if(vpad.trigger & VPAD_BUTTON_LEFT)
    {
        switch((int)*dev)
        {
            case NUSDEV_USB01:
            case NUSDEV_USB02:
                *dev = NUSDEV_MLC;
                break;
            case NUSDEV_MLC:
                *dev = NUSDEV_SD;
                break;
            case NUSDEV_SD:
                *dev = getUSB();
                if(!*dev)
                    *dev = NUSDEV_MLC;
                break;
        }
    }
    else
    {
        switch((int)*dev)
        {
            case NUSDEV_USB01:
            case NUSDEV_USB02:
                *dev = NUSDEV_SD;
                break;
            case NUSDEV_SD:
                *dev = NUSDEV_MLC;
                break;
            case NUSDEV_MLC:
                *dev = getUSB();
                if(!*dev)
                    *dev = NUSDEV_SD;
                break;
        }
    }
}

void predownloadMenu(const TitleEntry *entry)
{
    MCPTitleListType titleList;
    bool installed = isInstalled(entry, &titleList);
    NUSDEV usbMounted = getUSB();
    NUSDEV dlDev = usbMounted && dlToUSBenabled() ? usbMounted : NUSDEV_SD;
    NUSDEV instDev = NUSDEV_MLC;
    OPERATION operation = OPERATION_INSTALL;
    bool keepFiles = true;
    char folderName[FS_MAX_PATH - 11];
    char titleVer[33];
    folderName[0] = titleVer[0] = '\0';
    TMD *tmd;
    uint64_t dls;

    bool loop = true;
    bool inst, toUSB;
    bool redraw = false;

    bool toQueue = false;

    char tid[17];
    char downloadUrl[256];
downloadTMD:
    clearRamBuf();
    hex(entry->tid, 16, tid);

    debugPrintf("Downloading TMD...");
    strcpy(downloadUrl, DOWNLOAD_URL);
    strcat(downloadUrl, tid);
    strcat(downloadUrl, "/tmd");

    if(strlen(titleVer) > 0)
    {
        strcat(downloadUrl, ".");
        strcat(downloadUrl, titleVer);
    }

    if(downloadFile(downloadUrl, "title.tmd", NULL, (FileType)(FILE_TYPE_TMD | FILE_TYPE_TORAM), false))
    {
        clearRamBuf();
        debugPrintf("Error downloading TMD");
        saveConfig(false);
        return;
    }

    tmd = (TMD *)getRamBuf();
    if(!verifyTmd(tmd, getRamBufSize()))
    {
        clearRamBuf();
        saveConfig(false);

        drawErrorFrame(gettext("Invalid title.tmd file!"), ANY_RETURN);

        while(AppRunning())
        {
            if(app == APP_STATE_BACKGROUND)
                continue;
            if(app == APP_STATE_RETURNING)
                drawErrorFrame(gettext("Invalid title.tmd file!"), ANY_RETURN);

            showFrame();
            if(vpad.trigger)
                break;
        }

        return;
    }

    dls = 0;
    for(uint16_t i = 0; i < tmd->num_contents; ++i)
    {
        if((tmd->contents[i].type & 0x0003) == 0x0003)
            dls += 20;

        dls += tmd->contents[i].size;
    }

naNedNa:
    drawPDMenuFrame(entry, titleVer, dls, installed, folderName, usbMounted, dlDev, keepFiles, operation, instDev);

    while(loop && AppRunning())
    {
        if(app == APP_STATE_BACKGROUND)
            continue;
        if(app == APP_STATE_RETURNING)
            drawPDMenuFrame(entry, titleVer, dls, installed, folderName, usbMounted, dlDev, keepFiles, operation, instDev);

        showFrame();

        if(vpad.trigger & VPAD_BUTTON_B)
        {
            clearRamBuf();
            saveConfig(false);
            return;
        }

        if(vpad.trigger & (VPAD_BUTTON_RIGHT | VPAD_BUTTON_LEFT | VPAD_BUTTON_A))
        {
            switch(cursorPos)
            {
                case 17: // TODO: Change hardcoded numbers to something prettier
                    switchInstallDevice(&instDev);
                    break;
                case 18:
                    switchOperation(&operation);
                    break;
                case 19:
                    switchDownloadDevice(&dlDev);
                    break;
                case 20:
                    changeTitleVersion(titleVer);
                    goto downloadTMD;
                case 21:
                    changeFolderName(folderName);
                    break;
            }

            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_DOWN)
        {
            if(++cursorPos == 22) // TODO: Change hardcoded numbers to something prettier
                cursorPos = 17;

            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_UP)
        {
            if(--cursorPos == 16) // TODO: Change hardcoded numbers to something prettier
                cursorPos = 21;

            redraw = true;
        }

        if(vpad.trigger & VPAD_BUTTON_PLUS)
        {
            inst = operation == OPERATION_INSTALL;
            toUSB = instDev == NUSDEV_USB01 || instDev == NUSDEV_USB02;
            loop = false;
        }

        if(vpad.trigger & VPAD_BUTTON_MINUS)
        {
            inst = operation == OPERATION_INSTALL;
            toUSB = instDev == NUSDEV_USB01 || instDev == NUSDEV_USB02;
            toQueue = true;
            loop = false;
        }

        if(redraw)
        {
            drawPDMenuFrame(entry, titleVer, dls, installed, folderName, usbMounted, dlDev, keepFiles, operation, instDev);
            redraw = false;
        }
    }

    if(!AppRunning())
    {
        clearRamBuf();
        return;
    }

    startNewFrame();
    textToFrame(0, 0, gettext("Preparing the download of"));
    textToFrame(1, 3, entry->name == NULL ? "NULL" : entry->name);
    writeScreenLog(2);
    drawFrame();
    showFrame();

    saveConfig(false);

    if(isDemo(entry))
    {
        uint64_t t = entry->tid;
        t &= 0xFFFFFFF0FFFFFFF0; // TODO
        const TitleEntry *te = getTitleEntryByTid(t);
        if(te != NULL)
        {
            int ovl = drawPDDemoFrame(entry, inst);

            while(AppRunning())
            {
                if(app == APP_STATE_BACKGROUND)
                    continue;
                if(app == APP_STATE_RETURNING)
                    drawPDDemoFrame(entry, inst);

                showFrame();

                if(vpad.trigger & VPAD_BUTTON_B)
                    break;
                if(vpad.trigger & VPAD_BUTTON_A)
                {
                    removeErrorOverlay(ovl);
                    clearRamBuf();
                    entry = te;
                    goto downloadTMD;
                }
            }

            removeErrorOverlay(ovl);
        }
    }

    if(dlDev == NUSDEV_MLC)
    {
        int ovl = addErrorOverlay(gettext(
            "Downloading to NAND is dangerous,\n"
            "it could brick your Wii U!\n\n"

            "Are you sure you want to do this?"));

        while(AppRunning())
        {
            showFrame();

            if(vpad.trigger & VPAD_BUTTON_B)
            {
                removeErrorOverlay(ovl);
                loop = true;
                goto naNedNa;
            }
            if(vpad.trigger & VPAD_BUTTON_A)
                break;
        }

        removeErrorOverlay(ovl);
    }

    uint64_t freeSpace;
    const char *nd = dlDev == NUSDEV_USB01 ? NUSDIR_USB1 : (dlDev == NUSDEV_USB02 ? NUSDIR_USB2 : (dlDev == NUSDEV_SD ? NUSDIR_SD : NUSDIR_MLC)); // TODO: Make const
    if(FSGetFreeSpaceSize(__wut_devoptab_fs_client, getCmdBlk(), (char *)nd, &freeSpace, FS_ERROR_FLAG_ALL) == FS_STATUS_OK && dls > freeSpace)
    {
        char *toFrame = getToFrameBuffer();
        const char *i10n = gettext("Not enough free space on");
        strcpy(toFrame, i10n);
        sprintf(toFrame + strlen(i10n), "  %s\n\n", prettyDir(nd));
        strcat(toFrame, gettext("Press any key to return"));
        int ovl = addErrorOverlay(toFrame);

        while(AppRunning())
        {
            showFrame();

            if(vpad.trigger)
            {
                removeErrorOverlay(ovl);
                loop = true;
                goto naNedNa;
            }
        }

        removeErrorOverlay(ovl);
    }

    TMD *titleTMD = MEMAllocFromDefaultHeap(getRamBufSize());
    OSBlockMove(titleTMD, tmd, getRamBufSize(), false);

    TitleData *titleInfo = MEMAllocFromDefaultHeap(sizeof(TitleData));
    titleInfo->tmd = titleTMD;
    titleInfo->ramBufSize = getRamBufSize();
    titleInfo->entry = entry;
    strcpy(titleInfo->titleVer, titleVer);
    strcpy(titleInfo->folderName, folderName);
    titleInfo->inst = inst;
    titleInfo->dlDev = dlDev;
    titleInfo->toUSB = toUSB;
    titleInfo->keepFiles = keepFiles;

    if(toQueue)
        addToQueue(titleInfo);
    else if(checkSystemTitleFromEntry(entry))
        downloadTitle(tmd, getRamBufSize(), entry, titleVer, folderName, inst, dlDev, toUSB, keepFiles, false);

    clearRamBuf();
}
