#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "utils.h"
#include "texture.h"
#include "main.h"
#include "avplayer.h"
#include "tent.h"
#include <psp2/touch.h>

unsigned int sceLibcHeapSize = 90*1024*1024;

int initVita2d()
{
    vita2d_init();
    printf("Init\n");
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));
    initMainMenu();
    loadTextures();
    return 0;
}

int main()
{
    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
    sceIoMkdir("ux0:data/SubPlayer", 0777);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);   /* enable analog sticks */
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    /* Standard open-source vita2d is statically linked and fixed at the
     * Vita's native 960x544 framebuffer, so the vita2d_sys module load and
     * the 1080p resolution setters are no longer needed. */
    tentInit();        /* before initVita2d -> initMainMenu -> configLoad restores brightness */
    initVita2d();
    initAppUtil();
	SCE_CTRL_ENTER = getEnterButton();
	SCE_CTRL_CANCEL = getCancelButton();
    avPlayerInit();
    drawMainMenu();
    tentRestoreBrightness();   /* don't leave the screen dimmed after exit */
    termAppUtil();
    vita2d_fini();
    freeTextures();

	sceKernelExitProcess(0);
	return 0;
}
