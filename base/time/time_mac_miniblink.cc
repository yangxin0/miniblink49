// Self-contained macOS implementation of the base::Time / base::TimeTicks
// platform methods for the miniblink port.
//
// Chromium's base/time/time_mac.cc pulls base/mac/scoped_mach_port.h and a chain
// of base/mac RAII headers that miniblink never vendored. miniblink only needs
// Now()/Explode()/FromExploded()/TimeTicks::Now(), so this implements them
// directly against mach_absolute_time + gettimeofday + the libc calendar calls.
#include "base/time/time.h"

#include <mach/mach_time.h>
#include <sys/time.h>
#include <time.h>

namespace base {

// Microseconds between the Windows FILETIME epoch (1601-01-01) used by base::Time
// internally and the POSIX epoch (1970-01-01).
const int64 Time::kTimeTToMicrosecondsOffset = INT64_C(11644473600000000);

// static
Time Time::Now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  int64 us = static_cast<int64>(tv.tv_sec) * kMicrosecondsPerSecond + tv.tv_usec;
  return Time(us + kTimeTToMicrosecondsOffset);
}

// static
Time Time::NowFromSystemTime() {
  return Now();
}

void Time::Explode(bool is_local, Exploded* exploded) const {
  int64 unix_us = us_ - kTimeTToMicrosecondsOffset;
  time_t secs = static_cast<time_t>(unix_us / kMicrosecondsPerSecond);
  int64 us_remainder = unix_us % kMicrosecondsPerSecond;
  if (us_remainder < 0) {
    us_remainder += kMicrosecondsPerSecond;
    secs -= 1;
  }
  struct tm timestruct;
  if (is_local)
    localtime_r(&secs, &timestruct);
  else
    gmtime_r(&secs, &timestruct);

  exploded->year = timestruct.tm_year + 1900;
  exploded->month = timestruct.tm_mon + 1;
  exploded->day_of_week = timestruct.tm_wday;
  exploded->day_of_month = timestruct.tm_mday;
  exploded->hour = timestruct.tm_hour;
  exploded->minute = timestruct.tm_min;
  exploded->second = timestruct.tm_sec;
  exploded->millisecond = static_cast<int>(us_remainder / kMicrosecondsPerMillisecond);
}

// static
Time Time::FromExploded(bool is_local, const Exploded& exploded) {
  struct tm timestruct;
  timestruct.tm_year = exploded.year - 1900;
  timestruct.tm_mon = exploded.month - 1;
  timestruct.tm_mday = exploded.day_of_month;
  timestruct.tm_hour = exploded.hour;
  timestruct.tm_min = exploded.minute;
  timestruct.tm_sec = exploded.second;
  timestruct.tm_wday = exploded.day_of_week;
  timestruct.tm_yday = 0;
  timestruct.tm_isdst = -1;
  timestruct.tm_gmtoff = 0;
  timestruct.tm_zone = nullptr;

  time_t secs = is_local ? mktime(&timestruct) : timegm(&timestruct);
  int64 us = static_cast<int64>(secs) * kMicrosecondsPerSecond +
             static_cast<int64>(exploded.millisecond) * kMicrosecondsPerMillisecond;
  return Time(us + kTimeTToMicrosecondsOffset);
}

// static
TimeTicks TimeTicks::Now() {
  static mach_timebase_info_data_t timebase = {0, 0};
  if (timebase.denom == 0)
    mach_timebase_info(&timebase);
  uint64_t now = mach_absolute_time();
  // Convert mach ticks -> nanoseconds -> microseconds.
  int64 us = static_cast<int64>(
      (now * timebase.numer / timebase.denom) / 1000);
  return TimeTicks(us);
}

// static
TimeTicks TimeTicks::NowFromSystemTraceTime() {
  return Now();
}

}  // namespace base
