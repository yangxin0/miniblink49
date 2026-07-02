#!/usr/bin/env bash
#
# build-macos.sh — configure + build the macOS engine / mini-browser / demo.
#
# Wraps the CMake invocation so you don't have to remember:
#   * the engine is gated behind -DMINIBLINK_BUILD_BLINK/GIN/SKIA=ON,
#   * executables must be force-relinked (the V8 monolith is force_load'd, a dep
#     CMake doesn't track, so a stale binary won't pick up rebuilt archives).
#
# Usage:
#   tools/build-macos.sh [release|debug] [target ...]
#
# Examples:
#   tools/build-macos.sh                       # release minibrowser (default)
#   tools/build-macos.sh release mb_demo       # release demo
#   tools/build-macos.sh debug                 # debug minibrowser
#   tools/build-macos.sh release wke wke_globals   # just the wke libs
#
# Prereq: the pinned V8 8.7 monolith (tools/build-v8-8.7-macos.sh). Override its
# location with MINIBLINK_V8_ROOT (default: the v8-8.7/v8 checkout sitting
# alongside this repo, i.e. ../v8-8.7/v8).
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CONFIG="${1:-release}"; [ $# -gt 0 ] && shift || true
TARGETS=("$@"); [ ${#TARGETS[@]} -eq 0 ] && TARGETS=(minibrowser)

case "$CONFIG" in
    release) BUILD="build-mac";       BUILD_TYPE="Release" ;;
    debug)   BUILD="build-mac-debug"; BUILD_TYPE="Debug"   ;;
    *) echo "usage: tools/build-macos.sh [release|debug] [target ...]" >&2; exit 1 ;;
esac

V8ROOT="${MINIBLINK_V8_ROOT:-$REPO/../v8-8.7/v8}"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 8)"
BDIR="$REPO/$BUILD"

echo "==> configure $BUILD ($BUILD_TYPE)"
cmake -S "$REPO" -B "$BDIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DMINIBLINK_BUILD_BLINK=ON -DMINIBLINK_BUILD_GIN=ON -DMINIBLINK_BUILD_SKIA=ON \
    -DMINIBLINK_V8_ROOT="$V8ROOT" >/dev/null

# Force a relink of any executable target (stale exe won't see rebuilt archives
# because the V8 monolith is pulled via -force_load, which CMake doesn't track).
for t in "${TARGETS[@]}"; do
    [ -f "$BDIR/bin/$t" ] && rm -f "$BDIR/bin/$t"
done

echo "==> build: ${TARGETS[*]}  (-j$JOBS)"
cmake --build "$BDIR" --target "${TARGETS[@]}" -j"$JOBS"

echo "==> done. artifacts in $BUILD/bin and $BUILD/lib"
for t in "${TARGETS[@]}"; do
    [ -f "$BDIR/bin/$t" ] && ls -lh "$BDIR/bin/$t" | awk '{print "    "$5"  "$NF}'
done
