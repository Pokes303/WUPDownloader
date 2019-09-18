#include "main.hpp"

bool initScreen();
void shutdownScreen();

void startRefresh();
void endRefresh();

void write(uint32_t row, uint32_t column, const char* str);
void swrite(uint32_t row, uint32_t column, std::string str);
void writeParsed(uint32_t row, uint32_t column, const char* str);

void addToDownloadLog(std::string str);
void clearDownloadLog();
void writeDownloadLog();

void colorStartRefresh(uint32_t color);

enum ErrorOptions {
	B_RETURN = 0,
	B_RETURN__A_CONTINUE = 1,
	B_RETURN__Y_RETRY = 2
};

void errorScreen(uint32_t line, ErrorOptions option);
void writeRetry();

void enableShutdown();
void disableShutdown();

std::string b_tostring(bool b);

std::string hex(uint64_t i); //ex: 50D1
std::string hex0(uint64_t i, uint8_t digits); //ex: 000050D1