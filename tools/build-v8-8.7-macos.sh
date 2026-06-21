#!/usr/bin/env bash
#
# build-v8-8.7-macos.sh — build a native V8 8.7.220.3 monolith on macOS.
#
# This is the single pinned V8 shared by the Windows and macOS miniblink builds.
# It was brought up on Apple Silicon (arm64) + macOS 26 SDK + Command Line Tools
# only (no full Xcode.app), compiling a 2020 V8 tree with the system clang.
#
# Output: $V8_ROOT/v8/out/<cpu>.release/obj/libv8_monolith.a  (+ include/)
#
# Prereqs handled by this script: depot_tools, a `python`->python3 shim, the
# pinned gn binary, the pinned clang, and a set of small patches (see the
# v8_8_7_*.patch files next to this repo) needed to build a 2020 V8 on a 2026
# toolchain. Re-running is idempotent.
set -euo pipefail

# ---- Config ----------------------------------------------------------------
V8_VERSION="8.7.220.3"
V8_ROOT="${V8_ROOT:-$HOME/build/v8-8.7}"
TARGET_CPU="${TARGET_CPU:-arm64}"          # arm64 (native Apple Silicon) or x64
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # miniblink repo root
DEPOT_TOOLS="${DEPOT_TOOLS:-$HOME/build/depot_tools}"
PYSHIM="${PYSHIM:-$HOME/build/pyshim}"

export DEPOT_TOOLS_UPDATE=0

# ---- depot_tools -----------------------------------------------------------
if [ ! -d "$DEPOT_TOOLS" ]; then
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPOT_TOOLS"
fi
export PATH="$PYSHIM:$DEPOT_TOOLS:$PATH"
# Bootstrap depot_tools' own python3 (creates python3_bin_reldir.txt).
( cd "$DEPOT_TOOLS" && DEPOT_TOOLS_UPDATE=1 ./ensure_bootstrap >/dev/null 2>&1 || true )

# `python` shim -> depot_tools' python3 (has pkg_resources; old V8 calls `python`).
mkdir -p "$PYSHIM"
ln -sf "$DEPOT_TOOLS/$(cat "$DEPOT_TOOLS/python3_bin_reldir.txt")/python3" "$PYSHIM/python"

# ---- Fetch + pin V8 --------------------------------------------------------
mkdir -p "$V8_ROOT" && cd "$V8_ROOT"
cat > .gclient <<EOF
solutions = [
  { "name": "v8", "url": "https://chromium.googlesource.com/v8/v8.git@${V8_VERSION}",
    "managed": False, "custom_deps": {}, "custom_vars": {} },
]
target_os = ["mac"]
EOF
gclient sync -D --no-history --shallow

cd "$V8_ROOT/v8"

# ---- Pinned clang + gn (hooks that the mac_toolchain hook would otherwise block) ----
python tools/clang/scripts/update.py
GN_REV="$(grep -m1 "'gn_version':" DEPS | sed -E "s/.*'gn_version': *'([^']+)'.*/\1/")"
mkdir -p buildtools/mac
echo "gn/gn/mac-amd64 ${GN_REV}" > /tmp/gn.ensure
cipd ensure -root buildtools/mac -ensure-file /tmp/gn.ensure

# ---- Modern jinja2/markupsafe (the bundled 2020 ones break on Python 3.11) ----
DTPY="$DEPOT_TOOLS/$(cat "$DEPOT_TOOLS/python3_bin_reldir.txt")/python3"
"$DTPY" -m pip install --quiet --target /tmp/jenv jinja2 markupsafe
for pkg in jinja2 markupsafe; do
  [ -d "third_party/$pkg" ] && [ ! -d "third_party/$pkg.orig" ] && mv "third_party/$pkg" "third_party/$pkg.orig"
  rm -rf "third_party/$pkg"; cp -R "/tmp/jenv/$pkg" "third_party/$pkg"
done

# ---- Apply source/tooling patches (idempotent) -----------------------------
git apply --check "$REPO_DIR/v8_8_7_macos.patch" 2>/dev/null && git apply "$REPO_DIR/v8_8_7_macos.patch" || true
( cd build && git apply --check "$REPO_DIR/v8_8_7_build_dir.patch" 2>/dev/null && git apply "$REPO_DIR/v8_8_7_build_dir.patch" || true )
git apply --check "$REPO_DIR/v8_8_7_dcheck_kmax.patch" 2>/dev/null && git apply "$REPO_DIR/v8_8_7_dcheck_kmax.patch" || true
( cd third_party/zlib && git apply --check "$REPO_DIR/v8_8_7_zlib.patch" 2>/dev/null && git apply "$REPO_DIR/v8_8_7_zlib.patch" || true )

# ---- Configure + build -----------------------------------------------------
OUT="out/${TARGET_CPU}.release"
mkdir -p "$OUT"
cat > "$OUT/args.gn" <<EOF
is_debug = false
target_cpu = "${TARGET_CPU}"
v8_target_cpu = "${TARGET_CPU}"
v8_monolithic = true
v8_use_external_startup_data = false
is_component_build = false
use_custom_libcxx = false
treat_warnings_as_errors = false
v8_enable_i18n_support = false
v8_enable_inspector = false
is_clang = true
clang_base_path = "/Library/Developer/CommandLineTools/usr"
clang_use_chrome_plugins = false
symbol_level = 1
v8_enable_sandbox = false
dcheck_always_on = true  # avoids a release-optimizer heisenbug in DOM-wrapper construct (baidu)
EOF

gn gen "$OUT"
autoninja -C "$OUT" v8_monolith

echo
echo "==> Built: $V8_ROOT/v8/$OUT/obj/libv8_monolith.a"
lipo -info "$OUT/obj/libv8_monolith.a" 2>/dev/null || true
echo "==> Headers: $V8_ROOT/v8/include"
