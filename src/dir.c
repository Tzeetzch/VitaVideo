/* All Credits for this goes to Joel16 on github and his ElevenMPV app */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "avplayer.h"
#include "common.h"
#include "dir.h"
#include "utils.h"
#include "fs.h"
#include "texture.h"
#include "watchdb.h"

#ifndef SCE_S_ISDIR
#define 	SCE_S_ISDIR(m)   (((m) & SCE_S_IFMT) == SCE_S_IFDIR)
#endif

int config = 0;

File *files = NULL;

char continueTarget[512] = {0};   /* full path the Continue/Next action will play */
char continueLabel[320] = {0};    /* on-screen hint, e.g. "Next: Ep02.mp4" */


static void dirListFree(File *node) {
	if (node == NULL) // End of list
		return;
	
	dirListFree(node->next); // Nest further
	free(node); // Free memory
}

static void saveLastDirectory(void) {
	char *buf = malloc(256);
	int len = snprintf(buf, 256, "%s\n", curDir);
	writeFile("ux0:data/SubPlayer/lastdir.txt", buf, len);
	free(buf);
}

static int cmpstringp(const void *p1, const void *p2) {
	SceIoDirent *entryA = (SceIoDirent *)p1;
	SceIoDirent *entryB = (SceIoDirent *)p2;

	if ((SCE_S_ISDIR(entryA->d_stat.st_mode)) && !(SCE_S_ISDIR(entryB->d_stat.st_mode)))
		return -1;
	else if (!(SCE_S_ISDIR(entryA->d_stat.st_mode)) && (SCE_S_ISDIR(entryB->d_stat.st_mode)))
		return 1;
	else {
		if (config == 0) // Sort naturally (ascending: Ep1, Ep2 ... Ep10, Ep20)
			return strcmpnat(entryA->d_name, entryB->d_name);
		else if (config == 1) // Sort naturally (descending)
			return strcmpnat(entryB->d_name, entryA->d_name);
		else if (config == 2) // Sort by file size (largest first)
			return entryA->d_stat.st_size > entryB->d_stat.st_size ? -1 : entryA->d_stat.st_size < entryB->d_stat.st_size ? 1 : 0;
		else if (config == 3) // Sort by file size (smallest first)
			return entryB->d_stat.st_size > entryA->d_stat.st_size ? -1 : entryB->d_stat.st_size < entryA->d_stat.st_size ? 1 : 0;
	}

	return 0;
}

static int cmpNatName(const void *p1, const void *p2) {
	return strcmpnat(((SceIoDirent *)p1)->d_name, ((SceIoDirent *)p2)->d_name);
}

/* Find the video that comes after lastPath in the same folder (natural order).
 * Returns 0 and fills out on success, -1 if there is no next episode. */
int findNextEpisode(const char *lastPath, char *out, int outSize) {
	const char *slash = strrchr(lastPath, '/');
	if (!slash)
		return -1;
	int folderLen = (int)(slash - lastPath) + 1;   /* keep the trailing '/' */
	char folder[512];
	if (folderLen <= 0 || folderLen >= (int)sizeof(folder))
		return -1;
	memcpy(folder, lastPath, folderLen);
	folder[folderLen] = '\0';
	const char *lastName = slash + 1;

	SceUID dir = sceIoDopen(folder);
	if (dir < 0)
		return -1;

	SceIoDirent *entries = (SceIoDirent *)calloc(MAX_FILES, sizeof(SceIoDirent));
	if (!entries) { sceIoDclose(dir); return -1; }

	int count = 0;
	while (count < MAX_FILES && sceIoDread(dir, &entries[count]) > 0) {
		if (!SCE_S_ISDIR(entries[count].d_stat.st_mode) &&
		    !strncasecmp(getFileExt(entries[count].d_name), "mp4", 4))
			count++;   /* keep mp4 files; otherwise the slot is reused */
	}
	sceIoDclose(dir);

	qsort(entries, count, sizeof(SceIoDirent), cmpNatName);

	int ret = -1;
	for (int i = 0; i < count; i++) {
		if (!strcmp(entries[i].d_name, lastName)) {
			if (i + 1 < count) {
				int n = snprintf(out, outSize, "%s%s", folder, entries[i + 1].d_name);
				if (n > 0 && n < outSize)
					ret = 0;
			}
			break;
		}
	}
	free(entries);
	return ret;
}

/* Recompute what the Continue/Next action should play, plus its on-screen hint:
 *  - last video still in progress -> resume it ("Continue: ...")
 *  - last video finished          -> next episode in its folder ("Next: ...") */
void updateContinueTarget(void) {
	continueTarget[0] = '\0';
	continueLabel[0] = '\0';

	const char *last = watchdbGetLastPlayed();
	if (!last || !last[0])
		return;

	const char *baseName;
	if (watchdbGetState(last) == WATCH_INPROGRESS) {
		strncpy(continueTarget, last, sizeof(continueTarget) - 1);
		continueTarget[sizeof(continueTarget) - 1] = '\0';
		baseName = strrchr(continueTarget, '/');
		baseName = baseName ? baseName + 1 : continueTarget;
		snprintf(continueLabel, sizeof(continueLabel), "Continue: %.60s", baseName);
	} else {
		char nxt[512];
		if (findNextEpisode(last, nxt, sizeof(nxt)) == 0) {
			strncpy(continueTarget, nxt, sizeof(continueTarget) - 1);
			continueTarget[sizeof(continueTarget) - 1] = '\0';
			baseName = strrchr(continueTarget, '/');
			baseName = baseName ? baseName + 1 : continueTarget;
			snprintf(continueLabel, sizeof(continueLabel), "Next: %.60s", baseName);
		}
	}
}

int getDirListing(SceBool refresh)
{
    SceUID dir = 0;
    dirListFree(files);
	files = NULL;
	file_count = 0;

    SceBool parent_dir_set = SCE_FALSE;

    // Virtual root: when curDir is empty, list the available storage devices
    // instead of a real directory, so the user can choose ux0:, uma0:, etc.
    if (curDir[0] == '\0') {
        static const char *devices[] = {
            "ux0:", "uma0:", "ur0:", "imc0:", "xmc0:", "grw0:"
        };
        char probe[16];
        for (int d = 0; d < (int)(sizeof(devices) / sizeof(devices[0])); d++) {
            snprintf(probe, sizeof(probe), "%s/", devices[d]);
            if (!dirExists(probe))
                continue;

            File *item = (File *)malloc(sizeof(File));
            memset(item, 0, sizeof(File));
            item->is_dir = SCE_TRUE;
            strcpy(item->name, devices[d]);
            file_count++;

            if (files == NULL)
                files = item;
            else {
                File *list = files;
                while (list->next != NULL)
                    list = list->next;
                list->next = item;
            }
        }

        if (!refresh) {
            if (position >= file_count)
                position = file_count - 1;
        }
        else
            position = 0;

        return 0;
    }

    if (R_SUCCEEDED(dir = sceIoDopen(curDir))) {
        int count = 0;
        SceIoDirent *entries = (SceIoDirent *)calloc(MAX_FILES, sizeof(SceIoDirent));
        while (sceIoDread(dir, &entries[count]) > 0)
			count++;
        sceIoDclose(dir);
		printf("Closed Directory\n");
		qsort(entries, count, sizeof(SceIoDirent), cmpstringp);
		printf("Sorted\n");
        for (int i = -1; i < count; i++) {
			// Allocate Memory
			File *item = (File *)malloc(sizeof(File));
			memset(item, 0, sizeof(File));
			printf("Allocated Memory\n");
			if ((strcmp(curDir, root_path)) && (i == -1) && (!parent_dir_set)) {
				strcpy(item->name, "..");
				item->is_dir = SCE_TRUE;
				parent_dir_set = SCE_TRUE;
				file_count++;
			}
			else {
				if ((i == -1) && (!(strcmp(curDir, root_path))))
					continue;

				item->is_dir = SCE_S_ISDIR(entries[i].d_stat.st_mode);

				// Copy File Name
				strcpy(item->name, entries[i].d_name);
				strcpy(item->ext, getFileExt(item->name));
				file_count++;
			}

			// New List
			if (files == NULL) 
				files = item;

			// Existing List
			else {
				File *list = files;
					
				while(list->next != NULL) 
					list = list->next;

				list->next = item;
			}
		}

    }
    else
		return dir;
    
    if (!refresh) {
        if (position >= file_count)
            position = file_count - 1;
    }
    else
        position = 0;

    return 0;
}

void displayFiles() {
	int i = 0, printed = 0;
	File *file = files; // Draw file list
	int startXScale = (int)(FRAMEBUF_WIDTH*.0520f);
	float textureScale = (FRAMEBUF_WIDTH*.2f)/200.0f;
	float basePgfScale = FRAMEBUF_HEIGHT/544; // 544 is the perfect size for a scale of 1, and the default anyway
	for(; file != NULL; file = file->next) {
		if (printed == FILES_PER_PAGE)
			break;
		if (position < FILES_PER_PAGE || i > (position - FILES_PER_PAGE)) {
			if (i == position) {
				vita2d_draw_rectangle(0, (FRAMEBUF_HEIGHT*ENTRY_SCALE*printed), FRAMEBUF_WIDTH*0.6f, FRAMEBUF_HEIGHT*ENTRY_SCALE, RGBA8(120, 120, 255, 200));
				if (file->is_dir)
					vita2d_draw_texture_scale(folder, FRAMEBUF_WIDTH - (FRAMEBUF_WIDTH*.3f), FRAMEBUF_HEIGHT*.1f, textureScale, textureScale);
				else if ((!strncasecmp(file->ext, "flac", 4)) || (!strncasecmp(file->ext, "it", 4)) || (!strncasecmp(file->ext, "mod", 4))
					|| (!strncasecmp(file->ext, "mp3", 4)) || (!strncasecmp(file->ext, "ogg", 4)) || (!strncasecmp(file->ext, "opus", 4))
					|| (!strncasecmp(file->ext, "s3m", 4))|| (!strncasecmp(file->ext, "wav", 4)) || (!strncasecmp(file->ext, "xm", 4)))
						vita2d_draw_texture_scale(music, FRAMEBUF_WIDTH - (FRAMEBUF_WIDTH*.3f), FRAMEBUF_HEIGHT*.1f, textureScale, textureScale);
				else if ((!strncasecmp(file->ext, "mp4", 4)))
					vita2d_draw_texture_scale(video, FRAMEBUF_WIDTH - (FRAMEBUF_WIDTH*.3f), FRAMEBUF_HEIGHT*.1f, textureScale, textureScale);
				int textWidth = vita2d_pgf_text_width(pgf, basePgfScale, file->name);
				int x = (FRAMEBUF_WIDTH*.8f) - (textWidth/2.0f);
				if (FRAMEBUF_WIDTH*.65f > ((FRAMEBUF_WIDTH*.8f)-(textWidth/2.0f)))
					x = FRAMEBUF_WIDTH*.66f;
				vita2d_pgf_draw_text(pgf, x, (FRAMEBUF_HEIGHT*ENTRY_SCALE/2.0f) + (FRAMEBUF_HEIGHT*.5f), RGBA8(255, 255, 255, 255), basePgfScale, file->name);
			}

			if (strncmp(file->name, "..", 2) == 0)
				vita2d_pgf_draw_text(pgf, startXScale, (FRAMEBUF_HEIGHT*ENTRY_SCALE/2.0f) + (FRAMEBUF_HEIGHT*ENTRY_SCALE*(float)printed), RGBA8(255, 255, 255, 255), basePgfScale*.9f, "Parent folder");
			else
				vita2d_pgf_draw_text(pgf, startXScale, (FRAMEBUF_HEIGHT*ENTRY_SCALE/2.0f) + (FRAMEBUF_HEIGHT*ENTRY_SCALE*(float)printed), RGBA8(255, 255, 255, 255), basePgfScale*.9f, file->name);

			// Watched / in-progress dot for video files (green = watched, orange = partway)
			if (!file->is_dir && !strncasecmp(file->ext, "mp4", 4)) {
				char fullPath[512];
				snprintf(fullPath, sizeof(fullPath), "%s%s", curDir, file->name);
				int watchState = watchdbGetState(fullPath);
				if (watchState != WATCH_UNWATCHED) {
					float dotSize = FRAMEBUF_HEIGHT*ENTRY_SCALE*0.28f;
					float dotY = (FRAMEBUF_HEIGHT*ENTRY_SCALE*(float)printed) + (FRAMEBUF_HEIGHT*ENTRY_SCALE - dotSize)/2.0f;
					unsigned int dotColor = (watchState == WATCH_WATCHED) ? RGBA8(40, 200, 40, 255) : RGBA8(255, 170, 0, 255);
					vita2d_draw_rectangle(FRAMEBUF_WIDTH*0.022f, dotY, dotSize, dotSize, dotColor);
				}
			}

			printed++;
		}

		i++;
	}
	vita2d_draw_line(FRAMEBUF_WIDTH*0.6f,0,FRAMEBUF_WIDTH*0.6f,FRAMEBUF_HEIGHT, RGBA8(255, 255, 255, 255));
}

File *getFileIndex(int index) {
	int i = 0;
	File *file = files; // Find file Item
	
	for(; file != NULL && i != index; file = file->next)
		i++;

	return file; // Return file
}

int navigate(SceBool parent) {
	File *file = getFileIndex(position); // Get index
	
	if (file == NULL)
		return -1;

	// Special case ".."
	if ((parent) || (!strncmp(file->name, "..", 2))) {
		char *slash = NULL;

		// Find last '/' in working directory
		int i = strlen(curDir) - 2; for(; i >= 0; i--) {
			// Slash discovered
			if (curDir[i] == '/') {
				slash = curDir + i + 1; // Save pointer
				break; // Stop search
			}
		}

		if (slash == NULL)
			curDir[0] = 0; // At a device root (e.g. "ux0:/") -> back to device list
		else
			slash[0] = 0; // Terminate working directory
	}

	// Normal folder
	else {
		if (file->is_dir) {
			// Append folder to working directory
			strcpy(curDir + strlen(curDir), file->name);
			curDir[strlen(curDir) + 1] = 0;
			curDir[strlen(curDir)] = '/';
		}
	}

	saveLastDirectory();

	return 0; // Return success
}

void openFile(void) {
	char path[512];
	File *file = getFileIndex(position);

	if (file == NULL)
		return;

	strcpy(path, curDir);
	strcpy(path + strlen(path), file->name);

	if (file->is_dir) {
		// Attempt to navigate to target
		if (R_SUCCEEDED(navigate(SCE_FALSE))) {
			getDirListing(SCE_TRUE);
		}
	}
	else if ((!strncasecmp(file->ext, "mp4", 4))) {
		startPlayback(path);
    }
		//Menu_PlayAudio(path);
}

int getLastDirectory(void) {
	int ret = 0;

	// Empty root_path means the virtual device-selection screen is the top
	// level, so navigation can reach every mounted device (ux0:, uma0:, ...).
	root_path[0] = '\0';

	if (!fileExists("ux0:data/SubPlayer/lastdir.txt")) {
		curDir[0] = '\0'; // Start on the device list
	}
	else {
		SceOff size = 0;

		getFileSize("ux0:data/SubPlayer/lastdir.txt", &size);
		char *buf = malloc(size + 1);

		if (R_FAILED(ret = readFilemp("ux0:data/SubPlayer/lastdir.txt", buf, size))) {
			free(buf);
			return ret;
		}

		buf[size] = '\0';
		char path[512];
		path[0] = '\0';
		sscanf(buf, "%[^\n]s", path);

		if (path[0] != '\0' && dirExists(path)) // Restore last dir if it still exists...
			strcpy(curDir, path);
		else
			curDir[0] = '\0'; // ...otherwise fall back to the device list

		free(buf);
	}

	return 0;
}

int handleDirControls()
{
	if (file_count > 0) {
		if (pressed & SCE_CTRL_UP)
			position--;
		else if (pressed & SCE_CTRL_DOWN)
			position++;

		setMax(&position, 0, file_count - 1);
		setMin(&position, file_count - 1, 0);

		if (pressed & SCE_CTRL_LEFT)
			position = 0;
		else if (pressed & SCE_CTRL_RIGHT)
			position = file_count - 1;

		if (pressed & SCE_CTRL_CROSS) {
			File *sel = getFileIndex(position);
			SceBool wasVideo = (sel && !sel->is_dir);
			openFile();
			if (wasVideo) {                 /* just returned from playback */
				updateContinueTarget();
				getDirListing(SCE_FALSE);   /* refresh watched dots, keep position */
			}
		}
	}

	/* Triangle: smart Continue / Next-episode (resume last, or play the
	 * following episode if the last one is finished). */
	if ((pressed & SCE_CTRL_TRIANGLE) && continueTarget[0]) {
		startPlayback(continueTarget);
		updateContinueTarget();
		getDirListing(SCE_FALSE);
	}

	if ((strcmp(curDir, root_path) != 0) && (pressed & SCE_CTRL_CANCEL)) {
		navigate(SCE_TRUE);
		getDirListing(SCE_TRUE);
	}
    return 0;
}

