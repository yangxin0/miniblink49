# V8 7.5 → 8.7 API migration (engine compatibility)

The engine (`gin/`, `wke/wkeJsBind.cpp`, …) was written against the V8 ~7.5 C++
API. The cross-platform build links **V8 8.7.220.3**. Between those versions V8
deprecated/removed several APIs (the "context-and-Maybe" migration). This file
catalogs every delta found in the codebase and the fix pattern, so the migration
is mechanical and consistent.

Status: `gin/converter.cc` fully migrated and compiles clean against V8 8.7
(syntax-checked). Remaining files tracked below.

## Verification probe (per file, no full build needed)

```sh
V8INC=~/build/v8-8.7/v8/include
clang++ -fsyntax-only -std=c++14 \
  -DV8_COMPRESS_POINTERS -DV8_31BIT_SMIS_ON_64BIT_ARCH -DOS_MACOSX -DOS_POSIX \
  "-DDebugBreak()=abort()" \
  -I orig_chrome -I . -I "$V8INC" <file.cc>
```
(`gin` builds against the full `orig_chrome/base`, not the trimmed `base/`.
`DebugBreak` is engine logging, not V8 — defined by base in the real build.)

## Delta patterns (before → after)

| # | V8 7.5 (old) | V8 8.7 (new) | Notes |
|---|--------------|--------------|-------|
| 1 | `val->BooleanValue(context)` → `Maybe<bool>` | `val->BooleanValue(isolate)` → `bool` | takes Isolate, returns plain bool |
| 2 | `str->Utf8Length()` | `str->Utf8Length(isolate)` | Isolate arg added |
| 3 | `str->WriteUtf8(buf, …)` | `str->WriteUtf8(isolate, buf, …)` | Isolate arg first |
| 4 | `obj->Set(key, val)` | `obj->Set(context, key, val)` → `Maybe<bool>` | Context required |
| 5 | `arr->Set(i, val)` | `arr->Set(context, i, val)` → `Maybe<bool>` | Context required |
| 6 | `obj->Get(key)` / `arr->Get(i)` | `->Get(context, key/i).ToLocalChecked()` | returns `MaybeLocal` |
| 7 | `val->ToInt32(isolate)` | `val->ToInt32(context).ToLocalChecked()` | also ToNumber/ToString/ToUint32/ToObject; returns `MaybeLocal` |
| 8 | `String::Utf8Value(val)` | `String::Utf8Value(isolate, val)` | Isolate arg added |

Unchanged (still valid in 8.7):
- `val->IntegerValue(context)` → `Maybe<int64_t>` (keep)
- `val->ToBoolean(isolate)` → `Local<Boolean>` (ToBoolean is the exception: takes
  Isolate, not Context)
- `val.As<Int32>()->Value()`, `Integer::New`, `Number::New`, `Boolean::New`,
  `String::NewFromUtf8(isolate, …, NewStringType, len).ToLocalChecked()`

## Files

V8-API-clean = all V8 7.5→8.7 deltas fixed; any remaining errors are base/CEF
header-resolution or the macOS `std::wstring`-vs-`string16` base issue (tracked
in BUILD_CROSSPLATFORM.md), NOT V8 incompatibilities.

- [x] `gin/converter.cc` — migrated; 0 errors vs V8 8.7.
- [x] `gin/arguments.cc` — already V8-8.7-compatible (0 V8 errors).
- [x] `gin/dictionary.cc` / `gin/dictionary.h` — V8-API-clean (deltas #4,#6,#7,#8
      + `FunctionTemplate::GetFunction(context)`, `ToBoolean(isolate)->Value()`).
- [x] `gin/try_catch.cc` — V8-API-clean (`StackTrace::GetFrame(isolate, i)`).
- [ ] `gin/function_template.cc`, `object_template_builder.cc`, `wrappable.cc` —
      V8 deltas not yet visible: blocked behind CEF header chains
      (`cef_callback`/`cef_bind`/`cef_atomicops`) + a CEF-vs-base `template_util`
      redefinition. Resolve via the base/CEF port, then re-probe.
- [ ] `gin/per_context_data.cc`, `runner.cc`, `shell_runner.cc` — blocked behind
      the trimmed `base/time/time.h` missing `time_internal::SaturatedAdd/Sub`
      (base port; `orig_chrome` has no `time.h`, pull from Chromium).
- [ ] `wke/wkeJsBind.cpp` and other `wke/` V8 users — not yet probed.
- [ ] `content/` V8 users (if any reachable).

### New delta found (add to table as #9/#10)
- `FunctionTemplate::GetFunction()` → `GetFunction(context).ToLocalChecked()` (MaybeLocal)
- `StackTrace::GetFrame(i)` → `GetFrame(isolate, i)`
- `val->ToBoolean()->BooleanValue()` → `val->ToBoolean(isolate)->Value()`

## Engine-vs-V8 note

Some errors during the probe are NOT V8-API issues but engine Win32 coupling
(e.g. `DebugBreak`, `<windows.h>` includes). Those are guarded `#if defined(_WIN32)`
and resolved by the base/platform port, tracked in BUILD_CROSSPLATFORM.md — they
are listed here only so they're not mistaken for V8 incompatibilities.
