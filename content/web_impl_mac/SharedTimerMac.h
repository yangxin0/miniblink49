#ifndef content_web_impl_mac_SharedTimerMac_h
#define content_web_impl_mac_SharedTimerMac_h

// macOS sibling of content/browser/SharedTimerWin.h.
//
// On Windows the shared timer is driven by an off-screen timer window plus a
// background thread that PostMessage()s the UI thread every display frame, which
// ends up calling content::WebThreadImpl::fire() on the main thread.
//
// On macOS the same two entry points are provided here and implemented with a
// GCD dispatch-source timer scheduled on the main queue (see SharedTimerMac.mm),
// so blink's shared timer fires on the main thread just like on Windows.
//
// These two free functions are the only members of SharedTimerWin.h that
// WebThreadImpl.cpp actually references.

void setSharedTimerFireInterval(double intervalSeconds);
void stopSharedTimer();

#endif // content_web_impl_mac_SharedTimerMac_h
