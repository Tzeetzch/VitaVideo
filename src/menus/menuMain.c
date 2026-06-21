#include <stdio.h>
#include "main.h"
#include "common.h"
#include "utils.h"
#include "dir.h"
#include "avsound.h"
#include "watchdb.h"
#include "tent.h"
#include "config.h"

static int currentTab = TAB_FOLDERS;

/* Home: continue card on top, Favorites + Recently-watched columns below */
#define HOME_CARD_X (FRAMEBUF_WIDTH  * 0.05f)
#define HOME_CARD_Y (TOPBAR_H + FRAMEBUF_HEIGHT * 0.06f)
#define HOME_CARD_W (FRAMEBUF_WIDTH  * 0.90f)
#define HOME_CARD_H (FRAMEBUF_HEIGHT * 0.16f)
#define HOME_LIST_Y (TOPBAR_H + FRAMEBUF_HEIGHT * 0.32f)
#define HOME_ROW_H  (FRAMEBUF_HEIGHT * 0.10f)
#define HOME_ROWS   5
#define FAV_COL_X   (FRAMEBUF_WIDTH  * 0.05f)
#define FAV_COL_W   (FRAMEBUF_WIDTH  * 0.43f)
#define REC_COL_X   (FRAMEBUF_WIDTH  * 0.52f)
#define REC_COL_W   (FRAMEBUF_WIDTH  * 0.43f)

/* Last path component (handles a trailing '/' on folder paths). */
static void folderName(const char *p, char *out, int sz) {
    int len = strlen(p);
    if (len > 0 && p[len-1] == '/') len--;
    int start = len;
    while (start > 0 && p[start-1] != '/') start--;
    int n = len - start;
    if (n >= sz) n = sz - 1;
    if (n < 0) n = 0;
    memcpy(out, p + start, n);
    out[n] = '\0';
}

/* Settings tappable rows */
#define SET_ROW_X  (FRAMEBUF_WIDTH  * 0.10f)
#define SET_ROW_W  (FRAMEBUF_WIDTH  * 0.80f)
#define SET_ROW_H  (FRAMEBUF_HEIGHT * 0.12f)
#define SET_SORT_Y  (TOPBAR_H + FRAMEBUF_HEIGHT * 0.08f)
#define SET_DIM_Y   (TOPBAR_H + FRAMEBUF_HEIGHT * 0.23f)
#define SET_SLEEP_Y (TOPBAR_H + FRAMEBUF_HEIGHT * 0.38f)

#define TAP_MAX_MOVE (FRAMEBUF_HEIGHT * 0.12f)   /* beyond this a touch is a swipe, not a tap */

int initMainMenu()
{
    pgf = vita2d_load_default_pgf();
    subtitleFont = vita2d_load_custom_pvf("app0:OpenSans-Bold.ttf");
    watchdbLoad();
    configLoad();                 /* restore sort / brightness / default-sleep */
    favLoad();
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
    char nm[256];

    /* Continue Watching card */
    vita2d_pgf_draw_text(pgf, HOME_CARD_X, TOPBAR_H + FRAMEBUF_HEIGHT*0.045f,
        RGBA8(230, 230, 230, 255), 1.0f, "Continue Watching");
    if (continueLabel[0]) {
        vita2d_draw_rectangle(HOME_CARD_X, HOME_CARD_Y, HOME_CARD_W, HOME_CARD_H, RGBA8(120, 120, 255, 180));
        vita2d_pgf_draw_text(pgf, HOME_CARD_X + FRAMEBUF_WIDTH*0.02f, HOME_CARD_Y + HOME_CARD_H*0.45f,
            RGBA8(255, 255, 255, 255), 1.0f, continueLabel);
        vita2d_pgf_draw_text(pgf, HOME_CARD_X + FRAMEBUF_WIDTH*0.02f, HOME_CARD_Y + HOME_CARD_H*0.80f,
            RGBA8(235, 235, 235, 255), 0.85f, "tap to play");
    } else {
        vita2d_pgf_draw_text(pgf, HOME_CARD_X, HOME_CARD_Y + HOME_CARD_H*0.5f,
            RGBA8(170, 170, 170, 255), 0.95f, "Nothing in progress yet");
    }

    /* Favorites column */
    vita2d_pgf_draw_text(pgf, FAV_COL_X, HOME_LIST_Y - FRAMEBUF_HEIGHT*0.02f, RGBA8(230, 230, 230, 255), 0.95f, "Favorites");
    int fc = favCountGet(); if (fc > HOME_ROWS) fc = HOME_ROWS;
    for (int i = 0; i < fc; i++) {
        float ry = HOME_LIST_Y + i * HOME_ROW_H;
        vita2d_draw_rectangle(FAV_COL_X, ry, FAV_COL_W, HOME_ROW_H * 0.86f, RGBA8(55, 55, 60, 255));
        folderName(favGet(i), nm, sizeof(nm));
        vita2d_pgf_draw_text(pgf, FAV_COL_X + FRAMEBUF_WIDTH*0.015f, ry + HOME_ROW_H*0.55f, RGBA8(255, 255, 255, 255), 0.85f, nm);
    }
    if (favCountGet() == 0)
        vita2d_pgf_draw_text(pgf, FAV_COL_X, HOME_LIST_Y + HOME_ROW_H*0.6f, RGBA8(150, 150, 150, 255), 0.8f, "(star a folder in Folders)");

    /* Recently watched column */
    vita2d_pgf_draw_text(pgf, REC_COL_X, HOME_LIST_Y - FRAMEBUF_HEIGHT*0.02f, RGBA8(230, 230, 230, 255), 0.95f, "Recently watched");
    int rc = watchdbRecentCount(); if (rc > HOME_ROWS) rc = HOME_ROWS;
    for (int i = 0; i < rc; i++) {
        float ry = HOME_LIST_Y + i * HOME_ROW_H;
        vita2d_draw_rectangle(REC_COL_X, ry, REC_COL_W, HOME_ROW_H * 0.86f, RGBA8(55, 55, 60, 255));
        folderName(watchdbRecentGet(i), nm, sizeof(nm));
        vita2d_pgf_draw_text(pgf, REC_COL_X + FRAMEBUF_WIDTH*0.015f, ry + HOME_ROW_H*0.55f, RGBA8(255, 255, 255, 255), 0.85f, nm);
    }
    if (watchdbRecentCount() == 0)
        vita2d_pgf_draw_text(pgf, REC_COL_X, HOME_LIST_Y + HOME_ROW_H*0.6f, RGBA8(150, 150, 150, 255), 0.8f, "(nothing yet)");
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

    vita2d_draw_rectangle(SET_ROW_X, SET_SLEEP_Y, SET_ROW_W, SET_ROW_H, RGBA8(55, 55, 60, 255));
    if (tentGetSleepDefault() > 0)
        snprintf(line, sizeof(line), "Default sleep:  %d min", tentGetSleepDefault());
    else
        snprintf(line, sizeof(line), "Default sleep:  Off");
    vita2d_pgf_draw_text(pgf, SET_ROW_X + FRAMEBUF_WIDTH*0.02f, SET_SLEEP_Y + SET_ROW_H*0.62f, RGBA8(255, 255, 255, 255), 1.0f, line);

    vita2d_pgf_draw_text(pgf, SET_ROW_X, SET_SLEEP_Y + FRAMEBUF_HEIGHT*0.22f,
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
        if (dy < (int)TAP_MAX_MOVE && downY >= (int)TOPBAR_H) {
            if (continueLabel[0] &&
                downX > HOME_CARD_X && downX < HOME_CARD_X + HOME_CARD_W &&
                downY > HOME_CARD_Y && downY < HOME_CARD_Y + HOME_CARD_H) {
                playContinue();
            } else if (downX > FAV_COL_X && downX < FAV_COL_X + FAV_COL_W && downY >= HOME_LIST_Y) {
                int i = (int)((downY - HOME_LIST_Y) / HOME_ROW_H);
                if (i >= 0 && i < HOME_ROWS && i < favCountGet()) {
                    openFolder(favGet(i));      /* jump to the favourite folder */
                    currentTab = TAB_FOLDERS;
                }
            } else if (downX > REC_COL_X && downX < REC_COL_X + REC_COL_W && downY >= HOME_LIST_Y) {
                int i = (int)((downY - HOME_LIST_Y) / HOME_ROW_H);
                if (i >= 0 && i < HOME_ROWS && i < watchdbRecentCount())
                    playPath(watchdbRecentGet(i));   /* replay a recent video */
            }
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
            if (inRow(downX, downY, SET_SORT_Y)) { cycleSortOrder(); configSave(); }
            else if (inRow(downX, downY, SET_DIM_Y)) { tentCycleBrightness(); configSave(); }
            else if (inRow(downX, downY, SET_SLEEP_Y)) { tentCycleSleepDefault(); configSave(); }
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
            drawItemMenu();
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
