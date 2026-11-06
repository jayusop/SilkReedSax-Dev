#!/bin/sh
# Fetches JUCE (if missing) and configures a command-line (Makefiles) build for one CPU.
# Usage: ./setup.sh [arm64|x86_64|universal|native] [extra -D options]
#   arm64 -> build-arm64/   x86_64 -> build-x86_64/   universal -> build-universal/   native (default) -> this Mac's CPU
set -e
cd "$(dirname "$0")"
. Tools/arch_common.sh
silkreed_parse_arch "$@"
if [ ! -f ThirdParty/JUCE/CMakeLists.txt ]; then
  git clone --depth 1 --branch 8.0.9 https://github.com/juce-framework/JUCE.git ThirdParty/JUCE
fi
command -v cmake >/dev/null || { echo "cmake required: brew install cmake"; exit 1; }
EXTRA=""
for a in "$@"; do case "$a" in -D*) EXTRA="$EXTRA $a" ;; esac; done
cmake -B "$DIR" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DSILKREED_ARCH=$ARCH $EXTRA
