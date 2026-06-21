// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKIA_EXT_SKIA_UTILS_WIN_H_
#define SKIA_EXT_SKIA_UTILS_WIN_H_

#include "third_party/skia/include/core/SkColor.h"

struct SkIRect;
struct SkPoint;
struct SkRect;
// win_compat/windows.h already typedefs DWORD as uint32_t (32-bit, matching Win32);
// on macOS arm64 'unsigned long' is 64-bit, so redefining here would conflict. Only
// declare these when the macOS win_compat shim is NOT in scope (real Windows build).
#ifndef MINIBLINK_WIN_COMPAT_WINDOWS_H_
typedef unsigned long DWORD;
typedef DWORD COLORREF;
#endif
typedef struct tagPOINT POINT;
typedef struct tagRECT RECT;

namespace skia {

// Converts a Skia point to a Windows POINT.
POINT SkPointToPOINT(const SkPoint& point);

// Converts a Windows RECT to a Skia rect.
SkRect RECTToSkRect(const RECT& rect);

// Converts a Windows RECT to a Skia rect.
// Both use same in-memory format. Verified by COMPILE_ASSERT() in
// skia_utils.cc.
inline const SkIRect& RECTToSkIRect(const RECT& rect) {
  return reinterpret_cast<const SkIRect&>(rect);
}

// Converts a Skia rect to a Windows RECT.
// Both use same in-memory format. Verified by COMPILE_ASSERT() in
// skia_utils.cc.
inline const RECT& SkIRectToRECT(const SkIRect& rect) {
  return reinterpret_cast<const RECT&>(rect);
}

// Converts COLORREFs (0BGR) to the ARGB layout Skia expects.
SK_API SkColor COLORREFToSkColor(COLORREF color);

// Converts ARGB to COLORREFs (0BGR).
SK_API COLORREF SkColorToCOLORREF(SkColor color);

}  // namespace skia

#endif  // SKIA_EXT_SKIA_UTILS_WIN_H_

