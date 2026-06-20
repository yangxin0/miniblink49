// Smoke test: link against blink_platform (geometry) and exercise the actual
// geometry algorithms. Proves blink's foundational geometry layer builds, links
// against WTF + skia, and computes correctly on macOS.
#include "platform/geometry/IntRect.h"
#include "platform/geometry/Region.h"
#include <cstdio>

using blink::IntRect;
using blink::Region;

int main() {
    // Rect intersection: (0,0,10,10) ∩ (5,5,10,10) = (5,5,5,5).
    IntRect a(0, 0, 10, 10);
    IntRect b(5, 5, 10, 10);
    IntRect c = a;
    c.intersect(b);
    const bool rectOk = (c == IntRect(5, 5, 5, 5)) && a.intersects(b) && a.contains(3, 3);

    // Region union: two adjacent 10x10 rects -> bounds 20 wide.
    Region r;
    r.unite(Region(IntRect(0, 0, 10, 10)));
    r.unite(Region(IntRect(10, 0, 10, 10)));
    const bool regionOk = (r.bounds() == IntRect(0, 0, 20, 10)) && r.contains(blink::IntPoint(15, 5));

    printf("rect=%s region=%s\n", rectOk ? "ok" : "BAD", regionOk ? "ok" : "BAD");
    const bool ok = rectOk && regionOk;
    printf("blink geometry smoke: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
