// platform_thread_yield_posix.cc — base::PlatformThread::YieldCurrentThread.
//
// Pulled in as a dependency of base/lazy_instance.cc, which spins on
// YieldCurrentThread while another thread completes a lazy initialization.
// Chromium implements this in base/threading/platform_thread_posix.cc, but that
// file drags in the full posix thread-creation machinery (pthread attrs, thread
// priorities, the Thread delegate, etc.). YieldCurrentThread itself is a one-
// liner over sched_yield(), so we provide just it here to avoid that cascade.
// Posix/mac only; Windows uses platform_thread_win.cc.

#if !defined(_WIN32)

#include "base/threading/platform_thread.h"

#include <sched.h>

namespace base {

// static
void PlatformThread::YieldCurrentThread()
{
    sched_yield();
}

} // namespace base

#endif // !defined(_WIN32)
