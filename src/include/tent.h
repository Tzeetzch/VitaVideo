#ifndef TENT_H
#define TENT_H

/* "Tent" comfort features: battery readout, screen dimming, sleep timer. */

void tentInit(void);              /* capture max brightness, reset state */
void tentRestoreBrightness(void); /* set screen back to full (call on exit) */

int  tentBatteryPercent(void);    /* 0..100 */

void tentCycleBrightness(void);   /* cycle 100/60/30/10 % and apply */
int  tentBrightnessPercent(void);
int  tentGetBrightnessIdx(void);
void tentSetBrightnessIdx(int i); /* restore a saved level and apply */

void tentCycleSleep(void);        /* cycle off/15/30/60 min */
void tentCancelSleep(void);       /* turn the sleep timer off */
int  tentSleepMinutes(void);      /* current setting (0 = off) */
int  tentSleepRemainingSec(void); /* seconds left, or -1 if off */
int  tentSleepExpired(void);      /* 1 once the deadline has passed */
int  tentSleepArmed(void);        /* 1 if a deadline is set (manual or default) */

int  tentGetSleepDefault(void);   /* default sleep minutes (0 = off) */
void tentSetSleepDefault(int m);
void tentCycleSleepDefault(void);
void tentArmSleepDefault(void);   /* arm the default at playback start */

#endif
