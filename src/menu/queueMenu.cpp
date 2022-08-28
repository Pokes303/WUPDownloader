/***************************************************************************
 * This file is part of NUSspli.                                           *
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

#include <algorithm>
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

#define MAX_ENTRIES 10

static int cursorPos = 0;
std::deque<TitleData> *titleQueue = getTitleQueue();

static void drawQueueMenu()
{
    startNewFrame();
    char *toScreen = getToFrameBuffer();

    for(int i = 0; i < MAX_ENTRIES; ++i)
    {
        strcpy(toScreen, titleQueue->at(i).entry->name);
        textToFrame(i, 4, toScreen);
    }

    lineToFrame(MAX_LINES - 2, SCREEN_COLOR_WHITE);
    textToFrame(MAX_LINES - 1, 0, gettext("Press " BUTTON_B " to return || " BUTTON_PLUS " to start downloading || " BUTTON_MINUS " to delete an item"));

    arrowToFrame(cursorPos, 0);

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
        {
            return;
        }

        else if(vpad.trigger & VPAD_BUTTON_UP)
        {
            --cursorPos;
            if(cursorPos < 0)
                cursorPos = MAX_ENTRIES;
            redraw = true;
        }
        else if(vpad.trigger & VPAD_BUTTON_DOWN)
        {
            ++cursorPos;
            if(cursorPos > MAX_ENTRIES)
                cursorPos = 0;
            redraw = true;
        }

        if(vpad.trigger & VPAD_BUTTON_PLUS)
        {
            proccessQueue();
            return;
        }

        if(vpad.trigger & VPAD_BUTTON_MINUS)
        {
            titleQueue->erase(titleQueue->begin() + cursorPos);
            return;
        }

        if(redraw)
        {
            drawQueueMenu();
            redraw = false;
        }
    }
}