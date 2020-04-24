#include "log.hpp"
#include <whb/log_udp.h>
#include <whb/log.h>
#include <cstdarg>

void wlog(const char* str) {
#ifdef DEBUG
	WHBLogPrint(str);
#endif
}
void wlogf(const char* str, ...) {
#ifdef DEBUG
	char buffer[256];
	va_list args;
	va_start(args, str);
	vsnprintf(buffer, 256, str, args);
	WHBLogPrint(buffer);
	va_end(args);
#endif
}