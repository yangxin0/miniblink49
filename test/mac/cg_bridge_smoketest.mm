// Smoke test for the macOS paint backend's skia->CoreGraphics bridge.
// Renders an SkBitmap, wraps it in a CGImage, draws that CGImage into a fresh
// CGBitmapContext, and verifies the pixels round-trip. Headless-safe (no window).
#include "port/mac/skia_cg_bridge.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkRect.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cstdio>
#include <cstdint>

int main() {
    // 1. Render with skia: red rect on white.
    SkBitmap bm;
    bm.allocN32Pixels(64, 64);
    SkCanvas canvas(bm);
    canvas.clear(SK_ColorWHITE);
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    canvas.drawRect(SkRect::MakeXYWH(8, 8, 48, 48), paint);

    // 2. skia -> CGImage via the bridge.
    CGImageRef image = miniblink::mac::CGImageFromSkBitmap(bm);
    if (!image) { printf("cg bridge: FAIL (null image)\n"); return 1; }
    const bool dims = CGImageGetWidth(image) == 64 && CGImageGetHeight(image) == 64;

    // 3. Draw the CGImage into a fresh ARGB context and read pixels back.
    uint32_t px[64 * 64] = {0};
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        px, 64, 64, 8, 64 * 4, cs,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGContextDrawImage(ctx, CGRectMake(0, 0, 64, 64), image);

    // Row 0 in CG is the bottom; (32,32) is inside the rect either way.
    const uint32_t center = px[32 * 64 + 32];  // BGRA little -> 0xAARRGGBB in mem
    const uint32_t corner = px[1 * 64 + 1];
    // Premultiplied opaque red = 0xFFFF0000, white = 0xFFFFFFFF (ARGB).
    const bool colors = (center == 0xFFFF0000u) && (corner == 0xFFFFFFFFu);

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
    CGImageRelease(image);

    printf("dims=%s colors=%s (center=%08X corner=%08X)\n",
           dims ? "ok" : "BAD", colors ? "ok" : "BAD", center, corner);
    const bool ok = dims && colors;
    printf("skia->CoreGraphics bridge smoke: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
