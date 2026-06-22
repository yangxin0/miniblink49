# miniblink49 — cross-platform fork (macOS arm64 + Windows)

A fork of [weolar/miniblink49](https://github.com/weolar/miniblink49) — the small,
single-file browser widget based on Chromium/Blink — ported to build and **run on
macOS (Apple Silicon)** alongside the original Windows build, on **one shared,
pinned V8 8.7**.

On macOS the engine now executes page JavaScript and renders real sites (e.g.
baidu fully; YouTube/X app shells) through a Cocoa mini-browser that drives the
`wke` C API. The Windows `mb`/`wke` API is preserved; all macOS changes are
version-guarded so the Windows path is unchanged.

> New here? `wke/wke.h` is the public C API. The macOS host lives in
> `port/mac/minibrowser_main.mm`. Build details below.

---

## Build on macOS (Apple Silicon / arm64)

### Prerequisites

- macOS on Apple Silicon (arm64). x64 is supported via `TARGET_CPU=x64`.
- **Xcode Command Line Tools** (`xcode-select --install`) — full Xcode is *not* required.
- **CMake ≥ 3.16** (`brew install cmake`).
- ~30 GB free disk and a network connection (the V8 build fetches depot_tools + V8 source).

### Step 1 — Build the pinned V8 8.7 monolith

The JS engine is a single pinned **V8 8.7.220.3**, built from source once. The
script bootstraps depot_tools, applies the macOS/Clang patches (`v8_8_7_*.patch`),
and builds a static monolith:

```sh
tools/build-v8-8.7-macos.sh                  # native arm64 (default)
# TARGET_CPU=x64 tools/build-v8-8.7-macos.sh   # x64 instead
```

Output: `~/build/v8-8.7/v8/out/arm64.release/obj/libv8_monolith.a` (+ headers under
`~/build/v8-8.7/v8/include`). Re-running is idempotent.

### Step 2 — Configure & build the engine + mini-browser

```sh
cmake -S . -B build-mac
cmake --build build-mac --target minibrowser -j8
```

`CMakeLists.txt` finds V8 via `MINIBLINK_V8_ROOT` (default `~/build/v8-8.7/v8`)
and `MINIBLINK_V8_OUT` (default `out/<arch>.release`). Override if you built V8
elsewhere:

```sh
cmake -S . -B build-mac -DMINIBLINK_V8_ROOT=/path/to/v8-8.7/v8
```

### Step 3 — Run

```sh
build-mac/bin/minibrowser https://www.baidu.com
```

A Cocoa window opens with a toolbar (back / forward / reload + address bar) and
the rendered page. The address bar accepts a URL (bare hosts get `https://`
prepended) or a search query.

### Outputs

| Artifact | Path |
|----------|------|
| Mini-browser executable | `build-mac/bin/minibrowser` |
| Static `wke` C-API library | `build-mac/lib/libwke.a` |

> Tip: the V8 monolith is linked via `force_load`, a dependency CMake does not
> track. After rebuilding V8, run `rm -f build-mac/bin/minibrowser` before
> `cmake --build` to force a relink.

---

## Build on Windows

The original Visual Studio build is unchanged:

1. Open `build/miniblink.sln` in Visual Studio.
2. Build the desired configuration.

A cross-platform CMake path (sharing the same pinned V8 8.7) is also being wired
up — see [`BUILD_CROSSPLATFORM.md`](BUILD_CROSSPLATFORM.md). Build the matching
V8 8.7 with V8's standard GN + Ninja (the `v8_8_7_*.patch` files are
macOS/Clang-specific and not needed on Windows).

---

## Notes

- **One JS engine.** The six in-tree vendored V8 copies were removed; both OSes
  use the single pinned V8 8.7.220.3 — the first V8 with native Apple Silicon
  support, while staying close to the engine's existing V8 ~7.5 API. See
  [`V8_API_MIGRATION.md`](V8_API_MIGRATION.md).
- **macOS porting approach.** Changes are guarded with `#if defined(_WIN32)` /
  `#if V8_MAJOR_VERSION < 8`; a Win32 compatibility shim (`win_compat/`) and a
  Cocoa embedding host (`content/web_impl_mac/`, `port/mac/`) back the port. The
  `mc` compositor runs single-threaded on macOS.
- **Free & open source.** The VIP license-verification subsystem has been
  removed; all features are enabled by default.

## Credits

Engine and original Windows implementation by **weolar** —
<https://github.com/weolar/miniblink49> · <http://miniblink.net>. Please respect
the upstream author's years of work.
