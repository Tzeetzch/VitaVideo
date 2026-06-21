#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "dir.h"
#include "tent.h"

#define CONFIG_PATH "ux0:data/SubPlayer/settings.txt"

/* File format: one line, three ints "sortOrder brightnessIdx sleepDefaultMin". */

void configLoad(void) {
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0);
	if (fd < 0)
		return;
	char buf[128];
	int n = sceIoRead(fd, buf, sizeof(buf) - 1);
	sceIoClose(fd);
	if (n <= 0)
		return;
	buf[n] = '\0';

	int sort = 0, bright = 0, sleepDef = 0;
	if (sscanf(buf, "%d %d %d", &sort, &bright, &sleepDef) >= 1) {
		setSortOrder(sort);
		tentSetBrightnessIdx(bright);
		tentSetSleepDefault(sleepDef);
	}
}

void configSave(void) {
	SceUID fd = sceIoOpen(CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return;
	char buf[128];
	int n = snprintf(buf, sizeof(buf), "%d %d %d\n",
		getSortOrder(), tentGetBrightnessIdx(), tentGetSleepDefault());
	if (n > 0)
		sceIoWrite(fd, buf, n);
	sceIoClose(fd);
}
