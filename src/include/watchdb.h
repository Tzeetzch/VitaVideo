#ifndef WATCHDB_H
#define WATCHDB_H

#include <scetypes.h>
#include <stdint.h>

/* Per-video watch state, persisted to ux0:data/SubPlayer/watched.db */
#define WATCH_UNWATCHED  0
#define WATCH_INPROGRESS 1
#define WATCH_WATCHED    2

/* Load the database from disk into memory (call once at startup). */
void watchdbLoad(void);

/* Persist the in-memory database back to disk. */
void watchdbSave(void);

/* Position (ms) to resume a video from. Returns 0 if the file is unknown,
 * already finished, or only barely started (so it just plays from the top). */
uint64_t watchdbGetResume(const char *path);

/* WATCH_UNWATCHED / WATCH_INPROGRESS / WATCH_WATCHED for a file. */
int watchdbGetState(const char *path);

/* Watched fraction 0..100, or -1 if the file is unknown. */
int watchdbGetProgress(const char *path);

/* Record current playback position; auto-marks WATCHED once near the end. */
void watchdbUpdate(const char *path, uint64_t position, uint64_t duration);

/* Remember / fetch the most recently started video (for Continue / Next). */
void watchdbSetLastPlayed(const char *path);
const char *watchdbGetLastPlayed(void);

#endif
