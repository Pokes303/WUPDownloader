#ifndef WUPD_TICKET_H
#define WUPD_TICKET_H

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
	extern "C" {
#endif

void generateTik(FILE* tik, char* titleID, char* encKey);
bool generateFakeTicket();

#ifdef __cplusplus
	}
#endif

#endif // ifndef WUPD_TICKET_H
