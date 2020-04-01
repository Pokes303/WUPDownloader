#include "mem.h"

#include <stdbool.h>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memheap.h>

typedef struct
{
	MEMBaseHeapType type;
} MemoryDescription;

bool allocatorInitialized = false;
MEMHeapHandle mem1_handle;

void initMemoryAllocator()
{
	mem1_handle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
	allocatorInitialized = true;
}

void deinitMemoryAllocator()
{
	allocatorInitialized = false;
}

inline void *allocateMemory(size_t size)
{
	return aallocateMemory(size, 4);
}

void *aallocateMemory(size_t size, size_t align)
{
	if(!allocatorInitialized)
		return NULL;
	
	void *allocatedMem = MEMAllocFromDefaultHeapEx(size + sizeof(MemoryDescription), align);
	if(allocatedMem == NULL)
		return aallocateFastMemory(size, align);
	
	((MemoryDescription *)allocatedMem)->type = MEM_BASE_HEAP_MEM2;
	allocatedMem += sizeof(MemoryDescription);
	return allocatedMem;
}

inline void *allocateFastMemory(size_t size)
{
	return aallocateFastMemory(size, 4); //TODO
}

void *aallocateFastMemory(size_t size, size_t align)
{
	WHBLogPrintf("Trying to claim fast memory...");
	if(!allocatorInitialized)
	{
		WHBLogPrintf("Allocator not ready!");
		return NULL;
	}
	
	size += sizeof(MemoryDescription);
	if(MEMGetAllocatableSizeForExpHeapEx(mem1_handle, align) < size)
	{
		WHBLogPrintf("%d < %d", MEMGetAllocatableSizeForExpHeapEx(mem1_handle, align), size);
		return NULL;
	}
	
	void *ptr = MEMAllocFromExpHeapEx(mem1_handle, size, align);
	if(ptr != NULL)
	{
		((MemoryDescription *)ptr)->type = MEM_BASE_HEAP_MEM2;
		ptr += sizeof(MemoryDescription);
	}
	else
		WHBLogPrintf("Got NULL from Caffee!");
	
	return ptr; //TODO
}

void freeMemory(void *ptr)
{
	if(!allocatorInitialized)
		return;
	
	ptr -= sizeof(MemoryDescription);
	switch(((MemoryDescription *)ptr)->type)
	{
		case MEM_BASE_HEAP_MEM2:
			MEMFreeToDefaultHeap(ptr);
			break;
		case MEM_BASE_HEAP_MEM1:
			MEMFreeToExpHeap(mem1_handle, ptr);
			break;
		default: // Schould never be reached
			//TODO
			break;
	}
}
