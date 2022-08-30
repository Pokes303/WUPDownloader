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
#include <coreinit/memdefaultheap.h>
#include <deque>
#include <downloader.h>
#include <iostream>
#include <queue.h>

static std::deque<TitleData *> titleQueue;

void addToQueue(TitleData *data)
{
    titleQueue.push_front(data);
}

void proccessQueue()
{
    auto last = std::unique(titleQueue.begin(), titleQueue.end());
    titleQueue.erase(last, titleQueue.end());
    std::sort(titleQueue.begin(), titleQueue.end());
    last = std::unique(titleQueue.begin(), titleQueue.end());
    titleQueue.erase(last, titleQueue.end());
    for(auto title : titleQueue)
    {
        downloadTitle(title->tmd, title->ramBufSize, title->entry, title->titleVer, title->folderName, title->inst, title->dlDev, title->toUSB, title->keepFiles, true);
        MEMFreeToDefaultHeap(title);
    }
    titleQueue.clear();
}

void *getTitleQueue()
{
    return &titleQueue;
}