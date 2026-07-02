#!/usr/bin/env bash
#
# package-macos.sh — package the macOS wke SDK (static and/or dynamic lib +
# headers) into a zip for a given build configuration.
#
# Usage:
#   tools/package-macos.sh <build_dir> <v8_monolith.a> <label> <out_zip> [kind]
#
#   kind = static | dynamic | both   (default: both)
#
# e.g.
#   tools/package-macos.sh build-mac \
#     ~/build/glyph/v8-8.7/v8/out/arm64.release/obj/libv8_monolith.a \
#     release build-mac/release/miniblink-macos-arm64-release-static.zip static
#
# Produces, inside the zip:
#   <name>/include/wke/*.h, include/win_compat/*.h
#   <name>/lib/libwke.a      (all engine archives + V8 merged, statically linkable)  [static/both]
#   <name>/lib/libwke.dylib  (self-contained shared library exporting the wke C API) [dynamic/both]
#   <name>/README.md
set -euo pipefail

BUILD="${1:?build dir}"
V8MONO="${2:?v8 monolith .a}"
LABEL="${3:?label (release/debug)}"
OUT="${4:?output zip path}"
KIND="${5:-both}"
WANT_STATIC=false; WANT_DYNAMIC=false
case "$KIND" in
  static)  WANT_STATIC=true ;;
  dynamic) WANT_DYNAMIC=true ;;
  both)    WANT_STATIC=true; WANT_DYNAMIC=true ;;
  *) echo "kind must be static|dynamic|both" >&2; exit 1 ;;
esac

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIB="$BUILD/lib"

# Resolve OUT to an absolute path (we cd into a temp dir before zipping).
mkdir -p "$(dirname "$OUT")"
OUT="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"

# Engine archives, in link order (from the minibrowser link line).
ARCHIVES=(
  libwke.a libwke_globals.a libblink_bindings_infra.a libcontent_browser.a libmc.a
  libnet_portable.a libblink_web.a libblink_modules.a libblink_bindings.a libblink_core.a
  libblink_generated.a libblink_platform.a libblink_heap_asm.a libwtf.a libskia_ext.a
  libskia.a libharfbuzz.a libminiblink_png.a libminiblink_jpeg.a libminiblink_webp.a
  libminiblink_ots.a libgin.a libbase.a
)
A_PATHS=(); for a in "${ARCHIVES[@]}"; do A_PATHS+=("$LIB/$a"); done

# For the dylib, mirror the minibrowser link exactly: force_load the wke C-API
# archives (so all wke* exports are included) and link the rest normally (so only
# referenced objects are pulled — avoids dragging in e.g. SQLite-dependent objects
# that would need extra system libs). REST = everything after the first 3.
FORCE=( libwke.a libwke_globals.a libblink_bindings_infra.a )
REST_PATHS=(); for a in "${ARCHIVES[@]:3}"; do REST_PATHS+=("$LIB/$a"); done
FORCE_FLAGS=(); for a in "${FORCE[@]}"; do FORCE_FLAGS+=(-Wl,-force_load,"$LIB/$a"); done

NAME="$(basename "$OUT" .zip)"
STAGE="$(mktemp -d)/$NAME"
mkdir -p "$STAGE/lib" "$STAGE/include/wke" "$STAGE/include/win_compat"

if [ "$WANT_STATIC" = true ]; then
  echo "==> [$LABEL] merging static libwke.a (engine archives + V8)"
  libtool -static -o "$STAGE/lib/libwke.a" "${A_PATHS[@]}" "$V8MONO" 2>/dev/null
fi

if [ "$WANT_DYNAMIC" = true ]; then
  echo "==> [$LABEL] linking shared libwke.dylib"
  clang++ -dynamiclib -o "$STAGE/lib/libwke.dylib" \
    "${FORCE_FLAGS[@]}" "${REST_PATHS[@]}" "$V8MONO" \
    -licucore -lxml2 -lxslt -lz -lcurl \
    -framework Cocoa -framework CoreText -framework CoreGraphics \
    -framework CoreFoundation -framework Foundation -framework AppKit -framework Carbon \
    -Wl,-install_name,@rpath/libwke.dylib

  # Strip the local symbol table. The engine archives carry ~210k local symbols
  # (static functions, etc.) that bloat __LINKEDIT to ~48MB. `strip -x` removes
  # only local symbols, keeping every external/exported symbol — so all wke* C-API
  # exports (the reason to ship a dylib) survive and the library stays linkable.
  # Cuts libwke.dylib roughly in half (~89MB -> ~57MB).
  echo "==> [$LABEL] stripping local symbols from libwke.dylib"
  strip -x "$STAGE/lib/libwke.dylib"
fi

echo "==> [$LABEL] staging headers + README"
cp "$REPO"/wke/wke.h "$REPO"/wke/wkedefine.h "$REPO"/wke/wkeString.h "$STAGE/include/wke/" 2>/dev/null || true
cp "$REPO"/win_compat/*.h "$STAGE/include/win_compat/" 2>/dev/null || true

LIB_LINES=""; USE_LINES=""
if [ "$WANT_STATIC" = true ]; then
  LIB_LINES+="- \`lib/libwke.a\`     — static: all engine archives + V8 8.7 merged into one archive."$'\n'
  USE_LINES+="Link statically:
  clang++ app.mm -Iinclude lib/libwke.a -licucore -lxml2 -lxslt -lz -lcurl \\\\
    -framework Cocoa -framework CoreText -framework CoreGraphics \\\\
    -framework CoreFoundation -framework Foundation -framework AppKit -framework Carbon
"
fi
if [ "$WANT_DYNAMIC" = true ]; then
  LIB_LINES+="- \`lib/libwke.dylib\` — dynamic: self-contained shared library exporting the wke C API."$'\n'
  USE_LINES+="Link dynamically:
  clang++ app.mm -Iinclude -Llib -lwke -Wl,-rpath,@executable_path
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
