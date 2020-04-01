#include "mem.h"

#include <coreinit/memdefaultheap.h>
#include <coreinit/memheap.h>

typedef struct
{
	MEMBaseHeapType type;
} MemoryDescription;

inline void *allocateMemory(size_t size)
{
	return aallocateMemory(size, 4);
}

void *aallocateMemory(size_t size, size_t align)
{
	void *allocatedMem = MEMAllocFromDefaultHeapEx(size + sizeof(MemoryDescription), align);
	if(allocatedMem == NULL)
	{
		allocatedMem = atallocateMemory(MEM_BASE_HEAP_FG, size, align);
		if(allocatedMem == NULL)
			allocatedMem = atallocateMemory(MEM_BASE_HEAP_MEM2, size, align);
		return allocatedMem;
	}
	
	((MemoryDescription *)allocatedMem)->type = MEM_BASE_HEAP_MEM1;
	allocatedMem += sizeof(MemoryDescription);
	return allocatedMem;
}

inline void *tallocateMemory(MEMBaseHeapType type, size_t size)
{
	return atallocateMemory(type, size, 4); //TODO
}

void *atallocateMemory(MEMBaseHeapType type, size_t size, size_t align)
{
	return NULL; //TODO
}

void freeMemory(void *ptr)
{
	ptr -= sizeof(MemoryDescription);
	MemoryDescription *desc = ptr;
	if(desc->type == MEM_BASE_HEAP_MEM1)
	{
		MEMFreeToDefaultHeap(ptr);
		return;
	}
	//TODO: MEM2 / FG
}
