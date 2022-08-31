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

#include <wut-fixups.h>

#include <input.h>
#include <list.h>
#include <localisation.h>
#include <menu/main.h>
#include <menu/queueMenu.h>
#include <menu/utils.h>
#include <queue.h>
#include <renderer.h>
#include <state.h>

#include <string.h>

#define MAX_ENTRIES (MAX_LINES - 3)

static int cursorPos = 1;

static void drawQueueMenu(LIST *titleQueue)
{
    startNewFrame();
    boxToFrame(0, MAX_LINES - 2);
    char *toScreen = getToFrameBuffer();
    int p;
    TitleData *data;

    for(int i = 0; i < MAX_ENTRIES && i < getListSize(titleQueue); ++i)
    {
        data = getContent(titleQueue, i);

        if(isDLC(data->entry))
        {
            strcpy(toScreen, "[DLC] ");
            p = strlen("[DLC] ");
        }
        else if(isUpdate(data->entry))
        {
            strcpy(toScreen, "[UPD] ");
            p = strlen("[UPD] ");
        }
        else
            p = 0;

        strcpy(toScreen + p, data->entry->name);
        flagToFrame(i + 1, 7, data->entry->region);
        deviceToFrame(i + 1, 4, data->toUSB ? DEVICE_TYPE_USB : DEVICE_TYPE_NAND);
        textToFrame(i + 1, 10, toScreen);
    }

    lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);

    strcpy(toScreen, gettext("Press " BUTTON_B " to return"));
    strcat(toScreen, " || ");
    strcat(toScreen, gettext(BUTTON_PLUS " to start downloading"));
    strcat(toScreen, " || ");
    strcat(toScreen, gettext(BUTTON_MINUS " to delete an item"));
    textToFrame(MAX_LINES - 1, ALIGNED_CENTER, toScreen);
    if(getListSize(titleQueue) != 0)
        arrowToFrame(cursorPos, 1);

    drawFrame();
}

void queueMenu()
{
    LIST *titleQueue = getTitleQueue();
    drawQueueMenu(titleQueue);

    bool redraw = false;
    while(AppRunning())
    {
        if(app == APP_STATE_BACKGROUND)
            continue;
        if(app == APP_STATE_RETURNING)
            drawQueueMenu(titleQueue);

        showFrame();

        if(vpad.trigger & VPAD_BUTTON_B)
            return;

        if(vpad.trigger & VPAD_BUTTON_UP)
        {
            if(--cursorPos < 1)
                cursorPos = getListSize(titleQueue);
            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_DOWN)
        {
            if(++cursorPos > getListSize(titleQueue))
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
            removeContent(titleQueue, --cursorPos, true);
            redraw = true;
        }

        if(redraw)
        {
            drawQueueMenu(titleQueue);
            redraw = false;
        }
    }
}
