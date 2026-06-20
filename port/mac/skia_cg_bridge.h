// macOS paint backend (part 1): bridge skia's raster output to CoreGraphics.
//
// blink/skia render into an SkBitmap (N32, premultiplied). To get those pixels
// onto a Cocoa surface (NSView/CALayer) we wrap them in a CGImage. This is the
// foundational piece of the macOS windowing backend's paint path; the NSView
// subclass (part 2) draws the CGImage produced here.
#ifndef MINIBLINK_PORT_MAC_SKIA_CG_BRIDGE_H_
#define MINIBLINK_PORT_MAC_SKIA_CG_BRIDGE_H_

#include <CoreGraphics/CoreGraphics.h>

class SkBitmap;

namespace miniblink {
namespace mac {

// Wrap an N32 (BGRA, premultiplied) SkBitmap's pixels in a CGImage. The returned
// image is retained (caller releases via CGImageRelease); it copies the pixels so
// it stays valid after the bitmap changes. Returns nullptr if the bitmap is empty
// or not 32-bit.
CGImageRef CGImageFromSkBitmap(const SkBitmap& bitmap);

} // namespace mac
} // namespace miniblink

#endif // MINIBLINK_PORT_MAC_SKIA_CG_BRIDGE_H_
