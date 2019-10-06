#include "status.hpp"

#include "utils.hpp"
#include "input.hpp"

#include <coreinit/core.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>
#include <whb/proc.h>
#include <whb/gfx.h>

uint8_t app = 1;
uint8_t AppRunning() {
	if (!hbl) {
		if(!OSIsMainCore()) {
			WHBLogPrintf("Os not main core");
			ProcUISubProcessMessages(true);
		}
		else {
			WHBLogPrintf("haha yess");
			ProcUIStatus status = ProcUIProcessMessages(true);
			WHBLogPrintf("status: %d", status);
		
			switch (status) {
				case PROCUI_STATUS_EXITING: {
					WHBLogPrintf("exiting");
					// Being closed, deinit, free, and prepare to exit
					WHBLogPrintf("procui");
					ProcUIShutdown();
					WHBLogPrintf("screen");
					WHBLogPrintf("ok!");
					app = 0;
					break;
				}
				case PROCUI_STATUS_RELEASE_FOREGROUND: {
					WHBLogPrintf("release");
					// Free up MEM1 to next foreground app, deinit screen, etc.
					
					ProcUIDrawDoneRelease();
					
					//shutdownScreen();
					
					WHBGfxInit();
					WHBGfxBeginRender();

					WHBGfxBeginRenderTV();
					WHBGfxClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					WHBGfxFinishRenderTV();
					
					WHBGfxBeginRenderDRC();
					WHBGfxClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					WHBGfxFinishRenderDRC();
					
					WHBGfxFinishRender();
					
					WHBLogPrintf("draw ok");
					app = 2;
					WHBLogPrintf("-------------------");
					//OSScreenShutdown();??
					break;
				}
				case PROCUI_STATUS_IN_FOREGROUND: {
					WHBLogPrintf("foreground");
					if (app == 2) {
						WHBGfxShutdown();
						shutdownScreen();
						initScreen();
					}
					
					app = 1;
					// Executed while app is in foreground
					break;
				}
				case PROCUI_STATUS_IN_BACKGROUND: {
					WHBLogPrintf("background");
					app = 2;
					break;
				}
				default: {
					WHBLogPrintf("default");
				}
			}
			WHBLogPrintf("app: %d", app);
			return app;
		}
	}
	else
		return WHBProcIsRunning();
	
	return 1;
}