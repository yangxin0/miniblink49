// macOS backend (part 2): the NSView/NSWindow host that displays blink's output.
//
// The content view holds the latest rendered SkBitmap and blits it to the screen
// via the skia->CoreGraphics bridge. RenderSkBitmapToContext is the headless-
// testable core of the paint (drawRect: just forwards to it); the NSView/NSWindow
// wrappers wire it into Cocoa. Event translation (NSEvent -> blink input) lands
// here next.
#ifndef MINIBLINK_PORT_MAC_WEB_VIEW_MAC_H_
#define MINIBLINK_PORT_MAC_WEB_VIEW_MAC_H_

#include <CoreGraphics/CoreGraphics.h>

class SkBitmap;

namespace miniblink {
namespace mac {

// Draw an SkBitmap into a CoreGraphics context at dstRect, accounting for CG's
// bottom-left origin (the image is flipped so it appears upright). This is the
// paint primitive the content NSView uses; it needs no AppKit, so it is unit-
// testable headlessly.
void RenderSkBitmapToContext(CGContextRef context, const SkBitmap& bitmap, CGRect dstRect);

} // namespace mac
} // namespace miniblink

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>

// A view that displays the most recently rendered blink/skia frame. Call
// -setBitmap: with the new frame (it copies via the bridge) then -setNeedsDisplay.
@interface MbWebContentView : NSView
- (void)setBitmap:(const SkBitmap*)bitmap;
@end

namespace miniblink {
namespace mac {
// Create a titled window hosting an MbWebContentView. Returns the window (caller
// owns / shows it). The content view is the window's contentView.
NSWindow* CreateWebWindow(int x, int y, int width, int height, const char* title);
} // namespace mac
} // namespace miniblink

#endif // __OBJC__

#endif // MINIBLINK_PORT_MAC_WEB_VIEW_MAC_H_
