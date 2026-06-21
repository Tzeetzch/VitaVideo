/* Compatibility shim for the SCE SDK <sceavplayer.h>. VitaSDK port.
 *
 * VitaSDK's <psp2/avplayer.h> exposes only a subset of the SceAvPlayer API.
 * The SceAvPlayer_stub library, however, DOES export the stream-control
 * entry points (verified with nm), and the original player relies on them
 * plus the event/state/stream-type enums. Those are declared here so the
 * upstream sources keep working unchanged.
 *
 * The event-id and stream-type values are the well-known SceAvPlayer
 * constants (shared with the PS4/orbis lineage of the same library). */
#ifndef VMP_SCEAVPLAYER_SHIM_H
#define VMP_SCEAVPLAYER_SHIM_H

#include <psp2/avplayer.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Stream types (SceAvPlayerStreamInfo.type) --- */
typedef enum SceAvPlayerStreamType {
	SCE_AVPLAYER_VIDEO     = 0,
	SCE_AVPLAYER_AUDIO     = 1,
	SCE_AVPLAYER_TIMEDTEXT = 2,
	SCE_AVPLAYER_UNKNOWN   = 3
} SceAvPlayerStreamType;

/* --- Event ids delivered to the SceAvPlayerEventCallback --- */
#define SCE_AVPLAYER_STATE_STOP          0x01
#define SCE_AVPLAYER_STATE_READY         0x02
#define SCE_AVPLAYER_STATE_PLAY          0x03
#define SCE_AVPLAYER_STATE_PAUSE         0x04
#define SCE_AVPLAYER_STATE_BUFFERING     0x05
#define SCE_AVPLAYER_TIMED_TEXT_DELIVERY 0x10
#define SCE_AVPLAYER_WARNING_ID          0x20
#define SCE_AVPLAYER_ENCRYPTION          0x30
#define SCE_AVPLAYER_DRM_ERROR           0x40

/* --- Per-stream info returned by sceAvPlayerGetStreamInfo ---
 * Layout verified against Vita3K's SceAvPlayer implementation:
 *   type@0, reserved@4, details@8 (the reserved word is "unknown" in Vita3K).
 * The original app additionally reads .duration, so the official SDK struct
 * extends past Vita3K's model with duration@24 and startTime@32 (the app
 * compiled against the official header relied on these offsets).
 * The trailing reserved2[] is a safety margin in case the on-device struct
 * is larger than this — sceAvPlayerGetStreamInfo writes into this buffer. */
typedef struct SceAvPlayerStreamInfo {
	uint32_t type;                    /* one of SceAvPlayerStreamType (@0)  */
	uint32_t reserved;                /* "unknown" word                (@4)  */
	SceAvPlayerStreamDetails details; /*                               (@8)  */
	uint64_t duration;                /* stream duration in ms         (@24) */
	uint64_t startTime;               /*                               (@32) */
	uint8_t  reserved2[24];           /* margin against a larger on-device struct */
} SceAvPlayerStreamInfo;

/* Exported by SceAvPlayer_stub but missing from <psp2/avplayer.h> */
int sceAvPlayerStreamCount(SceAvPlayerHandle handle);
int sceAvPlayerGetStreamInfo(SceAvPlayerHandle handle, int argStreamID, SceAvPlayerStreamInfo *argStreamInfo);
int sceAvPlayerEnableStream(SceAvPlayerHandle handle, int argStreamID);
int sceAvPlayerDisableStream(SceAvPlayerHandle handle, int argStreamID);

#ifdef __cplusplus
}
#endif

#endif /* VMP_SCEAVPLAYER_SHIM_H */
