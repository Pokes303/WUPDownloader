#include "input.h"
#include "main.h"
#include "screen.h"
#include "status.h"
#include "utils.h"

#include <coreinit/core.h>
#include <coreinit/screen.h>
#include <proc_ui/procui.h>
#include <vpad/input.h>
#include <whb/proc.h>
#include <whb/gfx.h>

int app = 1;
int AppRunning() {
	if (!hbl) {
		if(!OSIsMainCore())
			ProcUISubProcessMessages(true);
		else {
			ProcUIStatus status = ProcUIProcessMessages(true);
		
			switch (status) {
				case PROCUI_STATUS_EXITING: {
					// Being closed, deinit, free, and prepare to exit
					ProcUIShutdown();
					app = 0;
					break;
				}
				case PROCUI_STATUS_RELEASE_FOREGROUND: {
					// Free up MEM1 to next foreground app, deinit screen, etc.
					ProcUIDrawDoneRelease();
					
					WHBGfxInit();
					WHBGfxBeginRender();

					WHBGfxBeginRenderTV();
					WHBGfxClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					WHBGfxFinishRenderTV();
					
					WHBGfxBeginRenderDRC();
					WHBGfxClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					WHBGfxFinishRenderDRC();
					
					WHBGfxFinishRender();
					
					app = 2;
					break;
				}
				case PROCUI_STATUS_IN_FOREGROUND: {
					// Executed while app is in foreground
					if (app == 2) {
						WHBGfxShutdown();
						shutdownScreen();
						initScreen();
					}
					
					app = 1;
					break;
				}
				case PROCUI_STATUS_IN_BACKGROUND: {
					app = 2;
					break;
				}
			}
			return app;
		}
	}
	else
		return WHBProcIsRunning();
	
	return 1;
}