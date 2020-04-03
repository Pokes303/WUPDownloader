#include <stdbool.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>

#include "mem1.h"

bool mem1ready = false;
MEMHeapHandle handle;

void initMem1()
{
	//TODO
	handle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
	mem1ready = true;
}

void deinitMem1()
{
	MEMFreeToFrmHeap(handle, MEM_FRM_HEAP_FREE_ALL);
	mem1ready = false;
}

void *allocateMem1aligned(size_t size, size_t alignment)
{
	if(!mem1ready)
		return NULL;
	
	if(alignment < 4)
		alignment = 4;
	
	size_t free = MEMGetAllocatableSizeForFrmHeapEx(handle, alignment);
	return free < size ? NULL : MEMAllocFromFrmHeapEx (handle, size, alignment);
}

void *allocateMem1(size_t size)
{
	return allocateMem1aligned(size, 4);
}

void freeMem1(void *ptr)
{
	//TODO: STUB
}
