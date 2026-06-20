#include "base/thread.h"

#if defined(_WIN32)
#include <windows.h>

typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // must be 0x1000
    LPCSTR szName; // pointer to name (in user addr space)
    DWORD dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
#else
#include <pthread.h>
#endif

namespace base {

void SetThreadName(const char* szThreadName) {

#if defined(_WIN32)
#if ENABLE_NOT_MEM_LOAD // ��dllͨ���ڴ���ص�ʱ�����������쳣����������
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = szThreadName;
    info.dwThreadID = ::GetCurrentThreadId();
    info.dwFlags = 0;

    __try {
        ::RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
#endif
#elif defined(__APPLE__)
    // macOS: pthread_setname_np sets the name of the calling thread only.
    pthread_setname_np(szThreadName);
#else
    pthread_setname_np(pthread_self(), szThreadName);
#endif
}

}
