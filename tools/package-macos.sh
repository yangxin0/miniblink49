#!/usr/bin/env bash
#
# package-macos.sh — package the macOS wke SDK (static and/or dynamic lib +
# headers) into a zip for a given build configuration.
#
# The libraries are the CMake `miniblink_static` (libminiblink.a) and `miniblink`
# (libminiblink.dylib) targets — this script builds them and ships their output,
# so the merge/link recipe lives in one place (CMakeLists.txt), shared with the
# in-tree build. $BUILD must already be configured (tools/build-macos.sh does it).
#
# Usage:
#   tools/package-macos.sh <build_dir> <label> <out_zip> [kind]
#
#   kind = static | dynamic | both   (default: both)
#
# e.g.
#   tools/package-macos.sh build-mac release \
#     build-mac/release/miniblink-macos-arm64-release-static.zip static
#
# Produces, inside the zip:
#   <name>/include/wke/*.h, include/win_compat/*.h
#   <name>/lib/libminiblink.a     (engine archives + V8 merged, statically linkable) [static/both]
#   <name>/lib/libminiblink.dylib (shared library exporting the wke C API)           [dynamic/both]
#   <name>/README.md
set -euo pipefail

BUILD="${1:?build dir}"
LABEL="${2:?label (release/debug)}"
OUT="${3:?output zip path}"
KIND="${4:-both}"
WANT_STATIC=false; WANT_DYNAMIC=false
case "$KIND" in
  static)  WANT_STATIC=true ;;
  dynamic) WANT_DYNAMIC=true ;;
  both)    WANT_STATIC=true; WANT_DYNAMIC=true ;;
  *) echo "kind must be static|dynamic|both" >&2; exit 1 ;;
esac

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIB="$BUILD/lib"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 8)"

# Resolve OUT to an absolute path (we cd into a temp dir before zipping).
mkdir -p "$(dirname "$OUT")"
OUT="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"

NAME="$(basename "$OUT" .zip)"
STAGE="$(mktemp -d)/$NAME"
mkdir -p "$STAGE/lib" "$STAGE/include/wke" "$STAGE/include/win_compat"

if [ "$WANT_STATIC" = true ]; then
  # miniblink_static's POST_BUILD libtool step merges the engine archives + V8
  # into libminiblink.a (CMAKE_ARCHIVE_OUTPUT_DIRECTORY = $LIB).
  echo "==> [$LABEL] building miniblink_static target (libminiblink.a)"
  cmake --build "$BUILD" --target miniblink_static -j"$JOBS"
  cp "$LIB/libminiblink.a" "$STAGE/lib/libminiblink.a"

  # Strip local symbols from the archive too (same rationale as the dylib below).
  # The -O3 engine + V8 objects carry ~140MB of local symbols (static functions,
  # etc.). `strip -x` removes only locals, keeping every external symbol — Apple's
  # strip preserves any local still referenced by a relocation, so the archive
  # stays statically linkable (verified: a consumer linking the wke* exports
  # resolves its whole dependency chain with zero relocation errors). Cuts
  # libminiblink.a ~276MB -> ~134MB.
  echo "==> [$LABEL] stripping local symbols from libminiblink.a"
  strip -x "$STAGE/lib/libminiblink.a"
fi

if [ "$WANT_DYNAMIC" = true ]; then
  # CMake writes libminiblink.dylib to $LIB (CMAKE_LIBRARY_OUTPUT_DIRECTORY) with
  # install_name @rpath/libminiblink.dylib (MACOSX_RPATH default).
  echo "==> [$LABEL] building miniblink target (libminiblink.dylib)"
  cmake --build "$BUILD" --target miniblink -j"$JOBS"
  cp "$LIB/libminiblink.dylib" "$STAGE/lib/libminiblink.dylib"

  # Strip the local symbol table. The engine archives carry ~210k local symbols
  # (static functions, etc.) that bloat __LINKEDIT to ~48MB. `strip -x` removes
  # only local symbols, keeping every external/exported symbol — so all wke* C-API
  # exports (the reason to ship a dylib) survive and the library stays linkable.
  # Cuts the dylib roughly in half (~89MB -> ~57MB).
  echo "==> [$LABEL] stripping local symbols from libminiblink.dylib"
  strip -x "$STAGE/lib/libminiblink.dylib"
fi

echo "==> [$LABEL] staging headers + README"
cp "$REPO"/wke/wke.h "$REPO"/wke/wkedefine.h "$REPO"/wke/wkeString.h "$STAGE/include/wke/" 2>/dev/null || true
cp "$REPO"/win_compat/*.h "$STAGE/include/win_compat/" 2>/dev/null || true

LIB_LINES=""; USE_LINES=""
if [ "$WANT_STATIC" = true ]; then
  LIB_LINES+="- \`lib/libminiblink.a\`     — static: all engine archives + V8 8.7 merged into one archive."$'\n'
  USE_LINES+="Link statically:
  clang++ app.mm -Iinclude lib/libminiblink.a -licucore -lxml2 -lxslt -lz -lcurl \\\\
    -framework Cocoa -framework CoreText -framework CoreGraphics \\\\
    -framework CoreFoundation -framework Foundation -framework AppKit -framework Carbon
"
fi
if [ "$WANT_DYNAMIC" = true ]; then
  LIB_LINES+="- \`lib/libminiblink.dylib\` — dynamic: self-contained shared library exporting the wke C API (macOS analog of miniblink.dll)."$'\n'
  USE_LINES+="Link dynamically:
  clang++ app.mm -Iinclude -Llib -lminiblink -Wl,-rpath,@executable_path
"
fi

cat > "$STAGE/README.md" <<EOF
# miniblink macOS SDK (arm64, $LABEL, $KIND)

miniblink \`wke\` C API for macOS (Apple Silicon).

$LIB_LINES- \`include/wke/\`     — public headers (\`wke.h\`, \`wkedefine.h\`, \`wkeString.h\`).
- \`include/win_compat/\` — Win32 shim headers wke.h transitively needs on macOS.

$USE_LINES
EOF

echo "==> [$LABEL] zipping -> $OUT"
mkdir -p "$(dirname "$OUT")"
rm -f "$OUT"
( cd "$(dirname "$STAGE")" && zip -q9 -r "$OUT" "$NAME" )
rm -rf "$(dirname "$STAGE")"
echo "==> [$LABEL] done: $OUT"
ls -lh "$OUT" | awk '{print "    "$5, $NF}'
