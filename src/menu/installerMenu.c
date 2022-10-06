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
#include <renderer.h>
#include <state.h>
#include <tmd.h>
#include <utils.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>

#include <string.h>

static int cursorPos = MAX_LINES - 4;

static void drawInstallerMenuFrame(const char *name, NUSDEV dev, NUSDEV toDev, bool usbMounted, bool keepFiles)
{
    startNewFrame();

    textToFrame(0, 0, name);

    lineToFrame(MAX_LINES - 5, SCREEN_COLOR_WHITE);

    arrowToFrame(cursorPos, 0);

    char *toFrame = getToFrameBuffer();
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
        textToFrame(MAX_LINES - 4, 4, toFrame);
    else
        textToFrameColored(MAX_LINES - 4, 4, toFrame, SCREEN_COLOR_WHITE_TRANSP);

    strcpy(toFrame, gettext("Keep downloaded files:"));
    strcat(toFrame, " ");
    strcat(toFrame, gettext(keepFiles ? "Yes" : "No"));
    if(dev == NUSDEV_SD)
        textToFrame(MAX_LINES - 3, 4, gettext(toFrame));
    else
        textToFrameColored(MAX_LINES - 3, 4, gettext(toFrame), SCREEN_COLOR_WHITE_TRANSP);

    lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);
    textToFrame(MAX_LINES - 1, 0, gettext("Press " BUTTON_B " to return"));

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
    const char *dir = fileBrowserMenu();
    if(dir == NULL || !AppRunning(true))
        return;

    NUSDEV dev = getDevFromPath(dir);
    NUSDEV toDev = getUSB();
    NUSDEV usbMounted = toDev & NUSDEV_USB;
    if(!usbMounted)
        toDev = NUSDEV_MLC;

    bool keepFiles = dev == NUSDEV_SD;
    const char *nd = prettyDir(dir);
    bool redraw = true;

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
            dir = fileBrowserMenu();
            if(dir == NULL)
                return;

            dev = getDevFromPath(dir);
            if(dev != NUSDEV_SD)
                keepFiles = false;

            nd = prettyDir(dir);
            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_A)
        {
            switch(cursorPos)
            {
                case MAX_LINES - 4:
                    TMD *tmd = getTmd(dir);
                    if(tmd != NULL)
                    {
                        if(checkSystemTitleFromTid(tmd->tid))
                        {
                            if(install(nd, false, dev, dir, toDev & NUSDEV_USB, keepFiles, tmd))
                                showFinishedScreen(nd, FINISHING_OPERATION_INSTALL);
                        }

                        MEMFreeToDefaultHeap(tmd);
                    }
                    else
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
                    }

                    return;
                case MAX_LINES - 3:
                    if(dev == NUSDEV_SD)
                        keepFiles = !keepFiles;
                    break;
            }

            redraw = true;
        }
        else if(vpad.trigger & (VPAD_BUTTON_RIGHT | VPAD_BUTTON_LEFT))
        {
            switch(cursorPos)
            {
                case MAX_LINES - 4:
                    if(usbMounted)
                    {
                        if(toDev & NUSDEV_USB)
                            toDev = NUSDEV_MLC;
                        else
                            toDev = usbMounted;
                    }
                    break;
                case MAX_LINES - 3:
                    if(dev == NUSDEV_SD)
                        keepFiles = !keepFiles;
                    break;
            }

            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_DOWN)
        {
            if(++cursorPos == MAX_LINES - 2)
                cursorPos = MAX_LINES - 4;

            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_UP)
        {
            if(--cursorPos == MAX_LINES - 5)
                cursorPos = MAX_LINES - 3;

            redraw = true;
        }
    }
}
