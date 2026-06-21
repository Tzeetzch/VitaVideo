#ifndef AVSOUND_H
#define AVSOUND_H

#include <stdint.h>

int avSoundInit();
int avSoundOutput(uint8_t* argpData, uint32_t argSampleRate, uint32_t argChannelCount);
int avSoundShutdown();
int avSoundOutputEnd();
int avSoundSetVolume(int pct);   /* 0..100 */
int avSoundGetVolume(void);

#endif