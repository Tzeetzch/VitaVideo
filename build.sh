#!/bin/bash
# Build script for the open-source VitaSDK port of Vita-Media-Player.
# Drives the VitaSDK toolchain directly (no cmake/make required).
set -e

: "${VITASDK:=/c/Dev/vitasdk}"
export VITASDK
PREFIX="$VITASDK/bin/arm-vita-eabi"
CC="$PREFIX-gcc"
OBJCOPY="$PREFIX-objcopy"
PACK_VPK="$VITASDK/bin/vita-pack-vpk"
MAKE_FSELF="$VITASDK/bin/vita-make-fself"
ELF_CREATE="$VITASDK/bin/vita-elf-create"
MKSFOEX="$VITASDK/bin/vita-mksfoex"

# Variant: "stable" keeps the original identity (SVMP00001) so it overwrites the
# proven build; "dev" uses a distinct Title ID + name so it installs as a SECOND
# bubble alongside the stable one (so the working version stays untouched).
VARIANT="${1:-stable}"
case "$VARIANT" in
  stable) TITLE="Vita Media Player";     TITLEID="SVMP00001"; OUT="VitaMediaPlayer";;
  dev)    TITLE="Vita Media Player DEV"; TITLEID="SVMP00002"; OUT="VitaMediaPlayerDEV";;
  *) echo "usage: build.sh [stable|dev]"; exit 1;;
esac
echo ">> Variant: $VARIANT  (Title ID $TITLEID, \"$TITLE\")"

cd "$(dirname "$0")"
BUILD=build
mkdir -p "$BUILD/resources"

# -fcommon: texture.h defines (non-extern) globals included by several units;
# the original SNC compiler merged them as common symbols. GCC 10 defaults to
# -fno-common, so restore the old behaviour rather than rewrite the headers.
CFLAGS="-Wl,-q -O2 -std=c99 -DNDEBUG -fcommon -Isrc/include -Wno-format"
SRCS="
  src/main.c src/utils.c src/dir.c src/fs.c src/texture.c src/commonValues.c src/watchdb.c src/tent.c
  src/menus/menuMain.c src/menus/menuInfo.c
  src/avplayer/avsubs.c src/avplayer/avplayer.c src/avplayer/avsound.c
  src/avplayer/avplayerUtils.c src/avplayer/overlay.c
"

# --- Embed PNG resources as object files (symbol names: _binary_resources_<name>_png_*) ---
RES_OBJS=""
for png in resources/*.png; do
  base=$(basename "$png" .png)
  obj="$BUILD/resources/${base}.png.o"
  # Run from project root so the symbol name encodes "resources/<base>.png".
  "$OBJCOPY" -I binary -O elf32-littlearm -B arm "$png" "$obj"
  RES_OBJS="$RES_OBJS $obj"
done

# --- Compile + link ---
LIBS="
  -lvita2d -lScePgf_stub -lScePvf_stub -lfreetype -lpng16 -ljpeg -lz
  -lSceGxm_stub -lSceCommonDialog_stub -lSceDisplay_stub -lSceSysmem_stub
  -lSceLibKernel_stub -lSceKernelThreadMgr_stub -lSceProcessmgr_stub
  -lSceCtrl_stub -lSceTouch_stub -lSceAudio_stub -lSceSysmodule_stub -lSceAppUtil_stub
  -lSceAppMgr_stub -lSceAvPlayer_stub -lScePower_stub -lSceAVConfig_stub -lm
"
echo ">> Compiling and linking..."
$CC $CFLAGS $SRCS $RES_OBJS $LIBS -o "$BUILD/$OUT.elf"

# --- Make the VPK ---
echo ">> Creating velf / eboot..."
"$ELF_CREATE" "$BUILD/$OUT.elf" "$BUILD/$OUT.velf"
# NOTE: no "-s" -> build an UNSAFE eboot (full permissions). The hardware video
# decoder (SceAvcodec) and the taiHEN-based reAvPlayer plugin need restricted
# APIs that a safe eboot is denied. This matches the original app's build.
"$MAKE_FSELF" "$BUILD/$OUT.velf" "$BUILD/eboot.bin"
"$MKSFOEX" -s TITLE_ID="$TITLEID" "$TITLE" "$BUILD/param.sfo"

echo ">> Packing VPK..."
"$PACK_VPK" -s "$BUILD/param.sfo" -b "$BUILD/eboot.bin" \
  --add resources/OpenSans-Bold.ttf=OpenSans-Bold.ttf \
  --add modules/reAvPlayer.suprx=modules/reAvPlayer.suprx \
  --add VMP_livearea/icon0.png=sce_sys/icon0.png \
  --add VMP_livearea/livearea/contents/bg0.png=sce_sys/livearea/contents/bg0.png \
  --add VMP_livearea/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
  --add VMP_livearea/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
  "$BUILD/$OUT.vpk"

echo ">> Done: $BUILD/$OUT.vpk"
