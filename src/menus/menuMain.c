#include <stdio.h>
#include "main.h"
#include "common.h"
#include "utils.h"
#include "dir.h"
#include "avsound.h"
#include "watchdb.h"
#include "tent.h"

static int currentTab = TAB_FOLDERS;

/* Home "Continue Watching" card */
#define HOME_CARD_X (FRAMEBUF_WIDTH  * 0.15f)
#define HOME_CARD_Y (FRAMEBUF_HEIGHT * 0.38f)
#define HOME_CARD_W (FRAMEBUF_WIDTH  * 0.70f)
#define HOME_CARD_H (FRAMEBUF_HEIGHT * 0.28f)

/* Settings tappable rows */
#define SET_ROW_X  (FRAMEBUF_WIDTH  * 0.10f)
#define SET_ROW_W  (FRAMEBUF_WIDTH  * 0.80f)
#define SET_ROW_H  (FRAMEBUF_HEIGHT * 0.12f)
#define SET_SORT_Y (TOPBAR_H + FRAMEBUF_HEIGHT * 0.10f)
#define SET_DIM_Y  (TOPBAR_H + FRAMEBUF_HEIGHT * 0.26f)

#define TAP_MAX_MOVE (FRAMEBUF_HEIGHT * 0.12f)   /* beyond this a touch is a swipe, not a tap */

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

static void drawTopBar(void)
{
    vita2d_draw_rectangle(0, 0, FRAMEBUF_WIDTH, TOPBAR_H, RGBA8(25, 25, 30, 255));
    const char *labels[3] = { "Home", "Folders", "Settings" };
    float tw = FRAMEBUF_WIDTH / 3.0f;
    for (int t = 0; t < 3; t++) {
        float tx = tw * t;
        if (t == currentTab)
            vita2d_draw_rectangle(tx, 0, tw, TOPBAR_H, RGBA8(120, 120, 255, 220));
        int w = vita2d_pgf_text_width(pgf, 1.0f, labels[t]);
        vita2d_pgf_draw_text(pgf, tx + (tw - w) / 2.0f, TOPBAR_H * 0.66f, RGBA8(255, 255, 255, 255), 1.0f, labels[t]);
    }
    vita2d_draw_line(0, TOPBAR_H, FRAMEBUF_WIDTH, TOPBAR_H, RGBA8(255, 255, 255, 255));
}

static void drawHome(void)
{
    vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.08f, TOPBAR_H + FRAMEBUF_HEIGHT*0.14f,
        RGBA8(230, 230, 230, 255), 1.1f, "Continue Watching");
    if (continueLabel[0]) {
        vita2d_draw_rectangle(HOME_CARD_X, HOME_CARD_Y, HOME_CARD_W, HOME_CARD_H, RGBA8(120, 120, 255, 180));
        vita2d_pgf_draw_text(pgf, HOME_CARD_X + FRAMEBUF_WIDTH*0.03f, HOME_CARD_Y + HOME_CARD_H*0.45f,
            RGBA8(255, 255, 255, 255), 1.0f, continueLabel);
        vita2d_pgf_draw_text(pgf, HOME_CARD_X + FRAMEBUF_WIDTH*0.03f, HOME_CARD_Y + HOME_CARD_H*0.78f,
            RGBA8(235, 235, 235, 255), 0.9f, "tap to play");
    } else {
        vita2d_pgf_draw_text(pgf, HOME_CARD_X, HOME_CARD_Y + HOME_CARD_H*0.5f,
            RGBA8(170, 170, 170, 255), 1.0f, "Nothing in progress - open the Folders tab");
    }
}

static void drawSettings(void)
{
    char line[80];
    vita2d_draw_rectangle(SET_ROW_X, SET_SORT_Y, SET_ROW_W, SET_ROW_H, RGBA8(55, 55, 60, 255));
    snprintf(line, sizeof(line), "Sort order:  %s", sortOrderName());
    vita2d_pgf_draw_text(pgf, SET_ROW_X + FRAMEBUF_WIDTH*0.02f, SET_SORT_Y + SET_ROW_H*0.62f, RGBA8(255, 255, 255, 255), 1.0f, line);

    vita2d_draw_rectangle(SET_ROW_X, SET_DIM_Y, SET_ROW_W, SET_ROW_H, RGBA8(55, 55, 60, 255));
    snprintf(line, sizeof(line), "Brightness:  %d%%", tentBrightnessPercent());
    vita2d_pgf_draw_text(pgf, SET_ROW_X + FRAMEBUF_WIDTH*0.02f, SET_DIM_Y + SET_ROW_H*0.62f, RGBA8(255, 255, 255, 255), 1.0f, line);

    vita2d_pgf_draw_text(pgf, SET_ROW_X, SET_DIM_Y + FRAMEBUF_HEIGHT*0.24f,
        RGBA8(160, 160, 160, 255), 0.9f, "Vita Media Player DEV  -  tap a row to change  -  L/R switch tabs");
}

static int inRow(int x, int y, float ry) {
    return (x > SET_ROW_X && x < SET_ROW_X + SET_ROW_W && y > ry && y < ry + SET_ROW_H);
}

/* Top-bar tab switching: tap a tab, or use the L/R triggers. */
static void handleTabBar(void)
{
    static int twas = 0, downX = 0, downY = 0, lastY = 0;
    if (touchActive) {
        if (!twas) { downX = touchX; downY = touchY; }
        lastY = touchY;
    } else if (twas) {
        int dy = lastY - downY; if (dy < 0) dy = -dy;
        if (dy < (int)TAP_MAX_MOVE && downY < (int)TOPBAR_H) {
            int t = (int)(downX * 3 / FRAMEBUF_WIDTH);
            if (t < 0) t = 0;
            if (t > 2) t = 2;
            currentTab = t;
        }
    }
    twas = touchActive;

    if (pressed & SCE_CTRL_LTRIGGER) currentTab = (currentTab + 2) % 3;
    if (pressed & SCE_CTRL_RTRIGGER) currentTab = (currentTab + 1) % 3;
}

static void handleHome(void)
{
    static int twas = 0, downX = 0, downY = 0, lastY = 0;
    if (touchActive) {
        if (!twas) { downX = touchX; downY = touchY; }
        lastY = touchY;
    } else if (twas) {
        int dy = lastY - downY; if (dy < 0) dy = -dy;
        if (dy < (int)TAP_MAX_MOVE && downY >= (int)TOPBAR_H && continueLabel[0] &&
            downX > HOME_CARD_X && downX < HOME_CARD_X + HOME_CARD_W &&
            downY > HOME_CARD_Y && downY < HOME_CARD_Y + HOME_CARD_H) {
            playContinue();
        }
    }
    twas = touchActive;

    if (pressed & SCE_CTRL_CROSS)
        playContinue();
}

static void handleSettings(void)
{
    static int twas = 0, downX = 0, downY = 0, lastY = 0;
    if (touchActive) {
        if (!twas) { downX = touchX; downY = touchY; }
        lastY = touchY;
    } else if (twas) {
        int dy = lastY - downY; if (dy < 0) dy = -dy;
        if (dy < (int)TAP_MAX_MOVE && downY >= (int)TOPBAR_H) {
            if (inRow(downX, downY, SET_SORT_Y)) cycleSortOrder();
            else if (inRow(downX, downY, SET_DIM_Y)) tentCycleBrightness();
        }
    }
    twas = touchActive;
}

int drawMainMenu()
{
    vita2d_set_vblank_wait(1);
    while(1) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        vita2d_draw_rectangle(0, 0, FRAMEBUF_WIDTH, FRAMEBUF_HEIGHT, RGBA8(41, 41, 41, 255));

        if (currentTab == TAB_FOLDERS) {
            displayFiles();
            drawContinueBanner();
        } else if (currentTab == TAB_HOME) {
            drawHome();
        } else {
            drawSettings();
        }

        drawTopBar();

        char batt[24];
        snprintf(batt, sizeof(batt), "Batt %d%%", tentBatteryPercent());
        vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.85f, FRAMEBUF_HEIGHT*0.975f, RGBA8(200, 200, 200, 255), 0.85f, batt);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();

        readControls();
        handleTabBar();
        if (currentTab == TAB_FOLDERS)
            handleDirControls();
        else if (currentTab == TAB_HOME)
            handleHome();
        else
            handleSettings();

        if (pressed == (SCE_CTRL_START | SCE_CTRL_SELECT))
            break;
    }
    return 0;
}
