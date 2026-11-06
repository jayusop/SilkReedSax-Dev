#!/bin/sh
# Post-build step for every SilkReed bundle (called by CMake for Xcode and Makefiles builds,
# and again by build.sh). Signs the bundle ad hoc and, for plug-ins built for this Mac's CPU,
# installs it into ~/Library/Audio/Plug-Ins.
# Usage: post_build.sh <bundle path> <arm64|x86_64|universal> <install|noinstall>
BUNDLE="$1"; ARCH="$2"; MODE="${3:-install}"
[ -d "$BUNDLE" ] || { echo "post_build: no bundle at $BUNDLE"; exit 1; }

codesign --force --deep --sign - "$BUNDLE" >/dev/null 2>&1
codesign --verify --deep --strict "$BUNDLE" || { echo "post_build: signature check failed: $BUNDLE"; exit 1; }

HOST=$(uname -m)
P="$HOME/Library/Audio/Plug-Ins"
case "$BUNDLE" in
  *.vst3)      DEST="$P/VST3" ;;
  *.component) DEST="$P/Components" ;;
  *)           exit 0 ;;                       # Standalone app: nothing to install
esac
NAME=$(basename "$BUNDLE")
if [ "$MODE" != install ]; then
  echo "post_build: $NAME built for $ARCH (install disabled)"; exit 0
fi
if [ "$ARCH" != "$HOST" ] && [ "$ARCH" != universal ]; then
  echo "post_build: $NAME is $ARCH-only; not installed on this $HOST Mac (copy it to an $ARCH Mac)"; exit 0
fi
mkdir -p "$DEST"
rm -rf "$DEST/$NAME"
ditto "$BUNDLE" "$DEST/$NAME"
# Note: no custom Finder icon on the installed bundle - Finder info is "detritus" that
# invalidates the code signature. The icon ships inside the bundle (Contents/Resources/Icon.icns).
codesign --verify --deep --strict "$DEST/$NAME" || { echo "post_build: installed copy failed signature check"; exit 1; }
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true
echo "post_build: installed $NAME ($ARCH) into $DEST"
