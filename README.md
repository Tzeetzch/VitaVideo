# VitaVideo

A reworked, open-source MP4 video player for the PS Vita / PSTV — a fork of
[SonicMastr's Vita-Media-Player](https://github.com/SonicMastr/Vita-Media-Player)
ported to the **free [VitaSDK](https://vitasdk.org) toolchain** and reshaped
around comfortable couch/in-bed viewing.

The on-device app is still called **Vita Media Player** (Title ID `SVMP00001`);
"VitaVideo" is the name of this fork.

## What's different from upstream

This fork keeps the original's hardware-accelerated `SceAvPlayer` playback and SRT
subtitle support, and adds:

- **Free-toolchain build** — compiles with the open VitaSDK directly via a single
  `build.sh` (no proprietary SDK, no cmake/make required).
- **Better browsing** — browses both `ux0:` and `uma0:`, touch navigation, sort
  options, per-file progress bars, and a "continue watching" banner.
- **Resume & watched tracking** — remembers where you stopped, marks items watched,
  and offers continue / next-episode actions.
- **Home / Folders / Settings tabs** with **Favorites** and **recently-watched** on Home.
- **Per-item action menu** — Play / Restart / Mark watched / Reset progress.
- **Touch player HUD** — on-screen icon buttons for playback control.
- **"Tent mode" comfort features** — brightness levels, an auto-arming **sleep timer**
  (off / 15 / 30 / 60 min), and a battery indicator, for watching in bed or a tent.
- **Persistent settings** — sort order, brightness, and default sleep timer are saved
  to `ux0:data/SubPlayer/settings.txt`.

## Subtitles

SRT subtitle support (the `.srt` must have the same filename as the video).

MKVs do **not** work. To preserve subtitles, convert to MP4 with Timed Text subtitles
(e.g. via HandBrake or FFmpeg).

## Building

Requires the free VitaSDK. The build script drives the toolchain directly:

```bash
# VITASDK defaults to /c/Dev/vitasdk; override the env var if yours differs
bash build.sh          # stable: Title ID SVMP00001  -> build/VitaMediaPlayer.vpk
bash build.sh dev      # dev:    Title ID SVMP00002  -> installs as a second bubble
```

Install the resulting `.vpk` with VitaShell.

> The eboot is built **unsafe** (full permissions) on purpose: the hardware video
> decoder and the taiHEN-based `reAvPlayer` plugin need restricted APIs that a safe
> eboot is denied.

## Credits

All of the original work is by **SonicMastr** and contributors — see the
[upstream repository](https://github.com/SonicMastr/Vita-Media-Player). This fork
only ports the build to the free VitaSDK and adds the features listed above.

Original special thanks:

- **GrapheneCt** — reverse-engineering of `SceAvPlayer` that made high-res playback possible
- **Cuevavirus** — Sharpscale (1080i support)
- **Joel16** — the original ElevenMPV file browser used as a UI base
- **SomeonPC** — Livearea assets
- **Usagi** — relentless encouragement
