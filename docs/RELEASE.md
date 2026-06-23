# Cross-platform port — release log

Version history of the cross-platform (macOS Apple Silicon / arm64 + Windows x64)
CMake port of miniblink49. Each version is an annotated git tag (`macos-port-v*`).
Both platforms share one pinned V8 8.7; every platform change is build-guarded so
the other platform is unchanged.

---

## v1.2 — 2026-06-23 (`macos-port-v1.2`)

**The Windows x64 build comes up via CMake.** The entire engine now builds on
Windows (Release) and the standalone runner launches the full engine — bringing
the Windows path to parity with the macOS port through one shared codebase.

- **Whole engine builds on Windows** via CMake + clang-cl/MSVC: the pinned V8 8.7
  monolith (clang-cl 22, since the DEPS-pinned clang 12 can't compile against the
  VS2022 STL), base/gin/wtf/skia, the full blink stack (platform/core/web/
  generated/bindings/bindings_infra/modules), and net/mc/wke_globals/
  content_browser/wke — all in Release to match the monolith ABI.
- Bundled third-party deps built from in-tree sources (zlib/libxml2/libxslt/
  libcurl) + a minimal ICU + skia DirectWrite fonts + the Oilpan x64 heap-asm.
- **All four deliverables build**: `miniblink_static.lib` (merged engine),
  `miniblink.dll` (exports 312 `wke*` C-API functions), `minibrowser.exe`, and
  `wkexe.exe`. Both executables launch and initialize V8 + blink + the compositor
  (~20 threads) without crashing.
- Reproducible V8 monolith build: `tools/build-v8-8.7-windows.ps1` (counterpart of
  the macOS `build-v8-8.7-macos.sh`).
- Recurring Windows-port mechanics: `/FI config.h`, platform-aware source
  selection (mac→win), `/utf-8`, `WIN32_LEAN_AND_MEAN` + targeted `/FI` of trimmed
  Win32 headers, `OS_WIN` native types, `/WHOLEARCHIVE` (the `-force_load` analog),
  a few V8-8.7 API migrations on Windows-only code paths, and stubbing optional
  subsystems (OrigChrome, printing/pdfium, NPAPI stream loader).

All Windows changes are `WIN32`-guarded; the macOS path is unchanged.

## v1.1 — 2026-06-22 (`macos-port-v1.1`)

**Page JavaScript now executes; real sites run.** v1 rendered static HTML/CSS
only; v1.1 runs page JS through the `wke` C API.

Root-caused and fixed the V8-8.7 ↔ blink-53 mismatches that blocked all page JS:
- `@@toStringTag` duplicate template property → DOM constructors (Window) failed →
  no main-world context. **This was the keystone — fixing it let any page JS run.**
- `GetCompatibleReceiver` global-proxy holder lookup (`window.*` methods threw
  "Illegal invocation" → jQuery broke).
- `V8_REVERSE_JSARGS` argument-ABI mismatch (all multi-arg DOM constructors
  crashed; was masquerading as a release "0x2 heisenbug").
- GC `HandleScope` in `UnifiedHeapController`; non-fatal DCHECKs; empty-context
  guard in `deleteHiddenValue`; `<input checkbox/radio>` fallback-theme null-deref.

Results & features:
- **baidu.com** renders + runs jQuery fully; **map.baidu.com** (canvas map) works;
  **m.youtube.com / x.com** run their app JS (app shell / splash).
- **minibrowser**: back/forward/reload buttons + address bar; Retina-crisp rendering.
- **mb_demo**: macOS port of weolar/mb-demo's "Estart" launcher on the wke lib.
- VIP licensing removed (features free by default).
- Build + packaging scripts: `tools/build-macos.sh`, `tools/package-macos.sh`.
- Release SDK zip (merged static `libwke.a` + shared `libwke.dylib` + headers).

Links the pinned V8 8.7 **release** monolith.

## v1.0 — 2026-06-21 (`macos-port-v1`)

**First macOS build that renders real websites** (static). The full
Chromium-derived engine (blink core/platform/web/modules/bindings, V8 8.7, skia,
mc compositor, net, base, gin) + a Cocoa embedding host build and run on macOS
arm64 (boot gap 1376 → 0).

- Deliverables: `libwke.a` (static), `libwke.dylib` (dynamic), a mini-browser.
- Renders example.com (http+https) and the full baidu.com homepage;
  mouse/keyboard/scroll input wired; loads heavy SPAs without crashing
  (dynamic content bounded by the 2016 engine vintage — page JS not yet running).
- V8 shipped with `dcheck_always_on` to dodge a release wrapper-construct crash
  (later root-caused as the v1.1 `V8_REVERSE_JSARGS` fix, so v1.1 uses release V8).
