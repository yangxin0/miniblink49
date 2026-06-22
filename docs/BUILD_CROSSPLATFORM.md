# Cross-platform build (CMake) + pinned V8 8.7

miniblink49 historically builds **only** on Windows via Visual Studio
(`build/miniblink.sln`, 182 `.vcxproj` files). This document describes the
**cross-platform build method** that lets the project build on both Windows and
macOS.

## JavaScript engine: one pinned V8 (8.7.220.3), shared by both OSes

The six in-tree vendored V8 copies (`v8_4_5` … `v8_7_5`, plus their `build/v8_*`
and `gen/v8_*`) have been **removed**. Both platforms now use a **single pinned
V8 8.7.220.3**, fetched from upstream and built from source.

Why 8.7: it is the **first V8 release with native Apple Silicon (arm64-darwin)
support** (Chrome 87 / Electron 11, Nov 2020), while being the closest release
to the engine's existing V8 ~7.5 API — so it minimizes engine-side API
migration.

### Building V8 on macOS

```sh
tools/build-v8-8.7-macos.sh          # native arm64 (default)
TARGET_CPU=x64 tools/build-v8-8.7-macos.sh   # x64 if needed
```

Output: `~/build/v8-8.7/v8/out/<cpu>.release/obj/libv8_monolith.a` + `include/`.

> **Status: DONE & verified.** A native **arm64** `libv8_monolith.a` (89 MB, 700
> objects, embedded snapshot) builds and **runs JavaScript** on macOS 26 with
> Command Line Tools only (no full Xcode). Verified via `test/v8/v8_smoketest`
> (`1 + 2 * 3 = 7`).

### What it took (a 2020 V8 on a 2026 toolchain)

The build script and these patches (in patches/) capture the bring-up:

- `v8_8_7_macos.patch` — V8 source fixes for Clang 21: `bit-field.h` `kMax`
  (out-of-enum-range `constexpr` cast), and scoped-enum comparison casts in
  `shared-function-info.h` / `scope-info.h` / `allocation-site.h`.
- `v8_8_7_build_dir.patch` — build `//build`: tolerate **Command Line Tools**
  (no full Xcode) in `sdk_info.py` / `find_sdk.py`; drop clang-12-only `-mllvm`
  flags from `config/compiler/BUILD.gn` so the system clang works.
- `v8_8_7_zlib.patch` — `third_party/zlib/zutil.h`: the modern macOS SDK defines
  `TARGET_OS_MAC`, which wrongly `#define`d `fdopen NULL`.
- Plus (handled by the script): a `python`→python3 shim, depot_tools bootstrap,
  pinned `gn` (mac-amd64, via Rosetta), the pinned clang, and replacing the
  bundled 2020 `jinja2`/`markupsafe` (broken on Python 3.11).

GN args of note: `v8_monolithic`, `v8_use_external_startup_data=false`,
`v8_enable_i18n_support=false`, `v8_enable_inspector=false`, system clang,
`use_custom_libcxx=false`.

> **Windows:** build the same pinned 8.7 with V8's standard GN+Ninja (the source
> patches above are macOS/Clang-specific and not needed there). A
> `build-v8-8.7-windows` companion is TODO.

## Using V8 from the CMake build

```sh
cmake -S . -B build-cmake -G Ninja -DMINIBLINK_V8_ROOT=~/build/v8-8.7/v8
cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure   # v8_smoketest passes
```

The root `CMakeLists.txt` exposes an imported `v8_monolith` target (with the
correct pointer-compression ABI defines) that engine targets will link against.

## Roadmap

1. **[DONE]** CMake scaffold + platform detection.
2. **[DONE]** Build & verify native-arm64 **V8 8.7 monolith** on macOS; remove
   in-tree vendored V8; expose it as a CMake imported target with a smoke test.
3. **[TODO]** Build the same pinned V8 8.7 on Windows; wire both into one build.
4. **[TODO]** Migrate the engine's V8 usage from **7.5 → 8.7 API** (`gin/`,
   `wke/wkeJsBind.cpp`), building `gin` against `v8_monolith`.
5. **[TODO]** OS abstraction layer for the engine: windowing, paint surface,
   threading, fonts — Win32 today; add macOS (Cocoa/CoreText/Metal) behind it.
6. **[TODO]** Non-`HWND` public API path so `wke.h` is usable on macOS.
