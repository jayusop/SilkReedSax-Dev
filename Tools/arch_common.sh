# Shared by build.sh / setup.sh: parses the target-CPU argument.
#   native (default: this Mac's CPU) | arm64 (aliases: apple, silicon) | x86_64 (aliases: intel, x64)
#   | universal (arm64 + x86_64 in one binary)
# Sets ARCH (arm64, x86_64 or universal) and DIR (build-<ARCH>).
silkreed_parse_arch() {
  ARCH=$(uname -m)
  for a in "$@"; do
    case "$a" in
      arm64|apple|silicon|applesilicon) ARCH=arm64 ;;
      x86_64|x64|intel)                 ARCH=x86_64 ;;
      native)                           ARCH=$(uname -m) ;;
      universal|uni)                    ARCH=universal ;;
    esac
  done
  DIR="build-$ARCH"
}
