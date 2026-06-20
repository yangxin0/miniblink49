/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_SpinLock_h
#define WTF_SpinLock_h

// DESCRIPTION
// spinLockLock() and spinLockUnlock() are simple spinlock primitives based on
// the standard CPU primitive of atomic increment and decrement of an int at
// a given memory address.

#include "wtf/Atomics.h"
#include <atomic>
#include <cstdint>

namespace WTF {

ALWAYS_INLINE void spinLockLock(int volatile* lock)
{
    while (UNLIKELY(atomicTestAndSetToOne(lock))) {
        while (*lock) { } // Spin without spamming locked instructions.
    }
}

ALWAYS_INLINE void spinLockUnlock(int volatile* lock)
{
    atomicSetOneToZero(lock);
}

class SpinLock {
public:
    SpinLock()
    {
        m_refCount = 0;
        m_owner = 0;
    }

    class Guard {
    public:
        explicit Guard(SpinLock& mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
        }

        ~Guard()
        {	
            m_mutex.unlock();
        }
    private:
        SpinLock& m_mutex;
    };

    // Portable per-thread id (avoids platform thread-id APIs): the address of a
    // thread_local object is unique per thread.
    static uintptr_t currentThreadId()
    {
        static thread_local char marker;
        return reinterpret_cast<uintptr_t>(&marker);
    }

    void lock()
    {
        uintptr_t owner = currentThreadId();
        for (;;) {
            uintptr_t expected = 0;
            if (m_owner.compare_exchange_weak(expected, owner))
                break;                       // acquired a free lock
            if (m_owner.load() == owner)
                break;                       // already owned by this thread (recursive)
        }
        m_refCount.fetch_add(1);
    }

    void unlock()
    {
        if (m_refCount.fetch_sub(1) == 1)    // was 1, now 0
            m_owner.store(0);
    }

private:
    std::atomic<int> m_refCount;
    std::atomic<uintptr_t> m_owner;
};

} // namespace WTF

using WTF::spinLockLock;
using WTF::spinLockUnlock;

#endif // WTF_SpinLock_h
