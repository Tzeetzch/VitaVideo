#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "watchdb.h"

#define WATCHDB_PATH "ux0:data/SubPlayer/watched.db"
#define RESUME_MIN_MS   5000   /* don't bother resuming the first few seconds */
#define WATCHED_PCT_NUM 9      /* >= 90% counts as finished */
#define WATCHED_PCT_DEN 10

typedef struct WatchEntry {
	char path[512];
	uint64_t position;
	uint64_t duration;
	int state;
	struct WatchEntry *next;
} WatchEntry;

static WatchEntry *dbHead = NULL;

static WatchEntry *findEntry(const char *path) {
	for (WatchEntry *e = dbHead; e != NULL; e = e->next)
		if (!strcmp(e->path, path))
			return e;
	return NULL;
}

void watchdbLoad(void) {
	/* Drop any existing in-memory list first. */
	WatchEntry *e = dbHead;
	while (e) { WatchEntry *n = e->next; free(e); e = n; }
	dbHead = NULL;

	SceUID fd = sceIoOpen(WATCHDB_PATH, SCE_O_RDONLY, 0);
	if (fd < 0)
		return;

	SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoLseek(fd, 0, SCE_SEEK_SET);
	if (size <= 0) { sceIoClose(fd); return; }

	char *buf = malloc(size + 1);
	if (!buf) { sceIoClose(fd); return; }
	int read = sceIoRead(fd, buf, size);
	sceIoClose(fd);
	if (read <= 0) { free(buf); return; }
	buf[read] = '\0';

	/* Each line: "<state> <position> <duration> <path>" */
	char *line = buf;
	while (line && *line) {
		char *nl = strchr(line, '\n');
		if (nl) *nl = '\0';

		char *p = line;
		int state = (int)strtol(p, &p, 10);
		uint64_t position = strtoull(p, &p, 10);
		uint64_t duration = strtoull(p, &p, 10);
		while (*p == ' ') p++;           /* p now at start of path */

		size_t len = strlen(p);
		while (len && (p[len-1] == '\r' || p[len-1] == ' ')) p[--len] = '\0';

		if (len > 0 && len < sizeof(((WatchEntry*)0)->path)) {
			WatchEntry *ne = malloc(sizeof(WatchEntry));
			if (ne) {
				strcpy(ne->path, p);
				ne->position = position;
				ne->duration = duration;
				ne->state = state;
				ne->next = dbHead;
				dbHead = ne;
			}
		}

		if (!nl) break;
		line = nl + 1;
	}
	free(buf);
}

void watchdbSave(void) {
	SceUID fd = sceIoOpen(WATCHDB_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return;

	char lineBuf[640];
	for (WatchEntry *e = dbHead; e != NULL; e = e->next) {
		int n = snprintf(lineBuf, sizeof(lineBuf), "%d %llu %llu %s\n",
			e->state, (unsigned long long)e->position,
			(unsigned long long)e->duration, e->path);
		if (n > 0)
			sceIoWrite(fd, lineBuf, n);
	}
	sceIoClose(fd);
}

uint64_t watchdbGetResume(const char *path) {
	WatchEntry *e = findEntry(path);
	if (!e || e->state == WATCH_WATCHED)
		return 0;
	if (e->position < RESUME_MIN_MS)
		return 0;
	return e->position;
}

int watchdbGetState(const char *path) {
	WatchEntry *e = findEntry(path);
	return e ? e->state : WATCH_UNWATCHED;
}

void watchdbUpdate(const char *path, uint64_t position, uint64_t duration) {
	if (!path || !path[0] || strlen(path) >= sizeof(((WatchEntry*)0)->path))
		return;

	WatchEntry *e = findEntry(path);
	if (!e) {
		e = malloc(sizeof(WatchEntry));
		if (!e)
			return;
		strcpy(e->path, path);
		e->next = dbHead;
		dbHead = e;
	}

	e->position = position;
	e->duration = duration;
	if (duration > 0 && position >= (duration * WATCHED_PCT_NUM) / WATCHED_PCT_DEN)
		e->state = WATCH_WATCHED;
	else
		e->state = WATCH_INPROGRESS;
}
