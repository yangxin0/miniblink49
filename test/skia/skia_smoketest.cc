// Smoke test: link against the macOS skia raster build and actually render.
// Draws an antialiased red rect onto a white bitmap and verifies the pixels,
// proving the skia 2D graphics library works end-to-end on this platform.
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "SkRect.h"
#include <cstdio>

int main() {
    SkBitmap bm;
    bm.allocN32Pixels(64, 64);

    SkCanvas canvas(bm);
    canvas.clear(SK_ColorWHITE);

    SkPaint paint;
    paint.setColor(SK_ColorRED);
    paint.setAntiAlias(true);
    canvas.drawRect(SkRect::MakeXYWH(8, 8, 48, 48), paint);

    const SkColor center = bm.getColor(32, 32);  // inside the rect -> red
    const SkColor corner = bm.getColor(1, 1);    // outside the rect -> white
    printf("center=%08X corner=%08X\n", center, corner);

    const bool ok = (center == SK_ColorRED) && (corner == SK_ColorWHITE);
    printf("skia raster smoke: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
