#include <stdio.h>
#include "common.h"
#include "overlay.h"
#include "texture.h"
#include "avsound.h"

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

    /* controls hints + volume */
    char volLine[24];
    snprintf(volLine, sizeof(volLine), "Vol %d%%", avSoundGetVolume());
    vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.04f, FRAMEBUF_HEIGHT*0.1f, RGBA8(220, 220, 220, 255), 1.0f, "R: Next episode");
    vita2d_pgf_draw_text(pgf, FRAMEBUF_WIDTH*0.78f, FRAMEBUF_HEIGHT*0.1f, RGBA8(220, 220, 220, 255), 1.0f, volLine);
    drawStatus();
    return 0;
}