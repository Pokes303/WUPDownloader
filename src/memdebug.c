/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2020-2021 V10lator <v10lator@myway.de>                    *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *
 ***************************************************************************/

// This is based on Quarkys awesome ASAN from
// https://github.com/QuarkTheAwesome/RetroArch/blob/asan/wiiu/system/memory.c
// Quarky gave permission to relicense these codes under the MIT license.
// Copyright of Quarky applies to this file, too.

// The ASAN isn't ready for multitasking, so we disable it completely for now.
#ifdef SOME_STUPID_NEVER_EVER_DEFINED_VARIABLE // NUSSPLI_DEBUG

#include <wut-fixups.h>

#include <memdebug.h>
#include <utils.h>

#include <string.h>

#include <coreinit/debug.h>
#include <coreinit/dynload.h>
#include <coreinit/fastmutex.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memory.h>

typedef struct
{
	bool allocated;
	void *addr;
	size_t size;
	void *owner;
} lsan_allocation;

#define LSAN_ALLOCS_SZ 0x20000
static volatile lsan_allocation lsan_allocs[LSAN_ALLOCS_SZ];
static volatile MEMHeapHandle asan_heap;
static volatile void *asan_heap_mem;
static volatile unsigned int asan_sz;

static volatile void *asan_shadow;
static volatile unsigned int asan_shadow_off;
static volatile bool asan_ready = false;

static volatile void *codeStart;
static volatile void *codeEnd;

static OSFastMutex asanMutex;

#define MEM_TO_SHADOW(ptr) (unsigned char*)((((void*)ptr - asan_heap_mem) >> 3) + asan_shadow)
#define SET_SHADOW(shadow, ptr) (*shadow) |= 1 << (((unsigned int)ptr) & 0x7)
#define CLR_SHADOW(shadow, ptr) (*shadow) &= ~(1 << (((unsigned int)ptr) & 0x7))
#define GET_SHADOW(shadow, ptr) (*shadow >> (((unsigned int)ptr) & 0x7)) & 0x1

bool asan_checkrange(void* ptr, size_t sz)
{
	if(ptr >= asan_heap_mem && ptr <= asan_shadow)
		for(int i = 0; i < sz; i++)
			if(!GET_SHADOW(MEM_TO_SHADOW(ptr + i), ptr + i))
				return false;
	return true;
}

void asan_getSymbolName(void *ptr, char *out, int outLen)
{
	OSGetSymbolName((unsigned int)ptr, out, outLen);
	if(ptr >= codeStart && ptr < codeEnd)
	{
		char* tmp = strchr(out, '|') + 1;
		strcpy(out, tmp);
	}
}

void asan_doreport(void* ptr, void* caller, const char* type)
{
	debugPrintf("======================== ASAN: out-of-bounds %s", type);
	
	char symbolName[128];
	asan_getSymbolName(caller, symbolName, 127);
	debugPrintf("Source: 0x%08X: %s", caller, symbolName);
	
	//Thanks, LSAN!
	int closest_over = -1, closest_under = -1, uaf = -1;
	for(uint32_t i = 0; i < LSAN_ALLOCS_SZ; i++)
	{
		if(lsan_allocs[i].allocated)
		{
			if(ptr > lsan_allocs[i].addr + lsan_allocs[i].size)
			{
				if(closest_over == -1 || ptr - (lsan_allocs[i].addr + lsan_allocs[i].size) < ptr - (lsan_allocs[closest_over].addr + lsan_allocs[closest_over].size))
					closest_over = i;
			}
			else if(ptr < lsan_allocs[i].addr)
			{
				if (closest_under == -1 || lsan_allocs[i].addr - ptr < lsan_allocs[closest_under].addr - ptr)
					closest_under = i;
			}
		}
		else if(ptr > lsan_allocs[i].addr && ptr < lsan_allocs[i].addr + lsan_allocs[i].size && uaf == -1)
			uaf = i;
	}
	
	debugPrintf("Bad %s was on address 0x%08X. Guessing possible issues:", type, ptr);
	debugPrintf("Guess     Chunk      ChunkSz    Dist       ChunkOwner");
	if(closest_over >= 0)
	{
		asan_getSymbolName(lsan_allocs[closest_over].owner, symbolName, 127);
		debugPrintf("Overflow  0x%08X 0x%08X 0x%08X %s", lsan_allocs[closest_over].addr, lsan_allocs[closest_over].size, ptr - (lsan_allocs[closest_over].addr + lsan_allocs[closest_over].size), symbolName);
	}
	if(closest_under >= 0)
	{
		asan_getSymbolName(lsan_allocs[closest_under].owner, symbolName, 127);
		debugPrintf("Underflow 0x%08X 0x%08X 0x%08X %s", lsan_allocs[closest_under].addr, lsan_allocs[closest_under].size, ptr - (lsan_allocs[closest_under].addr + lsan_allocs[closest_under].size), symbolName);
	}
	if(uaf >= 0)
	{
		asan_getSymbolName(lsan_allocs[uaf].owner, symbolName, 127);
		debugPrintf("UaF       0x%08X 0x%08X 0x%08X %s", lsan_allocs[uaf].addr, lsan_allocs[uaf].size, ptr - (lsan_allocs[uaf].addr + lsan_allocs[uaf].size), symbolName);
	}
}

void __asan_before_dynamic_init(const char *module_name)
{
	debugPrintf("STUB: __asan_before_dynamic_init(%s)", module_name);
}

void __asan_after_dynamic_init()
{
	debugPrintf("STUB: __asan_after_dynamic_init()");
}

void __asan_loadN_noabort(void* ptr, unsigned int size)
{
	debugPrintf("__asan_loadN_noabort()");
	if(!asan_ready)
		return;
	
	while(!OSFastMutex_TryLock(&asanMutex))
		;
	if(!asan_checkrange(ptr, (size_t)size))
		asan_doreport(ptr, __builtin_return_address(0), "load");
	OSFastMutex_Unlock(&asanMutex);
}

void __asan_storeN_noabort(void* ptr, unsigned int size)
{
	debugPrintf("__asan_storeN_noabort()");
	if(!asan_ready)
		return;
	
	while(!OSFastMutex_TryLock(&asanMutex))
		;
	if(!asan_checkrange(ptr, (size_t)size))
		asan_doreport(ptr, __builtin_return_address(0), "store");
	OSFastMutex_Unlock(&asanMutex);
}

#define ASAN_GENFUNC(type, num) void __asan_##type##num##_noabort(void* ptr) \
{ \
debugPrintf("__asan_XY_noabort()"); \
	if(!asan_ready) \
		return; \
	\
	while(!OSFastMutex_TryLock(&asanMutex)) \
		; \
	if(!asan_checkrange(ptr, num)) \
		asan_doreport(ptr, __builtin_return_address(0), #type); \
	OSFastMutex_Unlock(&asanMutex); \
}
ASAN_GENFUNC(load, 1);
ASAN_GENFUNC(store, 1);
ASAN_GENFUNC(load, 2);
ASAN_GENFUNC(store, 2);
ASAN_GENFUNC(load, 4);
ASAN_GENFUNC(store, 4);
ASAN_GENFUNC(load, 8);
ASAN_GENFUNC(store, 8);
ASAN_GENFUNC(load, 16);
ASAN_GENFUNC(store, 16);

void __asan_handle_no_return()
{
	//We only do heap checking, so no need to fiddle here
	debugPrintf("__asan_handle_no_return()");
}

static inline void setShadow(void *ptr, uint32_t size, void *owner) // inline so we don't have to use __builtin_return_address(1)
{
	if(ptr != NULL)
	{
		bool lsan_ok = false;
		while(!OSFastMutex_TryLock(&asanMutex))
			;
		
		for(int i = 0; i < LSAN_ALLOCS_SZ; i++)
			if(!lsan_allocs[i].allocated)
			{
				lsan_allocs[i].allocated = true;
				lsan_allocs[i].addr = ptr;
				lsan_allocs[i].size = size;
				lsan_allocs[i].owner = owner;
				lsan_ok = true;
				break;
			}
		
		if(!lsan_ok)
			debugPrintf("[A/LSAN] WARNING: Too many allocs!");
		else
			for(size_t i = 0; i < size; i++)
				SET_SHADOW(MEM_TO_SHADOW(ptr + i), ptr + i);
		
		OSFastMutex_Unlock(&asanMutex);
	}
}

void *ASANAllocAligned(uint32_t size, int align)
{
	debugPrintf("ASANAllocAligned()");
	if(!asan_ready)
		return NULL;
	
	void *ptr = MEMAllocFromExpHeapEx(asan_heap, size, align);
	setShadow(ptr, size,__builtin_return_address(0));
	return ptr;
}

void *ASANAlloc(uint32_t size)
{
	debugPrintf("ASANAlloc()");
	if(!asan_ready)
		return NULL;
	
	void *ptr = MEMAllocFromExpHeapEx(asan_heap, size, 4);
	setShadow(ptr, size, __builtin_return_address(0));
	return ptr;
}

void ASANFree(void *ptr)
{
	debugPrintf("ASANFree()");
	if(ptr == NULL)
		return;
	
	size_t sz = 0;
	
	bool lsan_ok = false;
	for(int i = 0; i < LSAN_ALLOCS_SZ; i++)
	{
		if(lsan_allocs[i].allocated && lsan_allocs[i].addr == ptr)
		{
			lsan_allocs[i].allocated = false;
			sz = lsan_allocs[i].size; //Thanks, LSAN!
			lsan_ok = true;
			break;
		}
	}
	
	if(!lsan_ok)
	{
		debugPrintf("[LSAN] WARNING: attempted free 0x%08X; not in table!", ptr);
		return;
	}
	
	for(size_t i = 0; i < sz; i++)
		CLR_SHADOW(MEM_TO_SHADOW(ptr + i), ptr + i);
	
	MEMFreeToExpHeap(asan_heap, ptr);
}

MEMAllocFromDefaultHeapFn defaultAllocFn;
MEMAllocFromDefaultHeapExFn defaultAllocAlignedFn;
MEMFreeToDefaultHeapFn defaultFreeFn;

void initASAN(MEMHeapHandle mem2)
{
	OSDynLoad_NotifyData rpxInfo[1];
	OSDynLoad_GetRPLInfo(0, 1, rpxInfo);
	codeStart = rpxInfo[0].textOffset;
	codeEnd = codeStart + rpxInfo[0].textSize;
	
	OSFastMutex_Init(&asanMutex, "NUSspli ASAN");
	OSFastMutex_Unlock(&asanMutex);
	
	for(uint32_t i = 0; i < LSAN_ALLOCS_SZ; i++)
		lsan_allocs[i].allocated = false;
    
	debugPrintf("[LSAN] LSAN initialized. Stack from 0x%08X to 0x%08X.", codeStart, codeEnd);
	
	asan_sz = (MEMGetAllocatableSizeForExpHeapEx(mem2, 4) >> 2) & ~0x3;
	asan_heap_mem = MEMAllocFromDefaultHeap(asan_sz << 1);
	
	asan_heap = MEMCreateExpHeapEx(asan_heap_mem, asan_sz, 0);
	uint32_t asan_heap_sz = MEMGetTotalFreeSizeForExpHeap(asan_heap);
	
	char *multiplierName[2];
	if(asan_heap_sz < 1024)
		multiplierName[0] = "B";
	else if(asan_heap_sz < 1024 * 1024)
	{
		asan_heap_sz >>= 10;
		multiplierName[0] = "KB";
	}
	else
	{
		asan_heap_sz >>= 20;
		multiplierName[0] = "MB";
	}
	
	uint32_t wrapsize = asan_sz;
	if(wrapsize < 1024)
		multiplierName[1] = "B";
	else if(wrapsize < 1024 * 1024)
	{
		wrapsize >>= 10;
		multiplierName[1] = "KB";
	}
	else
	{
		wrapsize >>= 20;
		multiplierName[1] = "MB";
	}
	
    debugPrintf("[ASAN] Allocated wrapheap at 0x%08X, size %d %s. Avail RAM is %d %s.", asan_heap, wrapsize, multiplierName[1], asan_heap_sz, multiplierName[0]);
	
	asan_shadow = (void*)(((unsigned int)asan_heap_mem) + asan_sz);
	asan_shadow_off = (unsigned int)asan_heap - (unsigned int)asan_shadow;
	
	OSBlockSet(asan_shadow, 0, asan_sz);

	debugPrintf("[ASAN] Shadow at 0x%08X. Final shadow address at 0x%08X, final alloced at 0x%08X", asan_shadow, asan_shadow + asan_sz, asan_heap_mem + (asan_sz << 1));
	asan_ready = true;
	
	defaultAllocFn = MEMAllocFromDefaultHeap;
	defaultAllocAlignedFn = MEMAllocFromDefaultHeapEx;
	defaultFreeFn = MEMFreeToDefaultHeap;
	
	MEMAllocFromDefaultHeap = ASANAlloc;
	MEMAllocFromDefaultHeapEx = ASANAllocAligned;
	MEMFreeToDefaultHeap = ASANFree;
}

void deinitASAN()
{
	unsigned long long alc = 0;
	for(uint32_t i = 0; i < LSAN_ALLOCS_SZ; i++)
		if(lsan_allocs[i].allocated)
			alc++;
	debugPrintf("[LSAN] Currently allocated: %llu", alc);
	
	MEMAllocFromDefaultHeap = defaultAllocFn;
	MEMAllocFromDefaultHeapEx = defaultAllocAlignedFn;
	MEMFreeToDefaultHeap = defaultFreeFn;
	
	asan_ready = false;
	
	MEMDestroyExpHeap(asan_heap);
	MEMFreeToDefaultHeap(asan_heap_mem);
}

#endif // ifdef NUSSPLI_DEBUG
