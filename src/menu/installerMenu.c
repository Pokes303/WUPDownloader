/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
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

#include <file.h>
#include <filesystem.h>
#include <input.h>
#include <installer.h>
#include <localisation.h>
#include <menu/filebrowser.h>
#include <menu/utils.h>
#include <queue.h>
#include <renderer.h>
#include <state.h>
#include <tmd.h>
#include <utils.h>

#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#include <string.h>

static int cursorPos = MAX_LINES - 5;

static bool addToOpQueue(const TitleEntry *entry, const char *dir, TMD *tmd, NUSDEV fromDev, bool toUSB, bool keepFiles)
{
    TitleData *titleInfo = MEMAllocFromDefaultHeapEx(FS_ALIGN(sizeof(TitleData)) + FS_MAX_PATH, 0x40);
    if(titleInfo == NULL)
        return false;

    strcpy(getPathFromInstData(titleInfo), dir);

    titleInfo->tmd = tmd;
#ifndef NUSSPLI_LITE
    titleInfo->operation = OPERATION_INSTALL;
#endif
    titleInfo->entry = entry;
    titleInfo->dlDev = fromDev;
    titleInfo->toUSB = toUSB;
    titleInfo->keepFiles = keepFiles;

    int ret = addToQueue(titleInfo);
    if(ret == 1)
        return true;

    MEMFreeToDefaultHeap(titleInfo);

    MEMFreeToDefaultHeap(tmd);
    return ret;
}

static void drawInstallerMenuFrame(const char *name, NUSDEV dev, NUSDEV toDev, bool usbMounted, bool keepFiles)
{
    startNewFrame();
    textToFrame(0, 0, name);
    arrowToFrame(cursorPos, 0);

    char *toFrame = getToFrameBuffer();
    strcpy(toFrame, gettext("Press " BUTTON_B " to return"));
    strcat(toFrame, " || ");
    strcat(toFrame, gettext(BUTTON_PLUS " to start"));
    textToFrame(MAX_LINES - 2, ALIGNED_CENTER, toFrame);

    strcpy(toFrame, gettext("Install to:"));
    strcat(toFrame, " ");
    switch((int)toDev)
    {
        case NUSDEV_USB01:
        case NUSDEV_USB02:
            strcat(toFrame, "USB");
            break;
        case NUSDEV_MLC:
            strcat(toFrame, "NAND");
            break;
    }

    if(usbMounted)
        textToFrame(MAX_LINES - 5, 4, toFrame);
    else
        textToFrameColored(MAX_LINES - 5, 4, toFrame, SCREEN_COLOR_WHITE_TRANSP);

    strcpy(toFrame, gettext("Keep downloaded files:"));
    strcat(toFrame, " ");
    strcat(toFrame, gettext(keepFiles ? "Yes" : "No"));
    if(dev == NUSDEV_SD)
        textToFrame(MAX_LINES - 4, 4, gettext(toFrame));
    else
        textToFrameColored(MAX_LINES - 4, 4, gettext(toFrame), SCREEN_COLOR_WHITE_TRANSP);

    lineToFrame(MAX_LINES - 3, SCREEN_COLOR_WHITE);

    strcpy(toFrame, gettext("Press " BUTTON_B " to return"));
    strcat(toFrame, " || ");
    strcat(toFrame, gettext(BUTTON_PLUS " to start"));
    textToFrame(MAX_LINES - 2, ALIGNED_CENTER, toFrame);

    textToFrame(MAX_LINES - 1, ALIGNED_CENTER, gettext(BUTTON_MINUS " to add to the queue"));

    drawFrame();
}

static NUSDEV getDevFromPath(const char *path)
{
    if(strncmp(NUSDIR_SD, path, strlen(NUSDIR_SD)) == 0)
        return NUSDEV_SD;
    if(strncmp(NUSDIR_USB1, path, strlen(NUSDIR_USB1)) == 0)
        return NUSDEV_USB01;
    if(strncmp(NUSDIR_USB2, path, strlen(NUSDIR_USB2)) == 0)
        return NUSDEV_USB02;
    if(strncmp(NUSDIR_MLC, path, strlen(NUSDIR_MLC)) == 0)
        return NUSDEV_MLC;

    return NUSDEV_NONE;
}

void installerMenu()
{
    const char *dir = fileBrowserMenu(true);
    if(dir == NULL || !AppRunning(true))
        return;

    NUSDEV dev = getDevFromPath(dir);
    bool keepFiles = dev == NUSDEV_SD;

    NUSDEV toDev = getUSB();
    NUSDEV usbMounted = toDev & NUSDEV_USB;
    if(!usbMounted)
        toDev = NUSDEV_MLC;

    TMD *tmd;
    const TitleEntry *entry;
    const char *nd;
    bool redraw;

refreshDir:
    tmd = getTmd(dir);
    if(tmd == NULL)
    {
        drawErrorFrame(gettext("Invalid title.tmd file!"), ANY_RETURN);

        while(AppRunning(true))
        {
            if(app == APP_STATE_BACKGROUND)
                continue;
            if(app == APP_STATE_RETURNING)
                drawErrorFrame(gettext("Invalid title.tmd file!"), ANY_RETURN);

            showFrame();
            if(vpad.trigger)
                break;
        }

        goto grabNewDir;
    }

    dev = getDevFromPath(dir);
    if(dev != NUSDEV_SD)
        keepFiles = false;

    entry = getTitleEntryByTid(tmd->tid);
    nd = entry == NULL ? prettyDir(dir) : entry->name;
    redraw = true;

    while(AppRunning(true))
    {
        if(app == APP_STATE_BACKGROUND)
            continue;
        if(app == APP_STATE_RETURNING)
            redraw = true;

        if(redraw)
        {
            drawInstallerMenuFrame(nd, dev, toDev, usbMounted, keepFiles);
            redraw = false;
        }
        showFrame();

        if(vpad.trigger & VPAD_BUTTON_B)
        {
            MEMFreeToDefaultHeap(tmd);
            goto grabNewDir;
        }

        if(vpad.trigger & VPAD_BUTTON_PLUS)
        {
            switch(cursorPos)
            {
                case MAX_LINES - 5:
                    if(checkSystemTitleFromTid(tmd->tid))
                    {
                        if(install(nd, false, dev, dir, toDev & NUSDEV_USB, keepFiles, tmd))
                            showFinishedScreen(nd, FINISHING_OPERATION_INSTALL);
                    }

                    MEMFreeToDefaultHeap(tmd);
                    return;
            }

            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_MINUS)
        {
            if(!addToOpQueue(entry, dir, tmd, dev, toDev & NUSDEV_USB, keepFiles))
                return;

            goto grabNewDir;
        }
        else if(vpad.trigger & (VPAD_BUTTON_A | VPAD_BUTTON_RIGHT | VPAD_BUTTON_LEFT))
        {
            switch(cursorPos)
            {
                case MAX_LINES - 5:
                    if(usbMounted)
                    {
                        if(toDev & NUSDEV_USB)
                            toDev = NUSDEV_MLC;
                        else
                            toDev = usbMounted;
                    }
                    break;
                case MAX_LINES - 4:
                    if(dev == NUSDEV_SD)
                        keepFiles = !keepFiles;
                    break;
            }

            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_DOWN)
        {
            if(++cursorPos == MAX_LINES - 3)
                cursorPos = MAX_LINES - 5;

            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_UP)
        {
            if(--cursorPos == MAX_LINES - 6)
                cursorPos = MAX_LINES - 4;

            redraw = true;
        }
    }

    MEMFreeToDefaultHeap(tmd);
    return;

grabNewDir:
    if(!AppRunning(true))
        return;

    dir = fileBrowserMenu(true);
    if(dir != NULL)
        goto refreshDir;
}
