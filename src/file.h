#ifndef WUPD_FILE_H
#define WUPD_FILE_H

#include <stdbool.h>

#include "main.h"
	
uint8_t readUInt8(char* file, uint32_t pos);
uint16_t readUInt16(char* file, uint32_t pos);
uint32_t readUInt32(char* file, uint32_t pos);
uint64_t readUInt64(char* file, uint32_t pos);

void initRandom();
void writeVoidBytes(FILE* fp, uint32_t length);
uint8_t charToByte(char c);
void writeCustomBytes(FILE* fp, char* str);
void writeRandomBytes(FILE* fp, uint32_t length);

bool fileExists(const char *path);
bool dirExists(const char *path);

#endif // ifndef WUPD_FILE_H
