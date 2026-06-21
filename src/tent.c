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

void tentCycleBrightness(void) {
	brightnessIdx = (brightnessIdx + 1) % (int)(sizeof(brightnessLevels) / sizeof(brightnessLevels[0]));
	int b = (maxBrightness * brightnessLevels[brightnessIdx]) / 100;
	if (b < 21) b = 21;   /* 0 would switch the panel off entirely */
	sceAVConfigSetDisplayBrightness(b);
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
