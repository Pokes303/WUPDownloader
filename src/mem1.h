#ifndef WUPD_MEM1_H
#define WUPD_MEM1_H

#ifdef __cplusplus
	extern "C" {
#endif

void initMem1();
void deinitMem1();
void *allocateMem1aligned(size_t size, size_t alignment);
void *allocateMem1(size_t size);
void freeMem1(void *ptr);

#ifdef __cplusplus
	}
#endif

#endif // ifndef WUPD_MEM_H
