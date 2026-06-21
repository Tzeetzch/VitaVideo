#ifndef DIR_H
#define DIR_H

#include <scetypes.h>

typedef struct File {
	struct File *next; // Next item
	SceBool is_dir;        // Folder flag
	char name[256];    // File name
	char ext[5];       // File extension
} File;

extern File *files;
extern char continueLabel[320];

void updateContinueTarget(void);
void drawContinueBanner(void);
void drawItemMenu(void);
void playContinue(void);
void cycleSortOrder(void);
const char *sortOrderName(void);
int getSortOrder(void);
void setSortOrder(int s);
void favLoad(void);
int favCountGet(void);
const char *favGet(int i);
int favIsFav(const char *path);
void favToggle(const char *path);
void openFolder(const char *path);
void playPath(const char *path);
int findNextEpisode(const char *lastPath, char *out, int outSize);
File *getFileIndex(int index);

int getDirListing(SceBool refresh);
void displayFiles();
void openFile(void);
int navigate(SceBool parent);
int getLastDirectory(void);
int handleDirControls(void);

#endif