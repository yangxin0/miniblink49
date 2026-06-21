// macOS implementation of the blink shared timer (sibling of SharedTimerWin.h).
//
// Mirrors the Windows behavior: drive content::WebThreadImpl::fire() on the main
// thread at the requested interval. Where Windows uses an off-screen timer window
// plus a background heartbeat thread, here we use a GCD dispatch-source timer on
// the main queue.

#include "config.h"

#include "content/web_impl_mac/SharedTimerMac.h"
#include "content/web_impl_win/WebThreadImpl.h"
#include "third_party/WebKit/public/platform/Platform.h"

#include <dispatch/dispatch.h>

namespace {

dispatch_source_t g_sharedTimer = nullptr;
double g_lastIntervalSeconds = -1.0;

void sharedTimerFiredFunction()
{
    static int s_enterCount = 0;
    if (0 != s_enterCount)
        return;
    ++s_enterCount;

    content::WebThreadImpl* threadImpl =
        static_cast<content::WebThreadImpl*>(blink::Platform::current()->currentThread());
    if (threadImpl)
        threadImpl->fire();

    --s_enterCount;
}

} // namespace

void setSharedTimerFireInterval(double intervalSeconds)
{
    if (intervalSeconds < 0.001)
        intervalSeconds = 0.001;

    // Same de-dup as the Windows path: ignore unchanged intervals.
    if (g_sharedTimer && g_lastIntervalSeconds == intervalSeconds)
        return;
    g_lastIntervalSeconds = intervalSeconds;

    if (!g_sharedTimer) {
        g_sharedTimer = dispatch_source_create(
            DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        dispatch_source_set_event_handler(g_sharedTimer, ^{
            sharedTimerFiredFunction();
        });
        dispatch_resume(g_sharedTimer);
    }

    uint64_t intervalNs = (uint64_t)(intervalSeconds * NSEC_PER_SEC);
    dispatch_source_set_timer(
        g_sharedTimer,
        dispatch_time(DISPATCH_TIME_NOW, intervalNs),
        intervalNs,
        intervalNs / 10 /* leeway */);
}

void stopSharedTimer()
{
    if (g_sharedTimer) {
        dispatch_source_cancel(g_sharedTimer);
        g_sharedTimer = nullptr;
    }
    g_lastIntervalSeconds = -1.0;
}
