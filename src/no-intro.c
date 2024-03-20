/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2023 V10lator <v10lator@myway.de>                         *
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
#include <ioQueue.h>
#include <no-intro.h>
#include <ticket.h>
#include <tmd.h>
#include <utils.h>

#include <stdbool.h>

#include <coreinit/filesystem_fsa.h>
#include <coreinit/memdefaultheap.h>

void destroyNoIntroData(NO_INTRO_DATA *data)
{
    MEMFreeToDefaultHeap(data->path);
    MEMFreeToDefaultHeap(data);
}

void revertNoIntro(NO_INTRO_DATA *data)
{
    char *newPath = MEMAllocFromDefaultHeapEx(FS_MAX_PATH, 0x40);
    if(newPath == NULL)
    {
        debugPrintf("EOM!");
        return;
    }

    size_t s = strlen(data->path);
    OSBlockMove(newPath, data->path, s + 1, false);
    char *dataP = data->path + s;
    char *toP = newPath + s;

    OSBlockMove(dataP, "title.tik", strlen("title.tik") + 1, false);
    FSError ret;

    flushIOQueue();
    if(!data->hadTicket)
    {
        ret = FSARemove(getFSAClient(), data->path);
        if(ret != FS_ERROR_OK)
            debugPrintf("Can't remove %s: %s", data->path, translateFSErr(ret));
    }
    else
    {
        OSBlockMove(toP, "cetk", strlen("cetk") + 1, false);
        ret = FSARename(getFSAClient(), data->path, newPath);
        if(ret != FS_ERROR_OK)
            debugPrintf("Can't move %s to %s: %s", data->path, newPath, translateFSErr(ret));
    }

    OSBlockMove(dataP, "title.cert", strlen("title.cert") + 1, false);
    ret = FSARemove(getFSAClient(), data->path);
    if(ret != FS_ERROR_OK)
        debugPrintf("Can't remove %s: %s", data->path, translateFSErr(ret));

    OSBlockMove(dataP, "title.tmd", strlen("title.tmd") + 1, false);
    OSBlockMove(toP, "tmd", strlen("tmd") + 1, false);
    ret = FSARename(getFSAClient(), data->path, newPath);
    if(ret != FS_ERROR_OK)
        debugPrintf("Can't move %s to %s: %s", data->path, newPath, translateFSErr(ret));

    // TODO: Rename .app files
    *dataP = '\0';
    FSADirectoryHandle dir;
    ret = FSAOpenDir(getFSAClient(), data->path, &dir);
    if(ret == FS_ERROR_OK)
    {
        FSADirectoryEntry entry;
        toP[8] = '\0';
        while(FSAReadDir(getFSAClient(), dir, &entry) == FS_ERROR_OK)
        {
            if((entry.info.flags & FS_STAT_DIRECTORY) || strlen(entry.name) != 12 || strcmp(entry.name + 8, ".app") != 0)
                continue;

            OSBlockMove(dataP, entry.name, 13, false);
            OSBlockMove(toP, entry.name, 8, false);
            if(FSARename(getFSAClient(), data->path, newPath) != FS_ERROR_OK)
                debugPrintf("Can't move %s to %s: %s", data->path, newPath, translateFSErr(ret));
        }

        FSACloseDir(getFSAClient(), dir);
    }
    else
        debugPrintf("Can't open %s: %s", data->path, translateFSErr(ret));

    destroyNoIntroData(data);
    MEMFreeToDefaultHeap(newPath);
}

NO_INTRO_DATA *transformNoIntro(const char *path)
{
    char *newPath = MEMAllocFromDefaultHeapEx(FS_MAX_PATH, 0x40);
    if(newPath == NULL)
    {
        debugPrintf("EOM!");
        return NULL;
    }

    char *pathTo = MEMAllocFromDefaultHeapEx(FS_MAX_PATH, 0x40);
    if(pathTo == NULL)
    {
        MEMFreeToDefaultHeap(newPath);
        debugPrintf("EOM!");
        return NULL;
    }

    NO_INTRO_DATA *data = MEMAllocFromDefaultHeap(sizeof(NO_INTRO_DATA));
    if(data == NULL)
    {
        MEMFreeToDefaultHeap(newPath);
        MEMFreeToDefaultHeap(pathTo);
        debugPrintf("EOM!");
        return NULL;
    }

    size_t s = strlen(path);
    OSBlockMove(newPath, path, s, false);
    if(newPath[s - 1] != '/')
        newPath[s++] = '/';

    newPath[s] = '\0';

    OSBlockMove(pathTo, newPath, s + 1, false);

    char *p = newPath + s;
    FSADirectoryHandle dir;
    FSError ret = FSAOpenDir(getFSAClient(), newPath, &dir);
    if(ret != FS_ERROR_OK)
    {
        debugPrintf("Can't open %s: %s", newPath, translateFSErr(ret));
        MEMFreeToDefaultHeap(newPath);
        MEMFreeToDefaultHeap(pathTo);
        MEMFreeToDefaultHeap(data);
        return NULL;
    }

    FSADirectoryEntry entry;
    char *fromP = newPath + s;
    char *toP = pathTo + s;

    data->path = newPath;
    data->hadTicket = false;
    data->tmdFound = false;
    data->ac = 0;

    while(FSAReadDir(getFSAClient(), dir, &entry) == FS_ERROR_OK)
    {
        if(entry.info.flags & FS_STAT_DIRECTORY)
            continue;

        strcpy(fromP, entry.name);
        if(strcmp(entry.name, "tmd") == 0)
        {
            OSBlockMove(fromP, "tmd", strlen("tmd") + 1, false);
            OSBlockMove(toP, "title.tmd", strlen("title.tmd") + 1, false);
            ret = FSARename(getFSAClient(), newPath, pathTo);
            if(ret != FS_ERROR_OK)
            {
                debugPrintf("Can't move %s to %s: %s", newPath, pathTo, translateFSErr(ret));
                MEMFreeToDefaultHeap(pathTo);
                FSACloseDir(getFSAClient(), dir);
                *fromP = '\0';
                revertNoIntro(data);
                return NULL;
            }

            data->tmdFound = true;
        }
        else if(strcmp(entry.name, "cetk") == 0)
        {
            OSBlockMove(fromP, "cetk", strlen("cetk") + 1, false);
            OSBlockMove(toP, "title.tik", strlen("title.tik") + 1, false);
            ret = FSARename(getFSAClient(), newPath, pathTo);
            if(ret != FS_ERROR_OK)
            {
                debugPrintf("Can't move %s to %s: %s", newPath, pathTo, translateFSErr(ret));
                MEMFreeToDefaultHeap(pathTo);
                FSACloseDir(getFSAClient(), dir);
                *fromP = '\0';
                revertNoIntro(data);
                return NULL;
            }

            data->hadTicket = true;
        }
        else if(strlen(entry.name) == 8)
        {
            OSBlockMove(fromP, entry.name, 9, false);
            OSBlockMove(toP, entry.name, 8, false);
            OSBlockMove(toP + 8, ".app", strlen(".app") + 1, false);
            ret = FSARename(getFSAClient(), newPath, pathTo);
            if(ret != FS_ERROR_OK)
            {
                debugPrintf("Can't move %s to %s: %s", newPath, pathTo, translateFSErr(ret));
                MEMFreeToDefaultHeap(pathTo);
                FSACloseDir(getFSAClient(), dir);
                *fromP = '\0';
                revertNoIntro(data);
                return NULL;
            }

            data->ac++;
        }
    }

    FSACloseDir(getFSAClient(), dir);
    MEMFreeToDefaultHeap(pathTo);

    if(!data->tmdFound || !data->ac)
    {
        revertNoIntro(data);
        return NULL;
    }

    *fromP = '\0';

#ifndef NUSSPLI_LITE
    TMD *tmd = getTmd(newPath, false);
    if(tmd == NULL)
    {
        *fromP = '\0';
        revertNoIntro(data);
        return NULL;
    }

    OSBlockMove(fromP, "title.tik", strlen("title.tik") + 1, false);
    if(!data->hadTicket)
    {
        debugPrintf("Creating ticket at at %s", newPath);
        if(!generateTik(newPath, getTitleEntryByTid(tmd->tid), tmd))
        {
            debugPrintf("Error creating ticket at %s", newPath);
            *fromP = '\0';
            revertNoIntro(data);
            return NULL;
        }
    }

    OSBlockMove(fromP, "title.cert", strlen("title.cert") + 1, false);
    debugPrintf("Creating cert at %s", newPath);
    if(!generateCert(tmd, NULL, 0, newPath))
    {
        debugPrintf("Error creating cert at %s", newPath);
        *fromP = '\0';
        revertNoIntro(data);
        return NULL;
    }
#endif

    *fromP = '\0';
    return data;
}
