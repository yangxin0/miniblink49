// Smoke test: bring up the Oilpan garbage collector on macOS and prove it
// actually runs — allocate finalizable GC objects, drop all references, force a
// collection, and verify the finalizers ran (objects were reclaimed). This
// exercises the heap end-to-end: ThreadState init, allocation, conservative
// stack scanning (pushAllRegisters), marking, sweeping, finalization.
#include "config.h"
#include "platform/EventTracer.h"
#include "platform/heap/Heap.h"
#include "platform/heap/Handle.h"
#include "platform/heap/ThreadState.h"
#include "public/platform/Platform.h"
#include "wtf/CryptographicallyRandomNumber.h"
#include "wtf/MainThread.h"
#include "wtf/WTF.h"
#include <cstdio>
#include <cstring>

using namespace blink;

namespace {

// Minimal Platform: Oilpan only needs a few hooks; the rest default to no-ops.
class StubPlatform : public Platform {
public:
    void cryptographicallyRandomValues(unsigned char* buf, size_t n) override {
        for (size_t i = 0; i < n; ++i) buf[i] = 0;
    }
    double monotonicallyIncreasingTime() override { return ++m_clock; }
    double currentTime() override { return m_clock; }
private:
    double m_clock = 0.0;
};

double zeroTime() { return 0.0; }
void zeroRandom(unsigned char* buf, size_t len) { std::memset(buf, 0, len); }

int g_liveCount = 0;

// A finalizable garbage-collected object that tracks how many are live.
class TestObject : public GarbageCollectedFinalized<TestObject> {
public:
    TestObject() { ++g_liveCount; }
    ~TestObject() { --g_liveCount; }
    DEFINE_INLINE_TRACE() {}
};

} // namespace

int main() {
    // Canonical blink bring-up sequence (mirrors RunAllTests.cpp): WTF threading
    // + main thread, then a Platform, then the heap.
    WTF::setRandomSource(zeroRandom);
    WTF::initialize(zeroTime, nullptr, nullptr, nullptr, nullptr);
    WTF::initializeMainThread(0);

    static StubPlatform platform;
    Platform::initialize(&platform);

    Heap::init();
    ThreadState::attachMainThread();
    EventTracer::initialize();

    // Allocate objects without keeping any references to them.
    const int kCount = 1000;
    for (int i = 0; i < kCount; ++i)
        new TestObject();
    printf("allocated %d objects, live=%d\n", kCount, g_liveCount);

    // Force a precise GC (no heap pointers on the stack) with a sweep.
    Heap::collectGarbage(ThreadState::NoHeapPointersOnStack,
                         ThreadState::GCWithSweep, Heap::ForcedGC);

    printf("after GC, live=%d\n", g_liveCount);
    const bool ok = (g_liveCount == 0);
    printf("blink oilpan GC smoke: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
