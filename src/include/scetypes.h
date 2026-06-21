/* Compatibility shim: maps the proprietary SCE SDK <scetypes.h> onto VitaSDK.
 * Part of the open-source VitaSDK port of Vita-Media-Player. */
#ifndef VMP_SCETYPES_SHIM_H
#define VMP_SCETYPES_SHIM_H

#include <stdint.h>
#include <stddef.h>
#include <psp2/types.h>
#include <psp2/rtc.h>   /* SceDateTime */

#ifndef SCE_NULL
#define SCE_NULL 0
#endif

#ifndef SCE_OK
#define SCE_OK 0
#endif

#ifndef SCE_UID_INVALID_UID
#define SCE_UID_INVALID_UID ((SceUID)-1)
#endif

#endif /* VMP_SCETYPES_SHIM_H */
