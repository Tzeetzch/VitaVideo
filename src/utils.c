#include "common.h"
#include "utils.h"
#include <string.h>
#include <ctype.h>
#include <appmgr.h>
#include <kernel/processmgr.h>
#include <system_param.h>
#include <psp2/touch.h>

/* Natural-order compare: runs of digits are compared by numeric value so that
 * "Ep2" < "Ep10" < "Ep20" instead of lexical "Ep1, Ep10, Ep2". Otherwise
 * case-insensitive. Fixes episode ordering in the file browser. */
int strcmpnat(const char *a, const char *b) {
	while (*a && *b) {
		if (isdigit((unsigned char)*a) && isdigit((unsigned char)*b)) {
			const char *ea, *eb;
			int la, lb, c;
			while (*a == '0') a++;   /* skip leading zeros */
			while (*b == '0') b++;
			ea = a; while (isdigit((unsigned char)*ea)) ea++;
			eb = b; while (isdigit((unsigned char)*eb)) eb++;
			la = (int)(ea - a); lb = (int)(eb - b);
			if (la != lb) return la - lb;       /* more digits => larger number */
			c = strncmp(a, b, la);              /* equal length => lexical is fine */
			if (c) return c;
			a = ea; b = eb;
		} else {
			int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
			if (ca != cb) return ca - cb;
			a++; b++;
		}
	}
	return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static SceCtrlData pad, old_pad;

int readControls(void) {
	memset(&pad, 0, sizeof(SceCtrlData));
	sceCtrlPeekBufferPositive(0, &pad, 1);

	pressed = pad.buttons & ~old_pad.buttons;
	stickLX = pad.lx;
	stickLY = pad.ly;

	/* Front touch panel reports at 2x the screen resolution (1920x1088). */
	SceTouchData td;
	memset(&td, 0, sizeof(td));
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &td, 1);
	if (td.reportNum > 0) {
		touchActive = 1;
		touchX = td.report[0].x / 2;
		touchY = td.report[0].y / 2;
	} else {
		touchActive = 0;
	}

	old_pad = pad;
	return 0;
}

void setMax(int *set, int value, int max) {
	if (*set > max)
		*set = value;
}

void setMin(int *set, int value, int min) {
	if (*set < min)
		*set = value;
}

int initAppUtil() {
	SceAppUtilInitParam init;
	SceAppUtilBootParam boot;
	memset(&init, 0, sizeof(SceAppUtilInitParam));
	memset(&boot, 0, sizeof(SceAppUtilBootParam));
	
	int ret = 0;
	
	if (R_FAILED(ret = sceAppUtilInit(&init, &boot)))
		return ret;

	if (R_FAILED(ret = sceAppUtilMusicMount()))
		return ret;
	
	return 0;
}

int termAppUtil() {
	int ret = 0;

	if (R_FAILED(ret = sceAppUtilMusicUmount()))
	
	if (R_FAILED(ret = sceAppUtilShutdown()))
		return ret;
	
	return 0;
}

int getEnterButton() {
	int button = 0;
	sceAppUtilSystemParamGetInt(2, &button);
	
	if (button == 0)
		return SCE_CTRL_CIRCLE;
	else
		return SCE_CTRL_CROSS;

	return 0;
}

int getCancelButton() {
	int button = 0;
	sceAppUtilSystemParamGetInt(2, &button);
	
	if (button == 0)
		return SCE_CTRL_CROSS;
	else
		return SCE_CTRL_CIRCLE;

	return 0;
}