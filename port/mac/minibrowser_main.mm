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
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), img);
    CGImageRelease(img);
    CGContextRelease(bmp);
    CGColorSpaceRelease(cs);
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
        [win center];
        [win makeKeyAndOrderFront:nil];
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
