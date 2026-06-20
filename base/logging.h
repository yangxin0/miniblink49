// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

// Portable DebugBreak() for non-Windows (used by CHECK below). On Windows it
// comes from <windows.h>.
#if !defined(_WIN32) && !defined(DebugBreak)
#if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
#define DebugBreak() __builtin_debugtrap()
#else
#define DebugBreak() __builtin_trap()
#endif
#endif

#ifndef CHECK
#define CHECK(condition)  \
  do {                    \
    if ((!(condition))) { \
      DebugBreak();       \
    }                     \
  } while (0)
#endif

// The DCHECK macro is equivalent to CHECK except that it only
// generates code in debug builds.
#ifdef DEBUG
#  define DCHECK(condition)      CHECK(condition)
#else
#  ifndef DCHECK
#    define DCHECK(condition)      ((void) 0)
#  endif
#endif

// Comparison variants of DCHECK (miniblink's logging.h omitted these; base/
// and gin/ use them widely). Minimal forms built on DCHECK.
#ifndef DCHECK_EQ
#define DCHECK_EQ(a, b) DCHECK((a) == (b))
#define DCHECK_NE(a, b) DCHECK((a) != (b))
#define DCHECK_LE(a, b) DCHECK((a) <= (b))
#define DCHECK_LT(a, b) DCHECK((a) <  (b))
#define DCHECK_GE(a, b) DCHECK((a) >= (b))
#define DCHECK_GT(a, b) DCHECK((a) >  (b))
#endif

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

// Additional logging macros used by base/ that the trimmed logging.h omitted.
#ifndef NOTREACHED
#define NOTREACHED() DCHECK(false)
#endif
#ifndef NOTIMPLEMENTED
#define NOTIMPLEMENTED() ((void)0)
#endif
#ifndef PCHECK
#define PCHECK(condition) CHECK(condition)
#endif

#endif // BASE_LOGGING_H_