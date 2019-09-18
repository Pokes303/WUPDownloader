#include "main.hpp"
	
uint8_t readUInt8(std::string file, uint32_t pos);
uint16_t readUInt16(std::string file, uint32_t pos);
uint32_t readUInt32(std::string file, uint32_t pos);
uint64_t readUInt64(std::string file, uint32_t pos);

void initRandom();
void writeVoidBytes(FILE* fp, uint32_t length);
uint8_t charToByte(char c);
void writeCustomBytes(FILE* fp, std::string str);
void writeRandomBytes(FILE* fp, uint32_t length);

int makeDir(const char *path);
bool fileExists(const char *path);
bool dirExists(const char *path);