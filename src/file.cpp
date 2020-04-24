#include "file.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <coreinit/time.h>

uint8_t readUInt8(std::string file, uint32_t pos) {
	FILE* fp = fopen((installDir + file).c_str(), "rb");
	fseek(fp, pos, SEEK_SET);
	uint8_t result = 0xFF;
	fread(&result, 1, 1, fp);
	fclose(fp);
	return result;
}
uint16_t readUInt16(std::string file, uint32_t pos) {
	FILE* fp = fopen((installDir + file).c_str(), "rb");
	fseek(fp, pos, SEEK_SET);
	uint16_t result = 0xFFFF;
	fread(&result, 2, 1, fp);
	fclose(fp);
	return result;
}
uint32_t readUInt32(std::string file, uint32_t pos) {
	FILE* fp = fopen((installDir + file).c_str(), "rb");
	fseek(fp, pos, SEEK_SET);
	uint32_t result = 0xFFFFFFFF;
	fread(&result, 4, 1, fp);
	fclose(fp);
	return result;
}
uint64_t readUInt64(std::string file, uint32_t pos) {
	FILE* fp = fopen((installDir + file).c_str(), "rb");
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
	uint8_t bytes[len] = {0};
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
void writeCustomBytes(FILE* fp, std::string str) {
	uint32_t len = str.size();
	uint8_t bytes[len / 2] = {0};
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
int makeDir(const char *path) {
	if (!path)
		return -1;
	
	int res = mkdir(path, 777);
	wlogf("mkdir res: %d", res);
	return res;
}
bool fileExists(const char *path) {
	FILE *temp = fopen(path, "r");
	if (temp == NULL)
		return false;
	
	fclose(temp);
	return true;
}
bool dirExists(const char *path) {
	wlogf("f");
	FSDirectoryHandle dh;
	
	FSStatus fss = FSOpenDir(fsCli, fsCmd, path, &dh, 0);
	wlogf("fss: %u", fss);
	if (fss == FS_STATUS_OK) {
		FSCloseDir(fsCli, fsCmd, dh, 0);
		return true;
	}
	return false;
}