#include "main.h"
#include "common.h"
#include "utils.h"
#include "dir.h"
#include "avsound.h"
#include "watchdb.h"

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
        if (continueLabel[0]) {
            vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.62f, FRAMEBUF_HEIGHT*0.88f, RGBA8(180, 180, 180, 255), 0.9f, "Triangle:");
            vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.62f, FRAMEBUF_HEIGHT*0.93f, RGBA8(120, 200, 255, 255), 0.9f, continueLabel);
        }
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