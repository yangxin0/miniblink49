// macOS paint backend (part 1): SkBitmap -> CGImage. See header.
#include "port/mac/skia_cg_bridge.h"

#include "SkBitmap.h"

namespace miniblink {
namespace mac {

namespace {
void releaseDataProviderBytes(void* info, const void*, size_t) {
    sk_free(info);
}
} // namespace

CGImageRef CGImageFromSkBitmap(const SkBitmap& bitmap) {
    const int width = bitmap.width();
    const int height = bitmap.height();
    if (width <= 0 || height <= 0)
        return nullptr;
    if (bitmap.colorType() != kN32_SkColorType)
        return nullptr;

    const size_t rowBytes = bitmap.rowBytes();
    const size_t size = rowBytes * static_cast<size_t>(height);

    // Copy the pixels so the CGImage owns a stable buffer.
    void* pixels = sk_malloc_throw(size);
    {
        SkAutoLockPixels lock(const_cast<SkBitmap&>(bitmap));
        if (!bitmap.getPixels()) {
            sk_free(pixels);
            return nullptr;
        }
        memcpy(pixels, bitmap.getPixels(), size);
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider =
        CGDataProviderCreateWithData(pixels, pixels, size, &releaseDataProviderBytes);

    // This in-tree skia builds N32 as RGBA (byte order R,G,B,A in memory),
    // premultiplied. Match it: alpha-last + big-endian-32 (byte 0 = R).
    const CGBitmapInfo bitmapInfo =
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;

    CGImageRef image = CGImageCreate(
        width, height, /*bitsPerComponent=*/8, /*bitsPerPixel=*/32, rowBytes,
        colorSpace, bitmapInfo, provider,
        /*decode=*/nullptr, /*shouldInterpolate=*/false, kCGRenderingIntentDefault);

    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    return image;  // caller owns; releaseDataProviderBytes frees `pixels`
}

} // namespace mac
} // namespace miniblink
