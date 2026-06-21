/* Compatibility shim: maps the proprietary SCE SDK <kernel.h> umbrella onto the
 * split VitaSDK kernel headers. Part of the open-source VitaSDK port. */
#ifndef VMP_KERNEL_SHIM_H
#define VMP_KERNEL_SHIM_H

#include <malloc.h>

#include <psp2/types.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/power.h>
#include <psp2/rtc.h>

/* sceKernelPowerTick and SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND are
 * provided natively by <psp2/kernel/processmgr.h>. */

/* Event-flag attribute / wait-mode names differ between the SCE SDK and
 * VitaSDK; map the old names onto VitaSDK's SCE_EVENT_* enum values. */
#ifndef SCE_KERNEL_ATTR_MULTI
#define SCE_KERNEL_ATTR_MULTI SCE_EVENT_WAITMULTIPLE
#endif
#ifndef SCE_KERNEL_EVF_WAITMODE_CLEAR_ALL
#define SCE_KERNEL_EVF_WAITMODE_CLEAR_ALL SCE_EVENT_WAITCLEAR
#endif

#endif /* VMP_KERNEL_SHIM_H */
