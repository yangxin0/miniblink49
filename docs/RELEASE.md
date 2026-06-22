# macOS port — release log

Version history of the macOS (Apple Silicon / arm64) port of miniblink49. Each
version is an annotated git tag (`macos-port-v*`). The port shares one pinned
V8 8.7 with the Windows build; all macOS changes are version-guarded so the
Windows path is unchanged.

---

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
