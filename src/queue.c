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

#include <downloader.h>
#include <list.h>
#include <queue.h>

#include <coreinit/memdefaultheap.h>

static LIST *titleQueue;

bool initQueue()
{
    titleQueue = createList();
    return titleQueue != NULL;
}

void shutdownQueue()
{
    destroyList(titleQueue, true);
}

void addToQueue(TitleData *data)
{
    addToListBeginning(titleQueue, data);
}

bool proccessQueue()
{
    TitleData *title;
    TitleData *last = NULL;
    forEachListEntry(titleQueue, title)
    {
        if(!downloadTitle(title->tmd, title->tmdSize, title->entry, title->titleVer, title->folderName, title->inst, title->dlDev, title->toUSB, title->keepFiles))
            return false;

        if(last != NULL)
        {
            removeFromList(titleQueue, last);
            MEMFreeToDefaultHeap(last);
        }

        last = title;
    }

    clearList(titleQueue, true);
    return true;
}

LIST *getTitleQueue()
{
    return titleQueue;
}
