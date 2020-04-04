#ifndef WUPD_UTILS_H
#define WUPD_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
	extern "C" {
#endif

void enableShutdown();
void disableShutdown();

char* b_tostring(bool b);

char* hex(uint64_t i, int digits); //ex: 000050D1

bool pathExists(char *path);

#ifdef __cplusplus
	}
#endif

#endif // ifndef WUPD_UTILS_H
