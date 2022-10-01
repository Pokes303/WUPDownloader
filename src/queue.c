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
    clearQueue();
    destroyList(titleQueue, false);
}

bool addToQueue(TitleData *data)
{
    return addToListEnd(titleQueue, data);
}

static inline void removeFQ(TitleData *title)
{
    if(title != NULL)
    {
        removeFromList(titleQueue, title);
        MEMFreeToDefaultHeap(title->tmd);
        MEMFreeToDefaultHeap(title);
    }
}

bool proccessQueue()
{
    TitleData *title;
    uint64_t sizes[3] = { 0, 0, 0 };

    forEachListEntry(titleQueue, title)
    {
        for(uint16_t i = 0; i < title->tmd->num_contents; ++i)
        {
            if(title->inst)
                sizes[title->toUSB ? 0 : 2] += title->tmd->contents[i].size;

            if(title->keepFiles)
            {
                int j = title->dlDev & NUSDEV_USB ? 0 : (title->dlDev & NUSDEV_SD ? 1 : 2);
                if((title->tmd->contents[i].type & 0x0003) == 0x0003)
                    sizes[j] += 20;

                sizes[j] += title->tmd->contents[i].size;
            }
        }
    }

    for(int i = 0; i < 3; ++i)
    {
        if(sizes[i] != 0)
        {
            NUSDEV toCheck;
            switch(i)
            {
                case 0:
                    toCheck = getUSB();
                    break;
                case 1:
                    toCheck = NUSDEV_SD;
                    break;
                default:
                    toCheck = NUSDEV_MLC;
            }

            if(!checkFreeSpace(toCheck, sizes[i]))
                return false;
        }
    }

    TitleData *last = NULL;
    forEachListEntry(titleQueue, title)
    {
        removeFQ(last);
        if(!downloadTitle(title->tmd, title->tmdSize, title->entry, title->titleVer, title->folderName, title->inst, title->dlDev, title->toUSB, title->keepFiles))
            return false;

        last = title;
    }

    removeFQ(last);
    return true;
}

bool removeFromQueue(uint32_t index)
{
    TitleData *title = getAndRemoveFromList(titleQueue, index);
    if(title == NULL)
        return false;

    MEMFreeToDefaultHeap(title->tmd);
    MEMFreeToDefaultHeap(title);
    return true;
}

void clearQueue()
{
    TitleData *title;
    TitleData *last = NULL;
    forEachListEntry(titleQueue, title)
    {
        removeFQ(last);
        last = title;
    }

    removeFQ(last);
}

LIST *getTitleQueue()
{
    return titleQueue;
}
