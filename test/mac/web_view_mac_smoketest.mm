// Smoke test for the macOS backend paint primitive (RenderSkBitmapToContext).
// Renders a two-tone SkBitmap into a CGBitmapContext and verifies the draw lands
// and is oriented upright (blink top-left -> CG bottom-left, single flip).
// Headless-safe: no AppKit/window server (the NSView/NSWindow code is compile-
// verified by linking the library).
#include "port/mac/web_view_mac.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkRect.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cstdio>
#include <cstdint>

int main() {
    const int W = 64, H = 64;
    // blink frame (top-left origin): top half red, bottom half white.
    SkBitmap bm;
    bm.allocN32Pixels(W, H);
    SkCanvas canvas(bm);
    canvas.clear(SK_ColorWHITE);
    SkPaint p; p.setColor(SK_ColorRED);
    canvas.drawRect(SkRect::MakeXYWH(0, 0, W, H / 2), p);  // top half red

    // Render into a fresh bottom-left CG context.
    uint32_t px[W * H] = {0};
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        px, W, H, 8, W * 4, cs,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);  // readback ARGB
    CGContextClearRect(ctx, CGRectMake(0, 0, W, H));
    miniblink::mac::RenderSkBitmapToContext(ctx, bm, CGRectMake(0, 0, W, H));

    // CGBitmapContext buffer row 0 = y=0 = bottom in CG. After the flip, blink's
    // top (red) lands at high y = high buffer-row index; blink's bottom (white)
    // at low row index.
    const uint32_t topVisual = px[(H - 1) * W + W / 2];  // visual top
    const uint32_t botVisual = px[0 * W + W / 2];        // visual bottom
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);

    const bool drew = (topVisual == 0xFFFF0000u);   // red present (draw works)
    const bool upright = (topVisual == 0xFFFF0000u) && (botVisual == 0xFFFFFFFFu);
    printf("topVisual=%08X botVisual=%08X drew=%s upright=%s\n",
           topVisual, botVisual, drew ? "ok" : "BAD", upright ? "ok" : "BAD");
    const bool ok = drew && upright;
    printf("macOS web-view paint smoke: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
