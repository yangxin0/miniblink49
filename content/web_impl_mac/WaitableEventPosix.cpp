// WaitableEventPosix.cpp — POSIX/macOS implementation of content::WaitableEvent.
//
// content/web_impl_win/WaitableEvent.h takes its OS_POSIX branch on macOS: a
// reference-counted WaitableEventKernel (base::Lock + std::list<Waiter*>) rather
// than a Win32 HANDLE. The Win32 body lives in WaitableEventWin.cpp (guarded out
// here). This file provides the POSIX wait-list implementation, ported from
// Chromium's base/synchronization/waitable_event_posix.cc and adapted to this
// class's lowercase method names (signal/wait/reset/isSignaled/timedWait/
// waitMany). It backs blink's createWaitableEvent / waitMultipleEvents on macOS.
//
// Windows is unaffected: this entire file compiles to nothing on _WIN32.

#if !defined(_WIN32)

#include "content/web_impl_win/WaitableEvent.h"

#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"

#include <algorithm>
#include <vector>

namespace content {

// -----------------------------------------------------------------------------
// Construction / destruction
// -----------------------------------------------------------------------------
WaitableEvent::WaitableEvent(bool manualReset, bool initiallySignaled)
    : kernel_(new WaitableEventKernel(manualReset, initiallySignaled))
{
}

WaitableEvent::~WaitableEvent()
{
}

void WaitableEvent::reset()
{
    base::AutoLock locked(kernel_->lock_);
    kernel_->signaled_ = false;
}

void WaitableEvent::signal()
{
    base::AutoLock locked(kernel_->lock_);

    if (kernel_->signaled_)
        return;

    if (kernel_->manual_reset_) {
        SignalAll();
        kernel_->signaled_ = true;
    } else {
        // Auto-reset: if no waiter was woken, we remain signaled.
        if (!SignalOne())
            kernel_->signaled_ = true;
    }
}

bool WaitableEvent::isSignaled()
{
    base::AutoLock locked(kernel_->lock_);

    const bool result = kernel_->signaled_;
    if (result && !kernel_->manual_reset_)
        kernel_->signaled_ = false;
    return result;
}

// -----------------------------------------------------------------------------
// Synchronous waiter: a stack-allocated Waiter that blocks on its own condition
// variable until fired.
// -----------------------------------------------------------------------------
namespace {

class SyncWaiter : public WaitableEvent::Waiter {
public:
    SyncWaiter()
        : fired_(false)
        , signaling_event_(nullptr)
        , lock_()
        , cv_(&lock_)
    {
    }

    bool fire(WaitableEvent* signaling_event) override
    {
        base::AutoLock locked(lock_);

        if (fired_)
            return false;

        fired_ = true;
        signaling_event_ = signaling_event;
        cv_.Broadcast();
        return true;
    }

    WaitableEvent* signaling_event() const { return signaling_event_; }

    // Stack-allocated; the ABA tag is just the object pointer.
    bool compare(void* tag) override { return this == tag; }

    bool fired() const { return fired_; }

    // Prevent an auto-reset event from thinking it signaled us after we've
    // decided to stop waiting (called with lock held).
    void disable() { fired_ = true; }

    base::Lock* lock() { return &lock_; }
    base::ConditionVariable* cv() { return &cv_; }

private:
    bool fired_;
    WaitableEvent* signaling_event_;
    base::Lock lock_;
    base::ConditionVariable cv_;
};

} // namespace

void WaitableEvent::wait()
{
    // Infinite wait: maxTime == INFINITE (the Win32 sentinel). timedWait
    // interprets INFINITE as no deadline.
    timedWait(0xFFFFFFFF /*INFINITE*/);
}

bool WaitableEvent::timedWait(DWORD maxTime)
{
    const bool finite_time = (maxTime != 0xFFFFFFFF /*INFINITE*/);
    const base::TimeTicks end_time = base::TimeTicks::Now()
        + base::TimeDelta::FromMilliseconds(static_cast<int64>(maxTime));

    kernel_->lock_.Acquire();
    if (kernel_->signaled_) {
        if (!kernel_->manual_reset_) {
            // Signaled with no waiters; auto-reset now that we waited.
            kernel_->signaled_ = false;
        }
        kernel_->lock_.Release();
        return true;
    }

    SyncWaiter sw;
    sw.lock()->Acquire();

    Enqueue(&sw);
    kernel_->lock_.Release();
    // We intentionally hold the SyncWaiter lock but not the kernel lock here;
    // this is safe because we never re-lock the kernel lock before releasing.

    for (;;) {
        const base::TimeTicks current_time = base::TimeTicks::Now();

        if (sw.fired() || (finite_time && current_time >= end_time)) {
            const bool return_value = sw.fired();

            sw.disable();
            sw.lock()->Release();

            kernel_->lock_.Acquire();
            kernel_->Dequeue(&sw, &sw);
            kernel_->lock_.Release();

            return return_value;
        }

        if (finite_time) {
            const base::TimeDelta max_wait = end_time - current_time;
            sw.cv()->TimedWait(max_wait);
        } else {
            sw.cv()->Wait();
        }
    }
}

// -----------------------------------------------------------------------------
// Synchronous wait on multiple events.
// -----------------------------------------------------------------------------
namespace {
bool cmp_fst_addr(const std::pair<WaitableEvent*, size_t>& a,
    const std::pair<WaitableEvent*, size_t>& b)
{
    return a.first < b.first;
}
} // namespace

// static
size_t WaitableEvent::waitMany(WaitableEvent** rawWaitables, size_t count)
{
    // Acquire locks in a globally consistent (address) order. Keep the original
    // index alongside each event so we can map back after firing.
    std::vector<std::pair<WaitableEvent*, size_t>> waitables;
    waitables.reserve(count);
    for (size_t i = 0; i < count; ++i)
        waitables.push_back(std::make_pair(rawWaitables[i], i));

    std::sort(waitables.begin(), waitables.end(), cmp_fst_addr);

    SyncWaiter sw;

    const size_t r = EnqueueMany(&waitables[0], count, &sw);
    if (r) {
        // One event was already signaled; EnqueueMany returns the count of
        // remaining waitables at that point, so the signaled index is count - r.
        return waitables[count - r].second;
    }

    // We now hold all the kernel locks and have enqueued our waiter everywhere.
    sw.lock()->Acquire();
    for (size_t i = 0; i < count; ++i)
        waitables[count - (1 + i)].first->kernel_->lock_.Release();

    for (;;) {
        if (sw.fired())
            break;
        sw.cv()->Wait();
    }
    sw.lock()->Release();

    WaitableEvent* const signaled_event = sw.signaling_event();
    size_t signaled_index = 0;

    for (size_t i = 0; i < count; ++i) {
        if (rawWaitables[i] != signaled_event) {
            rawWaitables[i]->kernel_->lock_.Acquire();
            rawWaitables[i]->kernel_->Dequeue(&sw, &sw);
            rawWaitables[i]->kernel_->lock_.Release();
        } else {
            // Taking this lock ensures signal() has completed before we return.
            rawWaitables[i]->kernel_->lock_.Acquire();
            rawWaitables[i]->kernel_->lock_.Release();
            signaled_index = i;
        }
    }

    return signaled_index;
}

// static
size_t WaitableEvent::EnqueueMany(WaiterAndIndex* waitables, size_t count, Waiter* waiter)
{
    if (!count)
        return 0;

    waitables[0].first->kernel_->lock_.Acquire();
    if (waitables[0].first->kernel_->signaled_) {
        if (!waitables[0].first->kernel_->manual_reset_)
            waitables[0].first->kernel_->signaled_ = false;
        waitables[0].first->kernel_->lock_.Release();
        return count;
    }

    const size_t r = EnqueueMany(waitables + 1, count - 1, waiter);
    if (r)
        waitables[0].first->kernel_->lock_.Release();
    else
        waitables[0].first->Enqueue(waiter);

    return r;
}

// -----------------------------------------------------------------------------
// Kernel
// -----------------------------------------------------------------------------
WaitableEvent::WaitableEventKernel::WaitableEventKernel(bool manual_reset, bool initially_signaled)
    : manual_reset_(manual_reset)
    , signaled_(initially_signaled)
{
}

WaitableEvent::WaitableEventKernel::~WaitableEventKernel()
{
}

// Called with the kernel lock held.
bool WaitableEvent::SignalAll()
{
    bool signaled_at_least_one = false;
    for (std::list<Waiter*>::iterator i = kernel_->waiters_.begin();
         i != kernel_->waiters_.end(); ++i) {
        if ((*i)->fire(this))
            signaled_at_least_one = true;
    }
    kernel_->waiters_.clear();
    return signaled_at_least_one;
}

// Called with the kernel lock held.
bool WaitableEvent::SignalOne()
{
    for (;;) {
        if (kernel_->waiters_.empty())
            return false;

        const bool r = (*kernel_->waiters_.begin())->fire(this);
        kernel_->waiters_.pop_front();
        if (r)
            return true;
    }
}

// Called with the kernel lock held.
void WaitableEvent::Enqueue(Waiter* waiter)
{
    kernel_->waiters_.push_back(waiter);
}

// Called with the kernel lock held.
bool WaitableEvent::WaitableEventKernel::Dequeue(Waiter* waiter, void* tag)
{
    for (std::list<Waiter*>::iterator i = waiters_.begin();
         i != waiters_.end(); ++i) {
        if (*i == waiter && (*i)->compare(tag)) {
            waiters_.erase(i);
            return true;
        }
    }
    return false;
}

} // namespace content

#endif // !defined(_WIN32)
