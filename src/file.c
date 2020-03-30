#include "file.h"
#include "utils.h"

#include <stdbool.h>

#include <coreinit/time.h>

uint8_t readUInt8(char* file, uint32_t pos) {
	FILE* fp = fopen(file, "rb");
	fseek(fp, pos, SEEK_SET);
	uint8_t result = 0xFF;
	fread(&result, 1, 1, fp);
	fclose(fp);
	return result;
}
uint16_t readUInt16(char* file, uint32_t pos) {
	FILE* fp = fopen(file, "rb");
	fseek(fp, pos, SEEK_SET);
	uint16_t result = 0xFFFF;
	fread(&result, 2, 1, fp);
	fclose(fp);
	return result;
}
uint32_t readUInt32(char* file, uint32_t pos) {
	FILE* fp = fopen(file, "rb");
	fseek(fp, pos, SEEK_SET);
	uint32_t result = 0xFFFFFFFF;
	fread(&result, 4, 1, fp);
	fclose(fp);
	return result;
}
uint64_t readUInt64(char* file, uint32_t pos) {
	FILE* fp = fopen(file, "rb");
	fseek(fp, pos, SEEK_SET);
	uint64_t result = 0xFFFFFFFFFFFFFFFF;
	fread(&result, 8, 1, fp);
	fclose(fp);
	return result;
}

void initRandom() {
	srand(OSGetTime());
}
void writeVoidBytes(FILE* fp, uint32_t len) {
	uint8_t bytes[len];
	memset(bytes, 0, sizeof(uint8_t) * len);
	fwrite(bytes, len, 1, fp);
}
uint8_t charToByte(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 0xA;
	else if (c >= 'a' && c <= 'f')
		return c - 'A' + 0xA;
	return 0x77;
}
void writeCustomBytes(FILE* fp, char* str) {
	uint32_t len = sizeof(str);
	
	uint8_t bytes[len / 2];
	memset(bytes, 0, (sizeof(uint8_t) / 2) * len); // Hopefully the compiler will optimize sizeof(uint8_t) / 2) * len to a fixed number!
	for (uint32_t i = 0; i < len; i++) {
		uint8_t b = charToByte(str[i]);
		bytes[i / 2] += + ((i % 2 == 0) ? b * 0x10 : b);
	}
	fwrite(bytes, len / 2, 1, fp);
}
void writeRandomBytes(FILE* fp, uint32_t len) {
	uint8_t bytes[len];
	for (uint32_t i = 0; i < len; i++) {
		bytes[i] = rand() % 0xFF;
	}
	fwrite(bytes, len, 1, fp);
}
//Ported from various sites
bool fileExists(const char *path) {
	FILE *temp = fopen(path, "r");
	if (temp == NULL)
		return false;
	
	fclose(temp);
	return true;
}
bool dirExists(const char *path) {
	WHBLogPrintf("f");
	FSDirectoryHandle dh;
	
	FSStatus fss = FSOpenDir(fsCli, fsCmd, path, &dh, 0);
	WHBLogPrintf("fss: %u", fss);
	if (fss == FS_STATUS_OK) {
		FSCloseDir(fsCli, fsCmd, dh, 0);
		return true;
	}
	return false;
}