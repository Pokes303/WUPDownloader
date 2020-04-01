#include "file.h"
#include "input.h"
#include "main.h"
#include "mem.h"
#include "menu.h"
#include "screen.h"
#include "status.h"
#include "ticket.h"
#include "utils.h"

#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#include <coreinit/foreground.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/title.h>

#include <proc_ui/procui.h>

#include <sysapp/launch.h>

#include <curl/curl.h>
#include <nsysnet/socket.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_udp.h>

bool hbl = true;

FSClient *fsCli;
FSCmdBlock *fsCmd;

VPADStatus vpad;
VPADReadError vError;

uint16_t contents = 0xFFFF; //Contents count
uint16_t dcontent = 0xFFFF; //Actual content number

typedef struct {
	uint32_t app;
	bool h3;
	uint64_t size;
} File_to_download;

const char* downloading = "UNKNOWN";
double downloaded = 0;
uint8_t second = 0xFF;
bool showSpeed = true;
char* downloadSpeed = NULL;

bool vibrateWhenFinished = true;
uint8_t vibrationPattern[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void readInput() {
	VPADRead(VPAD_CHAN_0, &vpad, 1, &vError);
	char *err;
	while (vError != VPAD_READ_SUCCESS) {
		err = NULL;
		VPADRead(VPAD_CHAN_0, &vpad, 1, &vError);
		colorStartRefresh(SCREEN_COLOR_LILA);
		write(0, 0, "Error reading the WiiU Gamepad:");
		switch(vError) {
			case VPAD_READ_SUCCESS: //Prevent 'default' detection
				write(2, 1, "Success for an strange reason");
				return;
			case VPAD_READ_NO_SAMPLES:
				write(2, 1, "Waiting for samples...");
				WHBLogPrintf("VPAD Error: Waiting for samples...");
				break;
			case VPAD_READ_INVALID_CONTROLLER:
				write(2, 1, "Invalid controller");
				WHBLogPrintf("VPAD Error: Invalid controller");
				break;
			default:
				err = hex(vError, 8);
				write(2, 1, "Unknown error: 0x");
				write(19,1, err);
//TODO				WHBLogPrintf("VPAD Error: Unknown error: 0x%s", hex(vError, 8).c_str());
				break;
		}
		if(err != NULL)
			freeMemory(err);
		endRefresh();
	}
    VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad.tpNormal, &vpad.tpNormal);
}

size_t writeCallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	return fwrite(ptr, size, nmemb, stream);
}

int multiplier;
char* multiplierName;
bool downloadPaused = false;
static int progressCallback(void *curl, double dltotal, double dlnow, double ultotal, double ulnow) {
	if(AppRunning())
	{
		if(app == 2)
		{
			if(!downloadPaused && curl_easy_pause(curl, CURLPAUSE_ALL) == CURLE_OK)
			{
				downloadPaused = true;
				WHBLogPrintf("Download paused!");
			}
			return 0;
		}
		else if(downloadPaused && curl_easy_pause(curl, CURLPAUSE_CONT) == CURLE_OK)
		{
			downloadPaused = false;
			WHBLogPrintf("Download resumed");
		}
	}
	else
	{
		WHBLogPrintf("Download cancelled!");
		return 1;
	}
	
	// WHBLogPrintf("Downloading: %s (%u/%u) [%u%%] %u / %u bytes", downloading, dcontent, contents, (uint32_t)(dlnow / ((dltotal > 0) ? dltotal : 1) * 100), (uint32_t)dlnow, (uint32_t)dltotal);
	startRefresh();
	if (dltotal == 0)
		write(0, 0, "Preparing");
	else {
		write(0, 0, "Downloading");
		if (dltotal > 0.0D) {
			if (multiplier == 0) {
				if (dltotal < 1024.0D)
					strcpy(multiplierName, "B");
				else if (dltotal < 1024.0D * 1024.0D) {
					multiplier = 1024;
					strcpy(multiplierName, "Kb");
				}
				else {
					multiplier = 1024 * 1024;
					strcpy(multiplierName, "Mb");
				}
			}
			char tmpString[64];
			sprintf(tmpString, "[%d%%] %.2f / %.2f %s", (int)(dlnow / dltotal * 100), dlnow / multiplier, dltotal / multiplier, multiplierName);
			write(0, 1, tmpString);
		}
	}
	
	write(12, 0, downloading);
	if (contents < 0xFFFF)
	{
		char tmpString[16];
		sprintf(tmpString, "(%d/%d)", dcontent + 1, contents);
		write(65, 0, tmpString);
	}
	
	write(65, 1, downloadSpeed);
	
	OSCalendarTime osc;
	OSTicksToCalendarTime(OSGetTime(), &osc);
	if (second != osc.tm_sec) {
		second = osc.tm_sec;
		
		if (dlnow != 0) {
			unsigned int dl = dlnow - downloaded;
			char buf[32];
			if (dl < 1024.0D)
				sprintf(buf, "%d B/s", dl);
			else if (dl < 1024 * 1024)
				sprintf(buf, "%d Kb/s", dl >> 10);
			else
				sprintf(buf, "%d Mb/s", dl >> 20);
			
			downloaded = dlnow;
			strcpy(downloadSpeed, buf);
		}
		else
			strcpy(downloadSpeed, "-- B/s");
	}
	
	writeDownloadLog();
	endRefresh();
	return 0;
}

int downloadFile(char* url, char* file, int type) {
	//Results: 0 = OK | 1 = Error | 2 = No ticket aviable | 3 = Exit
	//Types: 0 = .app | 1 = .h3 | 2 = title.tmd | 3 = tilte.tik
	WHBLogPrintf("Download URL: %s", url);
	WHBLogPrintf("Download PATH: %s", file);
	
	int haystack;
	for(haystack = strlen(file); file[haystack] != '/'; haystack--)
	{
	}
	downloading = &file[++haystack];
	
	struct stat fileStat;
	if(stat(file, &fileStat) == 0)
	{
		char toAdd[512];
		strcpy(toAdd, "Download ");
		strcat(toAdd, downloading);
		strcat(toAdd, " skipped!");
		addToDownloadLog(toAdd);
		return 0;
	}
	
	multiplierName = allocateMemory(sizeof(char) * 4);
	if(multiplierName == NULL)
		return 1;
	strcpy(multiplierName, "Unk");
	multiplier = 0;
	
	second = 0xFF;
	downloaded = 0;
	downloadSpeed[0] = '\0';
	
	FILE *fp = fopen(file, "wb");
	void *writeBuffer = allocateFastMemory(BUFSIZ);
	if(writeBuffer == NULL)
		WHBLogPrintf("WARNING: Couldn't claim fast memory area!");
	else
		setbuf(fp, writeBuffer);
	
	CURL *curl = curl_easy_init();
	if (!curl) {
		if(writeBuffer != NULL)
			freeMemory(writeBuffer);
		freeMemory(multiplierName);
		colorStartRefresh(SCREEN_COLOR_RED);
		write(0, 0, "ERROR: curl_easy_init failed");
		write(0, 2, "File: ");
		write(6, 2, file);
		errorScreen(3, B_RETURN);
		
		return 1;
	}
	
	CURLcode ret = curl_easy_setopt(curl, CURLOPT_URL, url);
	ret |= curl_easy_setopt(curl, CURLOPT_USERAGENT, "WUPDownloader"); //TODO: Spoof eShop here?
	
    ret |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    ret |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	ret |= curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressCallback);
	ret |= curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, curl);
	ret |= curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	
	if(ret != CURLE_OK)
	{
		if(writeBuffer != NULL)
			freeMemory(writeBuffer);
		freeMemory(multiplierName);
		curl_easy_cleanup(curl);
		WHBLogPrintf("curl_easy_setopt error!");
		return 1;
	}
	
	ret = curl_easy_perform(curl);
	WHBLogPrintf("curl_easy_perform returned");
	if(writeBuffer != NULL)
		freeMemory(writeBuffer);
	freeMemory(multiplierName);
	fflush(fp);
	fclose(fp);
	if(ret != CURLE_OK) {
		WHBLogPrintf("curl_easy_perform returned an error: %d", ret);
		curl_easy_cleanup(curl);
		remove(file);
		
		char* err[4];
		int errSize;
		switch(ret) {
			case CURLE_FAILED_INIT:
			case CURLE_COULDNT_RESOLVE_HOST:
				err[0] = "---> Network error";
				err[1] = "Your WiiU is not connected to the internet, check the";
				err[2] = "network settings and try again";
				errSize = 3;
				break;
			case CURLE_WRITE_ERROR:
				err[0] = "---> Error from SD Card";
				err[1] = "The SD Card was extracted or invalid to write data,";
				err[2] = "re-insert it or use another one and restart the app";
				errSize = 3;
				break;
			case CURLE_OPERATION_TIMEDOUT:
			case CURLE_GOT_NOTHING:
			case CURLE_SEND_ERROR:
			case CURLE_RECV_ERROR:
				err[0] = "---> Network error";
				err[1] = "Failed while trying to download data, probably your";
				err[2] = "router was turned off, check the internet connecition";
				err[3] = "and try again";
				errSize = 4;
				break;
			case CURLE_ABORTED_BY_CALLBACK:
				return 0;
			default:
				err[0] = "---> Unknown error";
				err[1] = "See: https://curl.haxx.se/libcurl/c/libcurl-errors.html";
				errSize = 2;
				break;
		}
		
		while (true) {
			readInput();

			colorStartRefresh(SCREEN_COLOR_RED);
			char toScreen[1024];
			sprintf(toScreen, "curl_easy_perform returned a non-valid value: %d", ret);
			write(0, 0, toScreen);
			for (int i = 0; i < errSize; i++)
				write(0, i + 2, err[i]);
			sprintf(toScreen, "File: %s", file);
			write(0, errSize + 3, toScreen);
			errorScreen(errSize + 4, B_RETURN__Y_RETRY); //CHANGE TO RETURN
			endRefresh();
			
			switch (vpad.trigger) {
				case VPAD_BUTTON_B:
					return 1;
				case VPAD_BUTTON_Y:
					writeRetry();
					return downloadFile(url, file, type);
			}
		}
	}
	WHBLogPrintf("curl_easy_perform executed successfully");

	long resp;
	ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp);
	curl_easy_cleanup(curl);
	
	WHBLogPrintf("The download returned: %u", resp);
	if (resp != 200) {
		remove(file);
		if (type == 2 && resp == 404) { //Title.tmd not found
			while(true) {
				readInput();
				
				colorStartRefresh(SCREEN_COLOR_RED);
				write(0, 0, "The download of title.tmd failed with error: 404");
				write(0, 2, "The title cannot be found on the NUS, maybe the provided");
				write(0, 3, "title ID doesn't exists or the TMD was deleted");
				errorScreen(4, B_RETURN__Y_RETRY);
				endRefresh();

				switch (vpad.trigger) {
					case VPAD_BUTTON_B:
						return 1;
					case VPAD_BUTTON_Y:
						writeRetry();
						return downloadFile(url, file, type);
				}
			}
		}
		else if (type == 3 && resp == 404) { //Fake ticket needed
			return 2;
		}
		else {
			int errLn;
			while(true) {
				readInput();
				
				colorStartRefresh(SCREEN_COLOR_RED);
				char toScreen[128];
				sprintf(toScreen, "The download returned a result different to 200 (OK): %ld", resp);
				write(0, 0, toScreen);
				if (resp == 400) {
					write(0, 2, "Request failed. Try again");
					errLn = 4;
				}
				else
					errLn = 2;
				write(0, errLn, "File: ");
				write(6, errLn, file);
				errorScreen(errLn + 1, B_RETURN__Y_RETRY);
				endRefresh();

				switch (vpad.trigger) {
					case VPAD_BUTTON_B:
						return 1;
					case VPAD_BUTTON_Y:
						return downloadFile(url, file, type);
				}
			}
		}
	}
	else {
		WHBLogPrintf("The file was downloaded successfully");
		char toAdd[512];
		strcpy(toAdd, "Download ");
		strcat(toAdd, downloading);
		strcat(toAdd, " finished!");
		addToDownloadLog(toAdd);
	}
	
	return 0;
}

bool downloadTitle(char* titleID, char* titleVer, char* folderName) {
	WHBLogPrintf("Downloading title... tID: %s, tVer: %s, fName: %s", titleID, titleVer, folderName);
	
	char downloadUrl[256];
	strcpy(downloadUrl, DOWNLOAD_URL);
	strcat(downloadUrl, titleID);
	strcat(downloadUrl, "/");
	
	if(strlen(folderName) == 0)
		folderName = titleID;
	else {
		strcat(folderName, " [");
		strcat(folderName, titleID);
		strcat(folderName, "]");
	}
	if (strlen(titleVer) > 0) {
		strcat(folderName, " [v");
		strcat(folderName, titleVer);
		strcat(folderName, "]");
	}
	
	char installDir[256];
	strcpy(installDir, "/vol/external01/install/");
	strcat(installDir, folderName);
	strcat(installDir, "/");
	
	char toScreen[512];
	strcpy(toScreen, "Started the download of: ");
	strcat(toScreen, titleID);
	addToDownloadLog(toScreen);
	strcpy(toScreen, "The content will be saved on \"sd:/install/");
	strcat(toScreen, folderName);
	strcat(toScreen, "\"");
	addToDownloadLog(toScreen);
	
	WHBLogPrintf("Creating directory");
	if(mkdir(installDir, 777) == -1)
	{
		int err = errno;
		if(errno == EEXIST)
			addToDownloadLog("WARNING: The download directory already exists");
		else
		{
			sprintf(toScreen, "Error creating directory: %d %s", err, strerror(err));
			addToDownloadLog(toScreen);
		}
	}
	else
		addToDownloadLog("Download directory successfully created");
	
	startRefresh();
	write(0, 0, "Preparing the download...");
	writeDownloadLog();
	endRefresh();
	
	WHBLogPrintf("Downloading TMD...");
	char tDownloadUrl[256];
	strcpy(tDownloadUrl, downloadUrl);
	strcat(tDownloadUrl, "tmd");
	if(strlen(titleVer) > 0)
	{
		strcat(tDownloadUrl, ".");
		strcat(tDownloadUrl, titleVer);
	}
	char tInstallDir[256];
	strcpy(tInstallDir, installDir);
	strcat(tInstallDir, "title.tmd");
	if (downloadFile(tDownloadUrl, tInstallDir, 2))
	{
		WHBLogPrintf("Error downloading TMD");
		return true;
	}
	addToDownloadLog("TMD Downloaded");
	
	strcpy(toScreen, "=>Title type: ");
	switch (readUInt32(tInstallDir, 0x18C)) { //Title type
		case 0x00050000:
			strcat(toScreen, "eShop or Packed");
			break;
		case 0x00050002:
			strcat(toScreen, "eShop/Kiosk demo");
			break;
		case 0x0005000C:
			strcat(toScreen, "eShop DLC");
			break;
		case 0x0005000E:
			strcat(toScreen, "eShop Update");
			break;
		case 0x00050010:
			strcat(toScreen, "System Application");
			break;
		case 0x0005001B:
			strcat(toScreen, "System Data Archive");
			break;
		case 0x00050030:
			strcat(toScreen, "Applet");
			break;
		// vWii //
		case 0x7:
			strcat(toScreen, "Wii IOS");
			break;
		case 0x00070002:
			strcat(toScreen, "vWii Default Channel");
			break;
		case 0x00070008:
			strcat(toScreen, "vWii System Channel");
			break;
		default:
			strcat(toScreen, "Unknown (");
			char *h = hex(readUInt32(tInstallDir, 0x18C), 8);
			strcat(toScreen, h);
			freeMemory(h);
			strcat(toScreen, ")");
			break;
	}
	addToDownloadLog(toScreen);
	strcpy(tDownloadUrl, downloadUrl);
	strcat(tDownloadUrl, "cetk");
	strcpy(tInstallDir, installDir);
	strcat(tInstallDir, "title.tik");
	int tikRes = downloadFile(tDownloadUrl, tInstallDir, 3);
	if (tikRes == 1)
		return true;
	else if (tikRes == 2) {
		addToDownloadLog("Title.tik not found on the NUS. A fake ticket can be created");
		char encKey[33];
		while (true) {
			readInput();

			colorStartRefresh(SCREEN_COLOR_LILA);
			strcpy(toScreen, "Input the Encrypted title key of ");
			strcat(toScreen, titleID);
			strcat(toScreen, " to create a");
			write(0, 0, toScreen);
			write(0, 1, " fake ticket (Without it, the download cannot be installed)");
			write(0, 3, "You can get it from any 'WiiU title key site'");
			write(0, 5, "Press (A) to show the keyboard [32 hexadecimal numbers]");
			write(0, 6, "Press (B) to continue the download without the fake ticket");
			endRefresh();

			if (vpad.trigger == VPAD_BUTTON_A) {
				if (showKeyboard(encKey, CHECK_HEXADECIMAL, 32, true)) {
					startRefresh();
					write(0, 0, "Creating fake title.tik");
					writeDownloadLog();
					endRefresh();
					FILE *tik = fopen(tInstallDir, "wb");
					generateTik(tik, titleID, encKey);
					fflush(tik);
					fclose(tik);
					addToDownloadLog("Fake ticket created successfully");
					break;
				}
			}
			else if (vpad.trigger == VPAD_BUTTON_B) {
				addToDownloadLog("Encrypted key wasn't wrote. Continuing without fake ticket");
				addToDownloadLog("(The download needs a fake ticket to be installed)");
				break;
			}
		}
	}

	strcpy(tInstallDir, installDir);
	strcat(tInstallDir, "title.tmd");
	contents = readUInt16(tInstallDir, 0x1DE);
	File_to_download ftd[contents]; //Files to download

	//Get .app and .h3 files
	for (int i = 0; i < contents; i++) {
		ftd[i].app = readUInt32(tInstallDir, 0xB04 + i * 0x30); //.app file
		ftd[i].h3 = readUInt16(tInstallDir, 0xB0A + i * 0x30) == 0x2003 ? true : false; //.h3?
		ftd[i].size = readUInt64(tInstallDir, 0xB0C + i * 0x30); //size
	}
	
	char *tmpHex;
	char tmpFileName[256];
	for (dcontent = 0; dcontent < contents; dcontent++) {
		tmpHex = hex(ftd[dcontent].app, 8);
		strcpy(tDownloadUrl, downloadUrl);
		strcat(tDownloadUrl, tmpHex);
		strcpy(tmpFileName, installDir);
		strcat(tmpFileName, tmpHex);
		strcpy(tInstallDir, tmpFileName);
		strcat(tInstallDir, ".app");
		if (downloadFile(tDownloadUrl, tInstallDir, 0) == 1)
		{
			freeMemory(tmpHex);
			return true;
		}
		strcpy(tInstallDir, tmpFileName);
		strcat(tInstallDir, ".h3");
		if (ftd[dcontent].h3) { //.h3 download
			strcat(tmpHex, ".h3");
			strcat(tDownloadUrl, ".h3");
			if (downloadFile(tDownloadUrl, tInstallDir, 1) == 1)
			{
				freeMemory(tmpHex);
				return true;
			}
		}
		freeMemory(tmpHex);
	}
	
	WHBLogPrintf("Creating CERT...");
	startRefresh();
	write(0, 0, "Creating CERT");
	writeDownloadLog();
	endRefresh();
	
    FILE* cert;
	strcpy(tmpFileName, installDir);
	strcat(tmpFileName, "title.cert");
	cert = fopen(tmpFileName, "wb");
	
	writeCustomBytes(cert, "000100042EA66C66CFF335797D0497B77A197F9FE51AB5A41375DC73FD9E0B10669B1B9A5B7E8AB28F01B67B6254C14AA1331418F25BA549004C378DD72F0CE63B1F7091AAFE3809B7AC6C2876A61D60516C43A63729162D280BE21BE8E2FE057D8EB6E204242245731AB6FEE30E5335373EEBA970D531BBA2CB222D9684387D5F2A1BF75200CE0656E390CE19135B59E14F0FA5C1281A7386CCD1C8EC3FAD70FBCE74DEEE1FD05F46330B51F9B79E1DDBF4E33F14889D05282924C5F5DC2766EF0627D7EEDC736E67C2E5B93834668072216D1C78B823A072D34FF3ECF9BD11A29AF16C33BD09AFB2D74D534E027C19240D595A68EBB305ACC44AB38AB820C6D426560C");
	writeVoidBytes(cert, 0x3C);
	writeCustomBytes(cert, "526F6F742D43413030303030303033");
	writeVoidBytes(cert, 0x34);
	writeCustomBytes(cert, "0143503030303030303062");
	writeVoidBytes(cert, 0x36);
	writeCustomBytes(cert, "137A080BA689C590FD0B2F0D4F56B632FB934ED0739517B33A79DE040EE92DC31D37C7F73BF04BD3E44E20AB5A6FEAF5984CC1F6062E9A9FE56C3285DC6F25DDD5D0BF9FE2EFE835DF2634ED937FAB0214D104809CF74B860E6B0483F4CD2DAB2A9602BC56F0D6BD946AED6E0BE4F08F26686BD09EF7DB325F82B18F6AF2ED525BFD828B653FEE6ECE400D5A48FFE22D538BB5335B4153342D4335ACF590D0D30AE2043C7F5AD214FC9C0FE6FA40A5C86506CA6369BCEE44A32D9E695CF00B4FD79ADB568D149C2028A14C9D71B850CA365B37F70B657791FC5D728C4E18FD22557C4062D74771533C70179D3DAE8F92B117E45CB332F3B3C2A22E705CFEC66F6DA3772B00010001");
	writeVoidBytes(cert, 0x35);
	writeCustomBytes(cert, "010003704138EFBBBDA16A987DD901326D1C9459484C88A2861B91A312587AE70EF6237EC50E1032DC39DDE89A96A8E859D76A98A6E7E36A0CFE352CA893058234FF833FCB3B03811E9F0DC0D9A52F8045B4B2F9411B67A51C44B5EF8CE77BD6D56BA75734A1856DE6D4BED6D3A242C7C8791B3422375E5C779ABF072F7695EFA0F75BCB83789FC30E3FE4CC8392207840638949C7F688565F649B74D63D8D58FFADDA571E9554426B1318FC468983D4C8A5628B06B6FC5D507C13E7A18AC1511EB6D62EA5448F83501447A9AFB3ECC2903C9DD52F922AC9ACDBEF58C6021848D96E208732D3D1D9D9EA440D91621C7A99DB8843C59C1F2E2C7D9B577D512C166D6F");
	writeCustomBytes(cert, "7E1AAD4A774A37447E78FE2021E14A95D112A068ADA019F463C7A55685AABB6888B9246483D18B9C806F474918331782344A4B8531334B26303263D9D2EB4F4BB99602B352F6AE4046C69A5E7E8E4A18EF9BC0A2DED61310417012FD824CC116CFB7C4C1F7EC7177A17446CBDE96F3EDD88FCD052F0B888A45FDAF2B631354F40D16E5FA9C2C4EDA98E798D15E6046DC5363F3096B2C607A9D8DD55B1502A6AC7D3CC8D8C575998E7D796910C804C495235057E91ECD2637C9C1845151AC6B9A0490AE3EC6F47740A0DB0BA36D075956CEE7354EA3E9A4F2720B26550C7D394324BC0CB7E9317D8A8661F42191FF10B08256CE3FD25B745E5194906B4D61CB4C2E");
	writeVoidBytes(cert, 0x3C);
	writeCustomBytes(cert, "526F6F74");
	writeVoidBytes(cert, 0x3F);
	writeCustomBytes(cert, "0143413030303030303033");
	writeVoidBytes(cert, 0x36);
	writeCustomBytes(cert, "7BE8EF6CB279C9E2EEE121C6EAF44FF639F88F078B4B77ED9F9560B0358281B50E55AB721115A177703C7A30FE3AE9EF1C60BC1D974676B23A68CC04B198525BC968F11DE2DB50E4D9E7F071E562DAE2092233E9D363F61DD7C19FF3A4A91E8F6553D471DD7B84B9F1B8CE7335F0F5540563A1EAB83963E09BE901011F99546361287020E9CC0DAB487F140D6626A1836D27111F2068DE4772149151CF69C61BA60EF9D949A0F71F5499F2D39AD28C7005348293C431FFBD33F6BCA60DC7195EA2BCC56D200BAF6D06D09C41DB8DE9C720154CA4832B69C08C69CD3B073A0063602F462D338061A5EA6C915CD5623579C3EB64CE44EF586D14BAAA8834019B3EEBEED37900010001");
	writeVoidBytes(cert, 0x35);
	writeCustomBytes(cert, "010004919EBE464AD0F552CD1B72E7884910CF55A9F02E50789641D896683DC005BD0AEA87079D8AC284C675065F74C8BF37C88044409502A022980BB8AD48383F6D28A79DE39626CCB2B22A0F19E41032F094B39FF0133146DEC8F6C1A9D55CD28D9E1C47B3D11F4F5426C2C780135A2775D3CA679BC7E834F0E0FB58E68860A71330FC95791793C8FBA935A7A6908F229DEE2A0CA6B9B23B12D495A6FE19D0D72648216878605A66538DBF376899905D3445FC5C727A0E13E0E2C8971C9CFA6C60678875732A4E75523D2F562F12AABD1573BF06C94054AEFA81A71417AF9A4A066D0FFC5AD64BAB28B1FF60661F4437D49E1E0D9412EB4BCACF4CFD6A3408847982");
	writeVoidBytes(cert, 0x3C);
	writeCustomBytes(cert, "526F6F742D43413030303030303033");
	writeVoidBytes(cert, 0x34);
	writeCustomBytes(cert, "0158533030303030303063");
	writeVoidBytes(cert, 0x36);
	writeCustomBytes(cert, "137A0894AD505BB6C67E2E5BDD6A3BEC43D910C772E9CC290DA58588B77DCC11680BB3E29F4EABBB26E98C2601985C041BB14378E689181AAD770568E928A2B98167EE3E10D072BEEF1FA22FA2AA3E13F11E1836A92A4281EF70AAF4E462998221C6FBB9BDD017E6AC590494E9CEA9859CEB2D2A4C1766F2C33912C58F14A803E36FCCDCCCDC13FD7AE77C7A78D997E6ACC35557E0D3E9EB64B43C92F4C50D67A602DEB391B06661CD32880BD64912AF1CBCB7162A06F02565D3B0ECE4FCECDDAE8A4934DB8EE67F3017986221155D131C6C3F09AB1945C206AC70C942B36F49A1183BCD78B6E4B47C6C5CAC0F8D62F897C6953DD12F28B70C5B7DF751819A983465262500010001");
	writeVoidBytes(cert, 0x34);
	
	fclose(cert);
	
	addToDownloadLog("Cert created!");
	
	for (int i = 0; i < 0x10; i++)
		VPADControlMotor(VPAD_CHAN_0, vibrationPattern, 0xF);
	
	enableShutdown(); //TODO
	
	while (AppRunning()) {
		if (app == 2)
			continue;
		
		readInput();
	
		colorStartRefresh(SCREEN_COLOR_GREEN);
		strcpy(toScreen, "Title ");
		strcat(toScreen, titleID);
		strcat(toScreen, " downloaded successfully");
		write(0, 0, toScreen);
		write(0, 1, "Press (A) to return");
		writeDownloadLog();
		endRefresh();
		
		if (vpad.trigger == VPAD_BUTTON_A) {
			clearDownloadLog();
			return true;
		}
	}
	return false;
}

int main() {
	WHBLogUdpInit();
	WHBLogPrintf("Initialising libraries...");
	
	if (OSGetTitleID() != 0 &&
        OSGetTitleID() != 0x000500101004A200 && // Mii Maker EUR
        OSGetTitleID() != 0x000500101004A100 && // Mii Maker USA
        OSGetTitleID() != 0x000500101004A000 && // Mii Maker JPN
        OSGetTitleID() != 0x0005000013374842) { // HBL Channel
			hbl = false;
			ProcUIInit(&OSSavesDone_ReadyToRelease);
		}
		else //HBL = true
			WHBProcInit();
	
	WHBLogPrintf("Init");
	
	initMemoryAllocator();
	FSInit();
	fsCli = (FSClient*)allocateMemory(sizeof(FSClient));
	FSAddClient(fsCli, 0);
	fsCmd = (FSCmdBlock*)allocateMemory(sizeof(FSCmdBlock));
	FSInitCmdBlock(fsCmd);
	WHBLogPrintf("FS started successfully");
	
	SWKBD_Init();
	WHBLogPrintf("SWKBD started successfully");
	
	initScreen();
	WHBLogPrintf("OSScreen started successfully");
	
	initRandom();
	WHBLogPrintf("Random started successfully");
	
	mkdir(INSTALL_DIR, 777);
	
	downloadSpeed = allocateMemory(sizeof(char) * 32);
	if(downloadSpeed == NULL)
		goto exit;
	
	while(AppRunning()) {
		if (app == 2)
			continue;
		
		readInput();
		
		writeMainMenu();
		
		switch (vpad.trigger) {
			case VPAD_BUTTON_A: { //Download title
				char titleID[17];
				char titleVer[33];
				char folderName[FILENAME_MAX + 1];
				titleID[0] = titleVer[0] = folderName[0] = '\0';
				
				while (AppRunning()) {
					if (app == 2)
						continue;
		
					readInput();

					startRefresh();
					write(0, 0, "Input a title ID to download their content [Ex: 000500001234ABCD]");
					write(0, 2, "If a fake ticket.tik is needed, you will need to write his");
					write(0, 3, " encrypted tile key (Check it on any 'WiiU title key site')");
					write(0, 5, "[USE THE VALID ENCRYPTED TITLE KEY FOR EACH TITLE, OTHERWISE, ");
					write(0, 6, " IT WILL THROW A TITLE.TIK ERROR WHILE INSTALLING IT]");
					write(0, 8, "Downloaded titles can be installed with WUP Installer GX2");
					write(0, 10, "Press (A) to show the keyboard [Only input hexadecimal numbers]");
					write(0, 11, "Press (B) to return");
					endRefresh();

					if (vpad.trigger == VPAD_BUTTON_A) {
						if (showKeyboard(titleID, CHECK_HEXADECIMAL, 16, true))
							goto dnext;
					}
					else if (vpad.trigger == VPAD_BUTTON_B)
						goto mainLoop;
				}
				break;
				
dnext:
				while(AppRunning()) {
					if (app == 2)
						continue;
		
					readInput();

					startRefresh();
					write(0, 0, "Provided title ID [Only 16 digit hexadecimal]:");
					write(3, 1, titleID);
					
					write(0, 2, "Provided title version [Only numbers]:");
					write(3, 3, titleVer[0] == '\0' ? "<LATEST>" : titleVer);
					
					write(0, 4, "Custom folder name [Only text and numbers]:");
					write(3, 5, "sd:/install/");
					write(15, 5, folderName[0] == '\0' ?  titleID :  folderName);
					
					write(0, 7, "Press (A) to download");
					write(0, 8, "Press (B) to return");
					paintLine(9, SCREEN_COLOR_WHITE);
					write(0, 10, "Press (UP) to set the title ID");
					write(0, 11, "Press (RIGHT) to set the title version");
					write(0, 12, "Press (DOWN) to set a custom name to the download folder");
					paintLine(13, SCREEN_COLOR_WHITE);
					char toScreen[59];
					
					strcpy(toScreen, "Press (Y) to ");
					strcat(toScreen, vibrateWhenFinished ? "deactivate" : "activate");
					strcat(toScreen, " the vibration when download finish");
					write(0, 14, toScreen); //Thinking to change this to activate HOME Button led
					endRefresh();
					
					if (vpad.trigger == VPAD_BUTTON_A)
						goto ddnext;
					else if (vpad.trigger == VPAD_BUTTON_B)
						goto mainLoop;
					else if (vpad.trigger == VPAD_BUTTON_UP) {
						char tmpTitleID[17];
						strcpy(tmpTitleID, titleID);
						if (!showKeyboard(titleID, CHECK_HEXADECIMAL, 16, true))
							strcpy(titleID, tmpTitleID);
					}
					else if (vpad.trigger == VPAD_BUTTON_RIGHT) {
						if (!showKeyboard(titleVer, CHECK_NUMERICAL, 5, false))
							titleVer[0] = '\0';
					}
					else if (vpad.trigger == VPAD_BUTTON_DOWN) {
						if (!showKeyboard(folderName, CHECK_NOSPECIAL, FILENAME_MAX, false))
							folderName[0] = '\0';
					}
					else if (vpad.trigger == VPAD_BUTTON_Y)
						vibrateWhenFinished = !vibrateWhenFinished;
					
				}
				break;
ddnext:
				disableShutdown(); //TODO

				if (!downloadTitle(titleID, titleVer, folderName))
					goto exit;
				
				break;
			}
			case VPAD_BUTTON_Y:
				if (!generateFakeTicket())
					goto exit;
				break;
			case VPAD_BUTTON_B:
				if (!hbl)
					SYSLaunchMenu();
				else
					goto exit;
		}
mainLoop:;
	}
exit:
	if(downloadSpeed != NULL)
		freeMemory(downloadSpeed);
	SWKBD_Shutdown();
	if (hbl)
		shutdownScreen();
	
	FSDelClient(fsCli, 0);
	freeMemory(fsCli);
	freeMemory(fsCmd);
   
	FSShutdown();
	if (hbl)
		WHBProcShutdown();
	WHBLogUdpDeinit();
	deinitMemoryAllocator();

	return 1;
}