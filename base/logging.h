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
// Portable OutputDebugStringA() for non-Windows (engine debug logging uses it).
#if !defined(_WIN32) && !defined(OutputDebugStringA)
#include <cstdio>
#define OutputDebugStringA(s) std::fputs((s), stderr)
#endif

// A no-op stream that swallows any operator<< (so CHECK/DCHECK can be followed
// by " << message" the way Chromium/orig_chrome code expects). LogVoidify has
// lower precedence than << so `LogVoidify() & NullStream() << x` is void.
#ifndef BASE_LOGGING_NULLSTREAM_DEFINED
#define BASE_LOGGING_NULLSTREAM_DEFINED
namespace base { namespace logging_internal {
class NullStream {
 public:
  template <typename T> NullStream& operator<<(const T&) { return *this; }
};
class LogVoidify { public: void operator&(const NullStream&) {} };
inline NullStream& nullStream() { static NullStream s; return s; }
}}  // namespace base::logging_internal
#endif

// CHECK: DebugBreak on failure; supports trailing "<< message". Single
// expression -> safe in if/else without braces.
#ifndef CHECK
#define CHECK(condition)                                                       \
  !(condition) ? (DebugBreak(), ::base::logging_internal::LogVoidify() &        \
                                    ::base::logging_internal::nullStream())     \
               : ::base::logging_internal::LogVoidify() &                       \
                     ::base::logging_internal::nullStream()
#endif

// DCHECK: no-op in this port (release-style), supports "<< message". It must NOT
// evaluate the condition (the original stub was ((void)0)); some existing DCHECK
// call sites pass expressions that don't type-check on macOS (e.g. IsStringASCII
// of a string16) and were only ever compiled with a no-op DCHECK.
#ifndef DCHECK
#define DCHECK(condition)                                                      \
  true ? (void)0                                                               \
       : ::base::logging_internal::LogVoidify() &                              \
             ::base::logging_internal::nullStream()
#endif

// LOG/DLOG/VLOG: no-op streamable sinks (the severity token is never looked up,
// so LOG(ERROR) etc. compile without ERROR/INFO/... being declared).
#ifndef LOG
#define LOG(severity) \
  true ? (void)0 : ::base::logging_internal::LogVoidify() & ::base::logging_internal::nullStream()
#endif
#ifndef DLOG
#define DLOG(severity) LOG(severity)
#endif
#ifndef VLOG
#define VLOG(verboselevel) LOG(INFO)
#endif
#ifndef LOG_IF
#define LOG_IF(severity, condition) LOG(severity)
#endif

// Comparison variants of DCHECK, also streamable.
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