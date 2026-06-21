#include "common.h"
#include "tent.h"
#include <psp2/power.h>
#include <psp2/avconfig.h>

static int maxBrightness = 65536;
static const int brightnessLevels[] = { 100, 60, 30, 10 };
static int brightnessIdx = 0;

static const int sleepOptions[] = { 0, 15, 30, 60 };   /* minutes; 0 = off */
static int sleepIdx = 0;
static SceUInt64 sleepDeadlineUs = 0;                  /* 0 = off */
static int sleepDefaultMin = 0;                        /* auto-arm on playback start */

void tentInit(void) {
	if (sceAVConfigGetDisplayMaxBrightness(&maxBrightness) < 0 || maxBrightness <= 0)
		maxBrightness = 65536;
	brightnessIdx = 0;
	sleepIdx = 0;
	sleepDeadlineUs = 0;
}

void tentRestoreBrightness(void) {
	sceAVConfigSetDisplayBrightness(maxBrightness);
}

int tentBatteryPercent(void) {
	int p = scePowerGetBatteryLifePercent();
	if (p < 0) p = 0;
	if (p > 100) p = 100;
	return p;
}

int tentBrightnessPercent(void) {
	return brightnessLevels[brightnessIdx];
}

static void applyBrightness(void) {
	int b = (maxBrightness * brightnessLevels[brightnessIdx]) / 100;
	if (b < 21) b = 21;   /* 0 would switch the panel off entirely */
	sceAVConfigSetDisplayBrightness(b);
}

void tentCycleBrightness(void) {
	brightnessIdx = (brightnessIdx + 1) % (int)(sizeof(brightnessLevels) / sizeof(brightnessLevels[0]));
	applyBrightness();
}

int tentGetBrightnessIdx(void) { return brightnessIdx; }

void tentSetBrightnessIdx(int i) {
	int n = (int)(sizeof(brightnessLevels) / sizeof(brightnessLevels[0]));
	if (i < 0) i = 0;
	if (i >= n) i = n - 1;
	brightnessIdx = i;
	applyBrightness();
}

void tentCycleSleep(void) {
	sleepIdx = (sleepIdx + 1) % (int)(sizeof(sleepOptions) / sizeof(sleepOptions[0]));
	if (sleepOptions[sleepIdx] == 0)
		sleepDeadlineUs = 0;
	else
		sleepDeadlineUs = sceKernelGetProcessTimeWide() + (SceUInt64)sleepOptions[sleepIdx] * 60ULL * 1000000ULL;
}

void tentCancelSleep(void) {
	sleepIdx = 0;
	sleepDeadlineUs = 0;
}

int tentSleepMinutes(void) {
	return sleepOptions[sleepIdx];
}

int tentSleepRemainingSec(void) {
	if (sleepDeadlineUs == 0)
		return -1;
	SceUInt64 now = sceKernelGetProcessTimeWide();
	if (now >= sleepDeadlineUs)
		return 0;
	return (int)((sleepDeadlineUs - now) / 1000000ULL);
}

int tentSleepExpired(void) {
	if (sleepDeadlineUs == 0)
		return 0;
	return sceKernelGetProcessTimeWide() >= sleepDeadlineUs;
}

int tentSleepArmed(void) {
	return sleepDeadlineUs != 0;
}

/* Default sleep timer: armed automatically when playback starts. */
int tentGetSleepDefault(void) { return sleepDefaultMin; }
void tentSetSleepDefault(int m) { sleepDefaultMin = m; }

void tentCycleSleepDefault(void) {
	int n = (int)(sizeof(sleepOptions) / sizeof(sleepOptions[0]));
	int idx = 0;
	for (int i = 0; i < n; i++)
		if (sleepOptions[i] == sleepDefaultMin) { idx = i; break; }
	sleepDefaultMin = sleepOptions[(idx + 1) % n];
}

void tentArmSleepDefault(void) {
	if (sleepDefaultMin > 0)
		sleepDeadlineUs = sceKernelGetProcessTimeWide() + (SceUInt64)sleepDefaultMin * 60ULL * 1000000ULL;
}
