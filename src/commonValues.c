#include "common.h"

vita2d_pvf *subtitleFont;
SceUInt32 SCE_CTRL_ENTER;
SceUInt32 SCE_CTRL_CANCEL;
SceUInt32 pressed = 0;
int touchActive = 0;
int touchX = 0;
int touchY = 0;
unsigned char stickLX = 128;
unsigned char stickLY = 128;
vita2d_pgf *pgf;
int position = 0;
int file_count = 0;
char curDir[512] = "app0:";
char root_path[8];
PlayerInitStatus playerStatus;
PlayerState playerState;
ThumbnailStatus thumbnailStatus;
uint64_t streamDuration = 0;
SubtitlesStatus subStatus;
SubtitlesType subType;