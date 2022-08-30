/***************************************************************************
 * This file is part of NUSspli.                                           *
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

#pragma once

#include <wut-fixups.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct ELEMENT ELEMENT;
    struct ELEMENT
    {
        void *content;
        ELEMENT *next;
    };

    typedef struct
    {
        ELEMENT *first;
        ELEMENT *last;
    } LIST;

    static inline LIST *createList()
    {
        LIST *ret = MEMAllocFromDefaultHeap(sizeof(LIST));
        ret->first = ret->last = NULL;
        return ret;
    }

    static inline void clearList(LIST *list, bool freeContents)
    {
        ELEMENT *tmp;
        while(list->first != NULL)
        {
            tmp = list->first;
            list->first = tmp->next;
            if(freeContents)
                MEMFreeToDefaultHeap(tmp->content);

            MEMFreeToDefaultHeap(tmp);
        }

        list->last = NULL;
    }

    static inline void destroyList(LIST *list, bool freeContents)
    {
        clearList(list, freeContents);
        MEMFreeToDefaultHeap(list);
    }

    static inline void addToListBeginning(LIST *list, void *content)
    {
        ELEMENT *newElement = MEMAllocFromDefaultHeap(sizeof(LIST));
        if(newElement == NULL)
            return;

        newElement->content = content;
        newElement->next = list->first;
        list->first = newElement;

        if(list->last == NULL)
            list->last = newElement;
    }

    static inline void addToListEnd(LIST *list, void *content)
    {
        ELEMENT *newElement = MEMAllocFromDefaultHeap(sizeof(LIST));
        if(newElement == NULL)
            return;

        newElement->content = content;
        newElement->next = NULL;
        if(list->last == NULL)
        {
            list->last = list->first = newElement;
            return;
        }

        list->last->next = newElement;
    }

#define forEachListEntry(x) for(ELEMENT *cur = x->first, void *content; cur != NULL && (content = cur->content); cur = cur->next)

    static inline void removeFromList(LIST *list, void *content, bool freeContent)
    {
        if(list->first == NULL)
            return;

        if(list->first->content == content)
        {
            ELEMENT *tmp = list->first;
            list->first = tmp->next;
            if(freeContent)
                MEMFreeToDefaultHeap(tmp->content);

            MEMFreeToDefaultHeap(tmp);

            if(list->first == NULL)
                list->last = NULL;

            return;
        }

        ELEMENT *last = list->first;
        for(ELEMENT *cur = list->first->next; cur != NULL; last = cur, cur = cur->next)
        {
            if(cur->content == content)
            {
                last->next = cur->next;
                MEMFreeToDefaultHeap(cur);
                return;
            }
        }
    }

    static inline void *getContent(LIST *list, uint32_t index)
    {
        ELEMENT *entry = list->first;
        for(uint32_t i = 0; i < index && entry != NULL; ++i)
            entry = entry->next;

        return entry == NULL ? NULL : entry->content;
    }

    static inline void *getAndRemoveFromList(LIST *list, uint32_t index)
    {
        ELEMENT *last = list->first;
        ELEMENT *entry = last;
        for(uint32_t i = 0; i < index && entry != NULL; ++i)
        {
            last = entry;
            entry = entry->next;
        }

        if(entry == NULL)
            return NULL;

        if(last == entry)
            list->first = entry->next;
        else
            last->next = entry->next;

        void *ret = entry->content;
        MEMFreeToDefaultHeap(entry);

        if(list->first == NULL)
            list->last = NULL;
        return ret;
    }

    static inline void *wrapLastEntry(LIST *list)
    {
        // TODO
        return NULL;
    }

#ifdef __cplusplus
}
#endif
