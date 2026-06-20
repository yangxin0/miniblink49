// macOS backend (part 2): NSView/NSWindow host + paint primitive. See header.
#include "port/mac/web_view_mac.h"
#include "port/mac/skia_cg_bridge.h"

#include "SkBitmap.h"

namespace miniblink {
namespace mac {

void RenderSkBitmapToContext(CGContextRef context, const SkBitmap& bitmap, CGRect dstRect) {
    if (!context)
        return;
    CGImageRef image = CGImageFromSkBitmap(bitmap);
    if (!image)
        return;
    // CoreGraphics has a bottom-left origin; flip vertically so the top-left
    // origin blink renders with appears upright.
    CGContextSaveGState(context);
    CGContextTranslateCTM(context, 0, dstRect.origin.y * 2 + dstRect.size.height);
    CGContextScaleCTM(context, 1, -1);
    CGContextDrawImage(context, dstRect, image);
    CGContextRestoreGState(context);
    CGImageRelease(image);
}

} // namespace mac
} // namespace miniblink

@implementation MbWebContentView {
    SkBitmap _bitmap;
    BOOL _hasBitmap;
}

- (void)setBitmap:(const SkBitmap*)bitmap {
    if (bitmap) {
        _bitmap = *bitmap;  // shares the underlying pixelref (skia is refcounted)
        _hasBitmap = YES;
    } else {
        _hasBitmap = NO;
    }
}

// Note: the view stays non-flipped (CG bottom-left origin); RenderSkBitmapToContext
// does the single vertical flip that maps blink's top-left frame upright.

- (void)drawRect:(NSRect)dirtyRect {
    if (!_hasBitmap)
        return;
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    miniblink::mac::RenderSkBitmapToContext(ctx, _bitmap, NSRectToCGRect(self.bounds));
}

@end

namespace miniblink {
namespace mac {

NSWindow* CreateWebWindow(int x, int y, int width, int height, const char* title) {
    NSRect frame = NSMakeRect(x, y, width, height);
    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                             NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    if (title)
        [window setTitle:[NSString stringWithUTF8String:title]];

    MbWebContentView* view = [[MbWebContentView alloc] initWithFrame:frame];
    [window setContentView:view];
    return window;
}

} // namespace mac
} // namespace miniblink
