// default_platform_wrap.h — macOS/cross-platform shim for gin's V8Platform.
//
// miniblink's gin/v8_platform.cc (V8 7+ path) delegates worker-thread task
// scheduling, the worker-thread count, and the clock to a "DefaultPlatformWrap"
// that, on the original tree, wrapped a bundled v8_7_5 libplatform. That bundled
// header is not present here; instead we wrap V8 8.7's own default platform
// (v8::platform::NewDefaultPlatform from include/libplatform/libplatform.h),
// which exposes exactly the v8::Platform methods gin forwards. This keeps gin's
// V8Platform fully functional (real worker pool) without vendoring the old v8_7_5
// libplatform sources.

#ifndef GIN_SHIM_DEFAULT_PLATFORM_WRAP_H_
#define GIN_SHIM_DEFAULT_PLATFORM_WRAP_H_

#include <memory>
#include <utility>

#include "libplatform/libplatform.h"
#include "v8-platform.h"
#include "v8.h"

namespace gin {

class DefaultPlatformWrap {
public:
    DefaultPlatformWrap()
        : m_platform(v8::platform::NewDefaultPlatform())
    {
    }

    ~DefaultPlatformWrap() = default;

    void CallOnWorkerThread(std::unique_ptr<v8::Task> task)
    {
        m_platform->CallOnWorkerThread(std::move(task));
    }

    void CallDelayedOnWorkerThread(std::unique_ptr<v8::Task> task, double delay_in_seconds)
    {
        m_platform->CallDelayedOnWorkerThread(std::move(task), delay_in_seconds);
    }

    double CurrentClockTimeMillis()
    {
        return m_platform->CurrentClockTimeMillis();
    }

    int NumberOfWorkerThreads()
    {
        return m_platform->NumberOfWorkerThreads();
    }

#if V8_MAJOR_VERSION >= 8
    std::unique_ptr<v8::JobHandle> PostJob(
        v8::TaskPriority priority, std::unique_ptr<v8::JobTask> job_task)
    {
        return m_platform->PostJob(priority, std::move(job_task));
    }
#endif

    v8::Platform* platform() { return m_platform.get(); }

private:
    std::unique_ptr<v8::Platform> m_platform;
};

} // namespace gin

#endif // GIN_SHIM_DEFAULT_PLATFORM_WRAP_H_
