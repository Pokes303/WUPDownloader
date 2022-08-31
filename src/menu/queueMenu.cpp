/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2022 Xpl0itU <DaThinkingChair@protonmail.com>             *
 * Copyright (c) 2022 V10lator <v10lator@myway.de>                         *
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

#include <algorithm>
#include <coreinit/memdefaultheap.h>
#include <cstring>
#include <deque>
#include <input.h>
#include <localisation.h>
#include <menu/main.h>
#include <menu/queueMenu.h>
#include <menu/utils.h>
#include <queue.h>
#include <renderer.h>
#include <state.h>

#define MAX_ENTRIES (MAX_LINES - 3)

static int cursorPos = 1;
static std::deque<TitleData *> *titleQueue = getTitleQueue();

static void drawQueueMenu()
{
    startNewFrame();
    boxToFrame(0, MAX_LINES - 2);
    char *toScreen = getToFrameBuffer();
    int p;

    for(int i = 0; i < MAX_ENTRIES && i < titleQueue->size(); ++i)
    {
        if(isDLC(titleQueue->at(i)->entry))
        {
            strcpy(toScreen, "[DLC] ");
            p = strlen("[DLC] ");
        }
        else if(isUpdate(titleQueue->at(i)->entry))
        {
            strcpy(toScreen, "[UPD] ");
            p = strlen("[UPD] ";
        }
        else
            p = 0;

        strcpy(toScreen + p, titleQueue->at(i)->entry->name);
        flagToFrame(i + 1, 7, titleQueue->at(i)->entry->region);
        deviceToFrame(i + 1, 4, titleQueue->at(i)->toUSB ? DEVICE_TYPE_USB : DEVICE_TYPE_NAND);
        textToFrame(i + 1, 10, toScreen);
    }

    lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);

    strcpy(toScreen, gettext("Press " BUTTON_B " to return"));
    strcat(toScreen, " || ");
    strcat(toScreen, gettext(BUTTON_PLUS " to start downloading"));
    strcat(toScreen, " || ");
    strcat(toScreen, gettext(BUTTON_MINUS " to delete an item"));
    textToFrame(MAX_LINES - 1, toScreen);
    if(titleQueue->size() != 0)
        arrowToFrame(cursorPos, 1);

    drawFrame();
}

void queueMenu()
{
    drawQueueMenu();

    bool redraw = false;
    while(AppRunning())
    {
        if(app == APP_STATE_BACKGROUND)
            continue;
        if(app == APP_STATE_RETURNING)
            drawQueueMenu();

        showFrame();

        if(vpad.trigger & VPAD_BUTTON_B)
            return;

        if(vpad.trigger & VPAD_BUTTON_UP)
        {
            if(--cursorPos < 1)
                cursorPos = titleQueue->size();
            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_DOWN)
        {
            if(++cursorPos > titleQueue->size())
                cursorPos = 1;
            redraw = true;
        }

        if(vpad.trigger & VPAD_BUTTON_PLUS)
        {
            proccessQueue();
            return;
        }

        if(vpad.trigger & VPAD_BUTTON_MINUS)
        {
            MEMFreeToDefaultHeap(titleQueue->at(cursorPos - 1));
            titleQueue->erase(titleQueue->begin() + cursorPos - 1);
            --cursorPos;
            redraw = true;
        }

        if(redraw)
        {
            drawQueueMenu();
            redraw = false;
        }
    }
}
