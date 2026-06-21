#ifndef OVERLAY_H
#define OVERLAY_H

/* On-screen HUD touch buttons (so physical buttons aren't needed). */
enum HudHit {
	HUD_HIT_NONE = 0,
	HUD_HIT_PLAYPAUSE,
	HUD_HIT_VOLDOWN,
	HUD_HIT_VOLUP,
	HUD_HIT_NEXT,
	HUD_HIT_SLEEP,
	HUD_HIT_DIM
};

int drawOverlay(uint64_t currentTime);
int overlayHitTest(int x, int y);   /* which HUD button is at screen (x,y) */

#endif