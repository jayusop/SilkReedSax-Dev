#!/bin/sh
# Generates the Xcode project (build-xcode/SilkReedSax.xcodeproj) and opens it.
# The CPU is chosen with the build configuration inside Xcode:
#   Debug_AppleSilicon / Release_AppleSilicon  -> arm64
#   Debug_Intel        / Release_Intel         -> x86_64
#   Debug_Universal    / Release_Universal     -> arm64 + x86_64
# Usage: ./xcode.sh [--no-open]
set -e
cd "$(dirname "$0")"
if [ ! -f ThirdParty/JUCE/CMakeLists.txt ]; then
  git clone --depth 1 --branch 8.0.9 https://github.com/juce-framework/JUCE.git ThirdParty/JUCE
fi
command -v cmake >/dev/null || { echo "cmake required: brew install cmake"; exit 1; }
OPEN=1
for a in "$@"; do
  case "$a" in
    --no-open) OPEN=0 ;;
    universal|arm64|x86_64|intel|native)
      echo "Note: the CPU is chosen per build configuration inside Xcode (argument '$a' ignored)." ;;
  esac
done
cmake -B build-xcode -G Xcode
echo "Xcode project: build-xcode/SilkReedSax.xcodeproj"
[ "$OPEN" = 1 ] && open build-xcode/SilkReedSax.xcodeproj
exit 0
