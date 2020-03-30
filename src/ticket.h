#ifndef WUPD_TICKET_H
#define WUPD_TICKET_H

#include <stdbool.h>

#include "main.h"

void generateTik(FILE* tik, char* titleID, char* encKey);
bool generateFakeTicket();

#endif // ifndef WUPD_TICKET_H
