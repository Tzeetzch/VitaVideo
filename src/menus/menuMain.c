#include <stdio.h>
#include "main.h"
#include "common.h"
#include "utils.h"
#include "dir.h"
#include "avsound.h"
#include "watchdb.h"
#include "tent.h"

int initMainMenu()
{
    pgf = vita2d_load_default_pgf();
    subtitleFont = vita2d_load_custom_pvf("app0:OpenSans-Bold.ttf");
    watchdbLoad();
    getLastDirectory();
    getDirListing(SCE_FALSE);
    updateContinueTarget();
    avSoundInit();
    return 0;
}

int drawMainMenu()
{
    vita2d_set_vblank_wait(1);
    while(1) {
        vita2d_start_drawing();
	    vita2d_clear_screen();
        vita2d_draw_rectangle(0, 0, FRAMEBUF_WIDTH, FRAMEBUF_HEIGHT, RGBA8(41, 41, 41, 255));
        displayFiles();
        char batt[24];
        snprintf(batt, sizeof(batt), "Batt %d%%", tentBatteryPercent());
        vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.82f, FRAMEBUF_HEIGHT*0.06f, RGBA8(200, 200, 200, 255), 0.9f, batt);
        drawContinueBanner();
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
		vita2d_swap_buffers();
        readControls();
        handleDirControls();

        if (pressed == (SCE_CTRL_START | SCE_CTRL_SELECT))
			break;
    }
    return 0;
}