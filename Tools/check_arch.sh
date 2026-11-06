#!/bin/sh
# Verifies the CPU architectures of the SilkReed binaries in an artefacts dir.
# Usage: Tools/check_arch.sh <artefacts/<config> dir> <arm64|x86_64|universal>
DIR="$1"; EXPECT="$2"
status=0
for B in "$DIR/VST3/SilkReed Sax.vst3/Contents/MacOS/SilkReed Sax" \
         "$DIR/AU/SilkReed Sax.component/Contents/MacOS/SilkReed Sax" \
         "$DIR/Standalone/SilkReed Sax.app/Contents/MacOS/SilkReed Sax"; do
  [ -f "$B" ] || { echo "missing: $B"; status=1; continue; }
  ARCHS=$(lipo -archs "$B")
  ok=0
  if [ "$EXPECT" = universal ]; then
    case "$ARCHS" in *arm64*x86_64*|*x86_64*arm64*) ok=1 ;; esac
  elif [ "$ARCHS" = "$EXPECT" ]; then ok=1; fi
  if [ $ok = 1 ]; then echo "$EXPECT OK ($ARCHS)  ${B#$DIR/}"; else echo "WRONG ARCH: expected $EXPECT, got ($ARCHS)  ${B#$DIR/}"; status=1; fi
done
exit $status
