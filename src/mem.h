#ifndef WUPD_MEM_H
#define WUPD_MEM_H

#include <coreinit/memheap.h>

#ifdef __cplusplus
	extern "C" {
#endif

void initMemoryAllocator(void);
void deinitMemoryAllocator(void);
void *allocateMemory(size_t size);
void *aallocateMemory(size_t size, size_t align);
void *allocateFastMemory(size_t size);
void *aallocateFastMemory(size_t size, size_t align);
void freeMemory(void *ptr);

#ifdef __cplusplus
	}
#endif

#endif // ifndef WUPD_MEM_H
