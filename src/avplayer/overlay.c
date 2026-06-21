#include <stdio.h>
#include <math.h>
#include "common.h"
#include "overlay.h"
#include "texture.h"
#include "tent.h"

/* Bottom HUD icon row — evenly spaced so nothing overlaps. The play/pause and
 * subtitle icons keep their original look; Next reuses the "forwards" texture;
 * the sleep (clock) and dim (sun) icons are drawn with primitives. */
#define HUD_ICON_Y    (FRAMEBUF_HEIGHT * 0.90f)
#define HUD_NEXT_X    (FRAMEBUF_WIDTH  * 0.10f)
#define HUD_SLEEP_X   (FRAMEBUF_WIDTH  * 0.30f)
#define HUD_PLAY_X    (FRAMEBUF_WIDTH  * 0.50f)
#define HUD_DIM_X     (FRAMEBUF_WIDTH  * 0.70f)
#define HUD_SUBS_X    (FRAMEBUF_WIDTH  * 0.90f)
#define HUD_ICON_HALF (FRAMEBUF_WIDTH  * 0.07f)

static int inIcon(int x, int y, float cx) {
	return (x > cx - HUD_ICON_HALF && x < cx + HUD_ICON_HALF &&
	        y > FRAMEBUF_HEIGHT * 0.84f && y < FRAMEBUF_HEIGHT * 0.99f);
}

int overlayHitTest(int x, int y) {
	if (inIcon(x, y, HUD_NEXT_X))  return HUD_HIT_NEXT;
	if (inIcon(x, y, HUD_SLEEP_X)) return HUD_HIT_SLEEP;
	if (inIcon(x, y, HUD_PLAY_X))  return HUD_HIT_PLAYPAUSE;
	if (inIcon(x, y, HUD_DIM_X))   return HUD_HIT_DIM;
	if (inIcon(x, y, HUD_SUBS_X))  return HUD_HIT_SUBS;
	return HUD_HIT_NONE;
}

/* sun icon for the brightness/dim control */
static void drawSun(float cx, float cy) {
	float r = FRAMEBUF_HEIGHT * 0.020f;
	unsigned int c = RGBA8(255, 205, 70, 255);
	vita2d_draw_fill_circle(cx, cy, r, c);
	for (int i = 0; i < 8; i++) {
		float a = i * (3.14159265f / 4.0f);
		vita2d_draw_line(cx + cosf(a) * r * 1.5f, cy + sinf(a) * r * 1.5f,
		                 cx + cosf(a) * r * 2.2f, cy + sinf(a) * r * 2.2f, c);
	}
}

/* clock icon for the sleep timer (rim glows orange when armed) */
static void drawClock(float cx, float cy, int armed) {
	float r = FRAMEBUF_HEIGHT * 0.024f;
	unsigned int rim = armed ? RGBA8(255, 180, 80, 255) : RGBA8(210, 210, 210, 255);
	vita2d_draw_fill_circle(cx, cy, r, rim);
	vita2d_draw_fill_circle(cx, cy, r * 0.72f, RGBA8(30, 30, 30, 255));
	vita2d_draw_line(cx, cy, cx, cy - r * 0.55f, rim);
	vita2d_draw_line(cx, cy, cx + r * 0.40f, cy + r * 0.05f, rim);
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
	float scale = (FRAMEBUF_WIDTH * .05f) / 200.0f;

	vita2d_draw_texture_scale_rotate(forwards, HUD_NEXT_X, HUD_ICON_Y, scale, scale, 0.0f);
	drawClock(HUD_SLEEP_X, HUD_ICON_Y, tentSleepMinutes() > 0);

	switch (playerState) {
		case PLAYER_PLAYING:
			vita2d_draw_texture_scale_rotate(play, HUD_PLAY_X, HUD_ICON_Y, scale, scale, 0.0f);
			break;
		case PLAYER_PAUSED:
			vita2d_draw_texture_scale_rotate(pause, HUD_PLAY_X, HUD_ICON_Y, scale, scale, 0.0f);
			break;
		case PLAYER_STOPPED:
			vita2d_draw_texture_scale_rotate(stop, HUD_PLAY_X, HUD_ICON_Y, scale, scale, 0.0f);
			break;
		case PLAYER_BUFFERING:
			vita2d_draw_texture_scale_rotate(loading, HUD_PLAY_X, HUD_ICON_Y, scale, scale, 0.0f);
			break;
		default:
			vita2d_draw_texture_scale_rotate(play, HUD_PLAY_X, HUD_ICON_Y, scale, scale, 0.0f);
			break;
	}

	drawSun(HUD_DIM_X, HUD_ICON_Y);

	switch (subStatus) {
		case SUBTITLES_DISABLED:
			vita2d_draw_texture_scale_rotate(subtitles_disabled, HUD_SUBS_X, HUD_ICON_Y, scale, scale, 0.0f);
			break;
		case SUBTITLES_ENABLED:
			vita2d_draw_texture_scale_rotate(subtitles, HUD_SUBS_X, HUD_ICON_Y, scale, scale, 0.0f);
			break;
		case SUBTITLES_NONE:
			vita2d_draw_texture_tint_scale_rotate(subtitles_disabled, HUD_SUBS_X, HUD_ICON_Y, scale, scale, 0.0f, RGBA8(140, 140, 140, 255));
			break;
	}
	return 0;
}

int drawOverlay(uint64_t currentTime)
{
	float percentageDone = streamDuration ? currentTime / (float)streamDuration : 0.0f;
	vita2d_draw_rectangle(0, 0, FRAMEBUF_WIDTH, FRAMEBUF_HEIGHT, RGBA8(0, 0, 0, 120));
	vita2d_draw_rectangle(FRAMEBUF_WIDTH*.12f, FRAMEBUF_HEIGHT*.8f, FRAMEBUF_WIDTH*.76f, FRAMEBUF_HEIGHT*.027f, RGBA8(255, 255, 255, 255));
	vita2d_draw_rectangle(FRAMEBUF_WIDTH*.12f, FRAMEBUF_HEIGHT*.8f, (FRAMEBUF_WIDTH*.76f)*percentageDone, FRAMEBUF_HEIGHT*.027f, RGBA8(120, 120, 255, 200));

	/* elapsed / total time, just above the bar */
	char elapsed[16], total[16], timeLine[40];
	fmtTime(currentTime, elapsed, sizeof(elapsed));
	fmtTime(streamDuration, total, sizeof(total));
	snprintf(timeLine, sizeof(timeLine), "%s / %s", elapsed, total);
	vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.12f, FRAMEBUF_HEIGHT*0.78f, RGBA8(255, 255, 255, 255), 1.0f, timeLine);

	/* battery (top-right) + sleep countdown (top-left, when armed) */
	char batt[24];
	snprintf(batt, sizeof(batt), "Batt %d%%", tentBatteryPercent());
	vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.80f, FRAMEBUF_HEIGHT*0.1f, RGBA8(220, 220, 220, 255), 1.0f, batt);
	int sleepRem = tentSleepRemainingSec();
	if (sleepRem >= 0) {
		char sl[24];
		snprintf(sl, sizeof(sl), "Sleep %dm", (sleepRem + 59) / 60);
		vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.04f, FRAMEBUF_HEIGHT*0.1f, RGBA8(255, 200, 120, 255), 1.0f, sl);
	}

	drawStatus();
	return 0;
}
