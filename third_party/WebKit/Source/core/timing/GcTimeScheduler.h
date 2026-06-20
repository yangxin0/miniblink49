#ifndef GcTimeScheduler_h
#define GcTimeScheduler_h

#include "public/platform/Platform.h"
#if defined(_WIN32)
#include "vc6/include/wnet/xpapi.h"
#elif defined(__APPLE__)
#include <mach/mach.h>
#endif

namespace blink {

// ����ڴ����xxM���ͻ���һ�Ρ����´�����ڴ滹�Ǵ���xxM���Ǿ͵�һ��ʱ���ٻ��գ���ֹ��ͣ��Ϊ��ԭ�����
class GcTimeScheduler {
public:
    const double kNextFireIntervalSec = 60;

    GcTimeScheduler()
    {
        double time = Platform::current()->monotonicallyIncreasingTime();
        m_lastGcTime = time;
        m_lastQueryMemTime = time;
    }

    bool needGc()
    {
        double time = Platform::current()->monotonicallyIncreasingTime();
        if (needGcByQueryMem(time))
            return true;

        if (time - m_lastGcTime < kNextFireIntervalSec)
            return false;
        m_lastGcTime = time;
        return true;
    }

    void setNextFireInterval(double sec)
    {
        double time = Platform::current()->monotonicallyIncreasingTime();
        m_lastGcTime = time - kNextFireIntervalSec + sec;
    }

private:

    bool needGcByQueryMem(double time)
    {
        const double kNextFireQueryMemIntervalSec = 3;
        if (time - m_lastQueryMemTime < kNextFireQueryMemIntervalSec)
            return false;

        m_lastQueryMemTime = time;
#if defined(_WIN32)
        HANDLE handle = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS_EX pmc = { 0 };
        if (!GetProcessMemoryInfoXp(handle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
            return false;

        if (pmc.PeakPagefileUsage / (1024 * 1024) > 500)
            return true;

        return false;
#elif defined(__APPLE__)
        // macOS port of the Windows peak-memory check: query this task's physical
        // footprint via mach task_info and trigger a GC past the same ~500MB
        // threshold the Windows path uses.
        task_vm_info_data_t vmInfo;
        mach_msg_type_number_t infoCount = TASK_VM_INFO_COUNT;
        if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&vmInfo), &infoCount) != KERN_SUCCESS)
            return false;

        if (vmInfo.phys_footprint / (1024 * 1024) > 500)
            return true;

        return false;
#else
        return false;
#endif
    }

    double m_lastGcTime;
    double m_lastQueryMemTime;
};

}

#endif