#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> //rand()
#include <malloc.h>
#include <string.h>
#include <sys/stat.h> //mkdir

#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>

#include <vpad/input.h>

#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/gfx.h>

#define VERSION "1.2"

extern bool hbl;

extern FSClient *fsCli;
extern FSCmdBlock *fsCmd;

extern VPADStatus vpad;
extern VPADReadError vError;

#define DOWNLOAD_URL "http://ccs.cdn.wup.shop.nintendo.net/ccs/download/"
#define INSTALL_DIR "/vol/external01/install/"

void readInput();