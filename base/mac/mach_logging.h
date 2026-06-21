// Minimal base/mac/mach_logging.h for the macOS port.
//
// Chromium's full version formats mach_error_t / kern_return_t into the log
// stream. miniblink's trimmed base only needs the macros to exist and be
// stream-compatible no-ops/asserts (time_mac.cc uses MACH_DCHECK around
// mach_timebase_info / thread_info, which never fail in practice). Map them to
// the base logging macros so the `<< "msg"` stream still compiles.
#ifndef BASE_MAC_MACH_LOGGING_H_
#define BASE_MAC_MACH_LOGGING_H_

#include "base/logging.h"

#define MACH_LOG(severity, mach_err)   LOG(severity)
#define MACH_DLOG(severity, mach_err)  DLOG(severity)
#define MACH_CHECK(condition, mach_err) CHECK(condition)
#define MACH_DCHECK(condition, mach_err) DCHECK(condition)
#define MACH_VLOG(verbose_level, mach_err) DLOG(INFO)
#define MACH_PLOG(severity, mach_err)  LOG(severity)

#endif  // BASE_MAC_MACH_LOGGING_H_
