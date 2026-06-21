#include <stdio.h>
#include "common.h"
#include "overlay.h"
#include "texture.h"
#include "avsound.h"
#include "tent.h"

/* On-screen HUD button geometry (framebuffer coords). */
#define HUD_BTN_W   (FRAMEBUF_WIDTH * 0.105f)
#define HUD_BTN_H   (FRAMEBUF_HEIGHT * 0.10f)
#define HUD_BTN_Y   (FRAMEBUF_HEIGHT * 0.87f)
#define HUD_VOLDN_X (FRAMEBUF_WIDTH * 0.02f)
#define HUD_VOLUP_X (FRAMEBUF_WIDTH * 0.13f)
#define HUD_SLEEP_X (FRAMEBUF_WIDTH * 0.24f)
#define HUD_DIM_X   (FRAMEBUF_WIDTH * 0.35f)
#define HUD_NEXT_X  (FRAMEBUF_WIDTH * 0.86f)

static int inRect(int x, int y, float bx, float by, float bw, float bh) {
	return (x >= bx && x <= bx + bw && y >= by && y <= by + bh);
}

static void drawButton(float x, const char *label) {
	vita2d_draw_rectangle(x, HUD_BTN_Y, HUD_BTN_W, HUD_BTN_H, RGBA8(50, 50, 50, 210));
	vita2d_pgf_draw_text(pgf, x + HUD_BTN_W * 0.14f, HUD_BTN_Y + HUD_BTN_H * 0.62f,
		RGBA8(255, 255, 255, 255), 1.0f, label);
}

int overlayHitTest(int x, int y) {
	if (inRect(x, y, HUD_VOLDN_X, HUD_BTN_Y, HUD_BTN_W, HUD_BTN_H)) return HUD_HIT_VOLDOWN;
	if (inRect(x, y, HUD_VOLUP_X, HUD_BTN_Y, HUD_BTN_W, HUD_BTN_H)) return HUD_HIT_VOLUP;
	if (inRect(x, y, HUD_SLEEP_X, HUD_BTN_Y, HUD_BTN_W, HUD_BTN_H)) return HUD_HIT_SLEEP;
	if (inRect(x, y, HUD_DIM_X,   HUD_BTN_Y, HUD_BTN_W, HUD_BTN_H)) return HUD_HIT_DIM;
	if (inRect(x, y, HUD_NEXT_X,  HUD_BTN_Y, HUD_BTN_W, HUD_BTN_H)) return HUD_HIT_NEXT;
	if (x > FRAMEBUF_WIDTH * 0.47f && x < FRAMEBUF_WIDTH * 0.55f &&
	    y > FRAMEBUF_HEIGHT * 0.84f && y < FRAMEBUF_HEIGHT * 0.99f) return HUD_HIT_PLAYPAUSE;
	return HUD_HIT_NONE;
}

/* milliseconds -> "M:SS" or "H:MM:SS" */
static void fmtTime(uint64_t ms, char *out, int sz) {
	unsigned int total = (unsigned int)(ms / 1000);
	unsigned int h = total / 3600, m = (total / 60) % 60, s = total % 60;
	if (h)
		snprintf(out, sz, "%u:%02u:%02u", h, m, s);
	else
		snprintf(out, sz, "%u:%02u", m, s);
}

static int drawStatus()
{
    float scale = (FRAMEBUF_WIDTH*.05f)/200.0f;
    switch (playerState) {
        case PLAYER_PLAYING:
            vita2d_draw_texture_scale_rotate(play, FRAMEBUF_WIDTH/2, FRAMEBUF_HEIGHT*.9f, scale, scale, 0.0f);
            break;
        case PLAYER_PAUSED:
            vita2d_draw_texture_scale_rotate(pause, FRAMEBUF_WIDTH/2, FRAMEBUF_HEIGHT*.9f, scale, scale, 0.0f);
            break;
        case PLAYER_STOPPED:
            vita2d_draw_texture_scale_rotate(stop, FRAMEBUF_WIDTH/2, FRAMEBUF_HEIGHT*.9f, scale, scale, 0.0f);
            break;
        case PLAYER_BUFFERING:
            vita2d_draw_texture_scale_rotate(loading, FRAMEBUF_WIDTH/2, FRAMEBUF_HEIGHT*.9f, scale, scale, 0.0f);
            break;
        default:
            break;
    }
    switch (subStatus) {
        case SUBTITLES_DISABLED:
            vita2d_draw_texture_scale_rotate(subtitles_disabled, FRAMEBUF_WIDTH*.9f, FRAMEBUF_HEIGHT*.9f, scale, scale, 0.0f);
            break;
        case SUBTITLES_ENABLED:
            vita2d_draw_texture_scale_rotate(subtitles, FRAMEBUF_WIDTH*.9f, FRAMEBUF_HEIGHT*.9f, scale, scale, 0.0f);
            break;
        case SUBTITLES_NONE:
            vita2d_draw_texture_tint_scale_rotate(subtitles_disabled, FRAMEBUF_WIDTH*.9f, FRAMEBUF_HEIGHT*.9f, scale, scale, 0.0f, RGBA8(140,140,140,255));
            break;
    }
    return 0;
}

int drawOverlay(uint64_t currentTime)
{
    float percentageDone = streamDuration ? currentTime/(float)streamDuration : 0.0f;
    vita2d_draw_rectangle(0,0,FRAMEBUF_WIDTH,FRAMEBUF_HEIGHT,RGBA8(0,0,0,120));
    vita2d_draw_rectangle(FRAMEBUF_WIDTH*.12f,FRAMEBUF_HEIGHT*.8f,FRAMEBUF_WIDTH*.76f, FRAMEBUF_HEIGHT*.027f, RGBA8(255,255,255,255));
    vita2d_draw_rectangle(FRAMEBUF_WIDTH*.12f,FRAMEBUF_HEIGHT*.8f,(FRAMEBUF_WIDTH*.76f)*percentageDone,FRAMEBUF_HEIGHT*.027f, RGBA8(120, 120, 255, 200));

    /* elapsed / total time, just above the bar */
    char elapsed[16], total[16], timeLine[40];
    fmtTime(currentTime, elapsed, sizeof(elapsed));
    fmtTime(streamDuration, total, sizeof(total));
    snprintf(timeLine, sizeof(timeLine), "%s / %s", elapsed, total);
    vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.12f, FRAMEBUF_HEIGHT*0.78f, RGBA8(255, 255, 255, 255), 1.0f, timeLine);

    /* top row: volume + battery, and the sleep countdown when armed */
    char info[48];
    snprintf(info, sizeof(info), "Vol %d%%   Batt %d%%", avSoundGetVolume(), tentBatteryPercent());
    vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.58f, FRAMEBUF_HEIGHT*0.1f, RGBA8(220, 220, 220, 255), 1.0f, info);
    int sleepRem = tentSleepRemainingSec();
    if (sleepRem >= 0) {
        char sl[24];
        snprintf(sl, sizeof(sl), "Sleep %dm", (sleepRem + 59) / 60);
        vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.04f, FRAMEBUF_HEIGHT*0.1f, RGBA8(255, 200, 120, 255), 1.0f, sl);
    }

    /* On-screen touch buttons (labels reflect current state) */
    char sleepBtn[16], dimBtn[16];
    if (tentSleepMinutes() > 0)
        snprintf(sleepBtn, sizeof(sleepBtn), "Sleep%d", tentSleepMinutes());
    else
        snprintf(sleepBtn, sizeof(sleepBtn), "Sleep");
    snprintf(dimBtn, sizeof(dimBtn), "Dim%d", tentBrightnessPercent());
    drawButton(HUD_VOLDN_X, "Vol-");
    drawButton(HUD_VOLUP_X, "Vol+");
    drawButton(HUD_SLEEP_X, sleepBtn);
    drawButton(HUD_DIM_X, dimBtn);
    drawButton(HUD_NEXT_X, "Next");
    drawStatus();
    return 0;
}