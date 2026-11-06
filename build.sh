#!/bin/sh
# Builds VST3 + AU + Standalone (Release).
# Usage: ./build.sh             this Mac's CPU
#        ./build.sh arm64       Apple Silicon                 -> build-arm64/
#        ./build.sh x86_64      Intel (alias: intel)          -> build-x86_64/
#        ./build.sh universal   Apple Silicon + Intel in one  -> build-universal/
# Plug-ins are installed into ~/Library/Audio/Plug-Ins only when they can run on this Mac.
set -e
cd "$(dirname "$0")"
. Tools/arch_common.sh
silkreed_parse_arch "$@"
./setup.sh "$@"
cmake --build "$DIR" --config Release -j"$(sysctl -n hw.ncpu)"
OUT="$DIR/SilkReedSax_artefacts/Release"
sh Tools/check_arch.sh "$OUT" "$ARCH"
# Re-run the post-build step: it also runs when nothing had to be relinked
INSTALL=install
grep -q "SILKREED_INSTALL_PLUGINS:BOOL=OFF" "$DIR/CMakeCache.txt" && INSTALL=noinstall
for B in "$OUT/VST3/SilkReed Sax.vst3" "$OUT/AU/SilkReed Sax.component" "$OUT/Standalone/SilkReed Sax.app"; do
  sh Tools/post_build.sh "$B" "$ARCH" "$INSTALL"
done
echo "Bundles: $OUT/{VST3,AU,Standalone}"
