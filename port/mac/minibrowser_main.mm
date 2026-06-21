// miniblink mini-browser for macOS — a minimal host that drives the wke C API.
//
// wke renders the page off-screen into a memory bitmap (the in-tree skia memory
// canvas, RGBA N32). This host:
//   1. wkeInitialize() + creates an off-screen wkeWebView,
//   2. hosts it in an NSWindow/NSView,
//   3. on wkeOnPaintUpdated marks the view dirty; drawRect: pulls the webview's
//      pixels via wkePaint() and blits them with CoreGraphics,
//   4. forwards mouse/resize, and runs the Cocoa run loop — blink's scheduler is
//      pumped by the GCD-backed SharedTimerMac, so timers/loading advance.
//
// Build: the `minibrowser` CMake target links the full engine archive set.
#import <Cocoa/Cocoa.h>
#include <vector>
#include "wke/wke.h"

// Win32 message + mouse-key-flag codes the wke input API expects (kept local so
// this Cocoa TU doesn't pull win_compat/windows.h).
enum {
    kWM_MOUSEMOVE = 0x0200, kWM_LBUTTONDOWN = 0x0201, kWM_LBUTTONUP = 0x0202,
    kWM_LBUTTONDBLCLK = 0x0203, kWM_RBUTTONDOWN = 0x0204, kWM_RBUTTONUP = 0x0205,
    kWM_MBUTTONDOWN = 0x0207, kWM_MBUTTONUP = 0x0208,
    kMK_LBUTTON = 0x0001, kMK_RBUTTON = 0x0002, kMK_SHIFT = 0x0004,
    kMK_CONTROL = 0x0008, kMK_MBUTTON = 0x0010,
    kVK_BACK = 0x08, kVK_TAB = 0x09, kVK_RETURN = 0x0D, kVK_ESCAPE = 0x1B,
    kVK_LEFT = 0x25, kVK_UP = 0x26, kVK_RIGHT = 0x27, kVK_DOWN = 0x28, kVK_DELETE = 0x2E,
};

static wkeWebView g_webView = nullptr;
static NSView* g_contentView = nil;

// ---- The content view: blits the wke webview's RGBA buffer ------------------
@interface MbBrowserView : NSView
@end

@implementation MbBrowserView
- (BOOL)isFlipped { return YES; }  // top-left origin, matching wke/blink

- (void)drawRect:(NSRect)dirtyRect {
    if (!g_webView)
        return;
    int w = (int)self.bounds.size.width;
    int h = (int)self.bounds.size.height;
    if (w <= 0 || h <= 0)
        return;

    // Pull the rendered page pixels (RGBA, w*4 pitch) from wke.
    int pitch = w * 4;
    static std::vector<unsigned char> buf;
    buf.assign((size_t)pitch * h, 0);
    static int dr = 0; ++dr;
    if (dr <= 3) NSLog(@"[minibrowser] drawRect %d: before wkePaint (w=%d h=%d)", dr, w, h);
    wkePaint(g_webView, buf.data(), pitch);
    if (dr <= 3) {
        size_t nz = 0; for (size_t i = 0; i < buf.size(); ++i) if (buf[i]) ++nz;
        NSLog(@"[minibrowser] drawRect %d: after wkePaint, nonzero bytes=%zu", dr, nz);
    }

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    CGContextRef bmp = CGBitmapContextCreate(buf.data(), w, h, 8, pitch, cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGImageRef img = CGBitmapContextCreateImage(bmp);
    // wke's buffer is top-down (row 0 = top). In this flipped NSView, draw the
    // image with a vertical flip so it appears upright instead of mirrored.
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 0, h);
    CGContextScaleCTM(ctx, 1, -1);
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), img);
    CGContextRestoreGState(ctx);
    CGImageRelease(img);
    CGContextRelease(bmp);
    CGColorSpaceRelease(cs);
}

// ---- Input: forward Cocoa events to wke -------------------------------------
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)becomeFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)e { return YES; }

- (void)updateTrackingAreas {
    for (NSTrackingArea* a in [self trackingAreas]) [self removeTrackingArea:a];
    NSTrackingArea* ta = [[NSTrackingArea alloc]
        initWithRect:[self bounds]
        options:(NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect)
        owner:self userInfo:nil];
    [self addTrackingArea:ta];
    [super updateTrackingAreas];
}

// Compute wke (x, y, flags) from an NSEvent. The view is flipped, so the
// converted point is already top-left origin (matching wke/blink).
- (void)wkePoint:(NSEvent*)e x:(int*)x y:(int*)y flags:(unsigned int*)flags {
    NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
    *x = (int)p.x; *y = (int)p.y;
    unsigned int f = 0;
    NSUInteger m = [e modifierFlags];
    if (m & NSEventModifierFlagShift)   f |= kMK_SHIFT;
    if (m & NSEventModifierFlagControl) f |= kMK_CONTROL;
    NSUInteger b = [NSEvent pressedMouseButtons];
    if (b & 1) f |= kMK_LBUTTON;
    if (b & 2) f |= kMK_RBUTTON;
    if (b & 4) f |= kMK_MBUTTON;
    *flags = f;
}
- (void)fireMouse:(unsigned int)msg event:(NSEvent*)e {
    if (!g_webView) return;
    int x, y; unsigned int flags; [self wkePoint:e x:&x y:&y flags:&flags];
    wkeFireMouseEvent(g_webView, msg, x, y, flags);
    [self setNeedsDisplay:YES];
}
- (void)mouseDown:(NSEvent*)e        { wkeSetFocus(g_webView); [self fireMouse:kWM_LBUTTONDOWN event:e]; }
- (void)mouseUp:(NSEvent*)e          { [self fireMouse:kWM_LBUTTONUP event:e]; }
- (void)mouseDragged:(NSEvent*)e     { [self fireMouse:kWM_MOUSEMOVE event:e]; }
- (void)mouseMoved:(NSEvent*)e       { [self fireMouse:kWM_MOUSEMOVE event:e]; }
- (void)rightMouseDown:(NSEvent*)e   {
    if (!g_webView) return;
    int x, y; unsigned int flags; [self wkePoint:e x:&x y:&y flags:&flags];
    wkeFireMouseEvent(g_webView, kWM_RBUTTONDOWN, x, y, flags);
    wkeFireContextMenuEvent(g_webView, x, y, flags);
    [self setNeedsDisplay:YES];
}
- (void)rightMouseUp:(NSEvent*)e     { [self fireMouse:kWM_RBUTTONUP event:e]; }
- (void)otherMouseDown:(NSEvent*)e   { [self fireMouse:kWM_MBUTTONDOWN event:e]; }
- (void)otherMouseUp:(NSEvent*)e     { [self fireMouse:kWM_MBUTTONUP event:e]; }
- (void)otherMouseDragged:(NSEvent*)e{ [self fireMouse:kWM_MOUSEMOVE event:e]; }

- (void)scrollWheel:(NSEvent*)e {
    if (!g_webView) return;
    int x, y; unsigned int flags; [self wkePoint:e x:&x y:&y flags:&flags];
    // Cocoa scroll deltas are small; scale toward a Win32 WHEEL_DELTA (120) step.
    double dy = [e hasPreciseScrollingDeltas] ? [e scrollingDeltaY] : [e scrollingDeltaY] * 10.0;
    int delta = (int)(dy * 3.0);
    if (delta) wkeFireMouseWheelEvent(g_webView, x, y, delta, flags);
    [self setNeedsDisplay:YES];
}

static unsigned int vkFromKeyCode(unsigned short kc) {
    switch (kc) {
        case 51: return kVK_BACK;   case 48: return kVK_TAB;    case 36: return kVK_RETURN;
        case 76: return kVK_RETURN; case 53: return kVK_ESCAPE; case 117: return kVK_DELETE;
        case 123: return kVK_LEFT;  case 124: return kVK_RIGHT; case 126: return kVK_UP; case 125: return kVK_DOWN;
        default: return 0;
    }
}
- (void)keyDown:(NSEvent*)e {
    if (!g_webView) return;
    unsigned int vk = vkFromKeyCode([e keyCode]);
    if (vk) wkeFireKeyDownEvent(g_webView, vk, 0, false);
    NSString* chars = [e characters];
    for (NSUInteger i = 0; i < [chars length]; ++i) {
        unichar c = [chars characterAtIndex:i];
        if (c >= 0x20 || c == '\r' || c == '\t')  // printable + enter/tab
            wkeFireKeyPressEvent(g_webView, c, 0, false);
    }
    [self setNeedsDisplay:YES];
}
- (void)keyUp:(NSEvent*)e {
    if (!g_webView) return;
    unsigned int vk = vkFromKeyCode([e keyCode]);
    if (vk) wkeFireKeyUpEvent(g_webView, vk, 0, false);
}
@end

// wke paint callback -> request a redraw on the main thread.
static void onPaintUpdated(wkeWebView, void*, const HDC, int x, int y, int cx, int cy) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_contentView setNeedsDisplay:YES];
    });
}

@interface MbAppDelegate : NSObject <NSApplicationDelegate>
@end
@implementation MbAppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)s { return YES; }
@end

int main(int argc, const char** argv) {
    @autoreleasepool {
        const char* url = (argc > 1) ? argv[1] : "https://example.com";
        const int W = 1024, H = 768;

        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        MbAppDelegate* del = [[MbAppDelegate alloc] init];
        [NSApp setDelegate:del];

        NSRect frame = NSMakeRect(0, 0, W, H);
        NSWindow* win = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
            backing:NSBackingStoreBuffered defer:NO];
        [win setTitle:@"miniblink (macOS)"];
        g_contentView = [[MbBrowserView alloc] initWithFrame:frame];
        [win setContentView:g_contentView];
        [win setAcceptsMouseMovedEvents:YES];   // deliver mouseMoved for hover
        [win center];
        [win makeKeyAndOrderFront:nil];
        [win makeFirstResponder:g_contentView]; // keyboard goes to the page
        [NSApp activateIgnoringOtherApps:YES];

        // Bring up the engine and load the page.
        wkeInitialize();
        g_webView = wkeCreateWebView();
        wkeResize(g_webView, W, H);
        wkeOnPaintUpdated(g_webView, onPaintUpdated, nullptr);
        wkeLoadURL(g_webView, url);

        NSLog(@"[minibrowser] loading %s", url);

        // Drive the render: each frame, let wke run its pending paint (which fires
        // onPaintUpdated when the page changes) and mark the view dirty. Without an
        // active pump the offscreen page never composites and the window stays blank.
        __block int ticks = 0;
        [NSTimer scheduledTimerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer*) {
            ++ticks;
            if (ticks <= 3) NSLog(@"[minibrowser] tick %d: before wkeRepaintIfNeeded", ticks);
            wkeRepaintIfNeeded(g_webView);
            if (ticks <= 3) NSLog(@"[minibrowser] tick %d: after wkeRepaintIfNeeded", ticks);
            [g_contentView setNeedsDisplay:YES];
            if (ticks % 60 == 0)
                NSLog(@"[minibrowser] tick=%d loading=%d complete=%d", ticks,
                      wkeIsLoading(g_webView), wkeIsLoadingCompleted(g_webView));
        }];
        NSLog(@"[minibrowser] timer scheduled, entering run loop");

        // Cocoa run loop; blink's scheduler advances via the GCD SharedTimerMac.
        [NSApp run];
    }
    return 0;
}
