#ifndef CONFIG_H
#define CONFIG_H

/* Persist user settings (sort order, brightness, default sleep timer) to
 * ux0:data/SubPlayer/settings.txt so they survive a restart. */
void configLoad(void);   /* read + apply (call once at startup) */
void configSave(void);   /* write current settings */

#endif
