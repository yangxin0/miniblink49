// macOS shim for <shlwapi.h>. The wke download/path helpers (wke.cpp, wke2.cpp,
// wkeSimpleDownload.h) include this Win32 path-utility header. On macOS the real
// file-dialog / save-path flow goes through the Cocoa layer (port/mac/*), so these
// path helpers are inert stubs purely so the includes resolve and the archive links.
//
// Windows builds use the real <shlwapi.h>; this file is only on the include path for
// the non-MSVC (macOS) build, so it never shadows the SDK header on Windows.
#pragma once

#include "win_compat/windows.h"  // BOOL, LPWSTR/LPCWSTR, etc.

#ifndef MINIBLINK_WIN_COMPAT_SHLWAPI_H_
#define MINIBLINK_WIN_COMPAT_SHLWAPI_H_

// Inert path helpers. Templated on the char type so callers passing wchar_t* (32-bit on
// macOS) or WCHAR* (16-bit) both bind; the buffer they passed in is left untouched.
template<typename T, typename U> static inline BOOL PathAppendW(T* /*pszPath*/, const U* /*pszMore*/) { return FALSE; }
template<typename T> static inline BOOL PathIsDirectoryW(const T* /*pszPath*/) { return FALSE; }

#endif // MINIBLINK_WIN_COMPAT_SHLWAPI_H_
