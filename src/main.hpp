#include <stdio.h>
#include <stdlib.h> //rand()
#include <malloc.h>
#include <string>
#include <cstring>
#include <vector>
#include <sys/stat.h> //mkdir

#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>

#include <vpad/input.h>

#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/gfx.h>

#define DEBUG false

extern FSClient *fsCli;
extern FSCmdBlock *fsCmd;

extern VPADStatus vpad;
extern VPADReadError vError;

extern std::string downloadUrl;
extern std::string installDir;

void readInput();