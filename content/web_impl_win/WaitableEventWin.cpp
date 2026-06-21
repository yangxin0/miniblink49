// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/web_impl_win/WaitableEvent.h"

// This is the Win32-event-based implementation of content::WaitableEvent.
// It relies on the OS_WIN member layout (m_handle, the HANDLE ctor) declared
// in WaitableEvent.h. On macOS/POSIX that header takes its OS_POSIX branch
// (a reference-counted, lock+condition-variable kernel that does not declare
// m_handle), so this entire Win32 body must be excluded there. The POSIX
// behavior is provided by the in-tree sibling
// orig_chrome/base/synchronization/waitable_event_posix.cc; keeping this file
// as an empty translation unit on non-Windows mirrors the Windows build
// without duplicating that implementation.
#if defined(_WIN32)

#include <windows.h>

namespace content {

WaitableEvent::WaitableEvent(bool manualReset, bool signaled)
    : m_handle(CreateEvent(NULL, manualReset, signaled, NULL))
{
    // We're probably going to crash anyways if this is ever NULL, so we might as
    // well make our stack reports more informative by crashing here.
    ASSERT(m_handle != INVALID_HANDLE_VALUE);
}

WaitableEvent::WaitableEvent(HANDLE handle)
    : m_handle(handle)
{
    ASSERT(m_handle == INVALID_HANDLE_VALUE); // << "Tried to create WaitableEvent from NULL handle";
}

WaitableEvent::~WaitableEvent()
{
    ::CloseHandle(m_handle);
}

void WaitableEvent::reset()
{
    ::ResetEvent(m_handle);
}

void WaitableEvent::signal()
{
    ::SetEvent(m_handle);
}

bool WaitableEvent::isSignaled()
{
    return timedWait(0);
}

void WaitableEvent::wait()
{
    DWORD result = WaitForSingleObject(m_handle, INFINITE);
    // It is most unexpected that this should ever fail.  Help consumers learn
    // about it if it should ever fail.
    ASSERT(WAIT_OBJECT_0 == result); // << "WaitForSingleObject failed";
}

bool WaitableEvent::timedWait(DWORD maxTime)
{
    ASSERT(maxTime >= 0);
    // Truncate the timeout to milliseconds. The API specifies that this method
    // can return in less than |max_time| (when returning false), as the argument
    // is the maximum time that a caller is willing to wait.
    DWORD timeout = maxTime;

    DWORD result = ::WaitForSingleObject(m_handle, timeout);
    switch (result) {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        return false;
    }
    // It is most unexpected that this should ever fail.  Help consumers learn
    // about it if it should ever fail.
    ASSERT(false); // << "WaitForSingleObject failed";
    return false;
}

// static
size_t WaitableEvent::waitMany(WaitableEvent** events, size_t count)
{
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    ASSERT(count <= static_cast<size_t>(MAXIMUM_WAIT_OBJECTS)); // << "Can only wait on " << MAXIMUM_WAIT_OBJECTS << " with WaitMany";

    for (size_t i = 0; i < count; ++i)
        handles[i] = events[i]->handle();

    // The cast is safe because count is small - see the CHECK above.
    DWORD result =
        WaitForMultipleObjects(static_cast<DWORD>(count),
            handles,
            FALSE,      // don't wait for all the objects
            INFINITE);  // no timeout
    if (result >= WAIT_OBJECT_0 + count) {
        ASSERT(false); // << "WaitForMultipleObjects failed";
        return 0;
    }

    return result - WAIT_OBJECT_0;
}

}  // namespace content

#endif  // defined(_WIN32)
