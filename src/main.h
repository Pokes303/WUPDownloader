#ifndef WUPD_MAIN_H
#define WUPD_MAIN_H

#include <stdbool.h>

#include <coreinit/filesystem.h>

#include <vpad/input.h>

#define VERSION "1.2"

#ifdef __cplusplus
	extern "C" {
#endif

extern bool hbl;

extern FSClient *fsCli;
extern FSCmdBlock *fsCmd;

extern VPADStatus vpad;
extern VPADReadError vError;

#define DOWNLOAD_URL "http://ccs.cdn.wup.shop.nintendo.net/ccs/download/"
#define INSTALL_DIR "/vol/external01/install/"

void readInput();

#ifdef __cplusplus
	}
#endif

#endif // ifndef WUPD_MAIN_H
