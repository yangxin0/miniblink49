// mb_demo for macOS — a Cocoa port of weolar/mb-demo's Win32 "Estart" launcher
// demo, driving the macOS wke C API (libwke) instead of the Win32 windowing path.
//
// The original demo (mb-demo/src/main.cpp) uses wkeCreateWebWindow (a native
// Win32 HWND) + GDI. The macOS wke library has no native-window path; it renders
// the page off-screen into a memory bitmap. So this host mirrors port/mac/
// minibrowser_main.mm: it hosts an off-screen wkeWebView in a borderless NSWindow,
// blits wkePaint() pixels with CoreGraphics, and forwards Cocoa input.
//
// It keeps the demo's JavaScript<->C++ bindings (eMsg / eShellExec / eSetDir) so
// the same Vue page (resources/view/index.html) drives the window: the in-page
// title bar's close/min/max buttons and header drag all work through eMsg.
#import <Cocoa/Cocoa.h>
#include <vector>
#include <string>
#include "wke/wke.h"

// Win32 message + mouse-key-flag codes the wke input API expects (kept local so
// this Cocoa TU mirrors minibrowser_main.mm).
enum {
    kWM_MOUSEMOVE = 0x0200, kWM_LBUTTONDOWN = 0x0201, kWM_LBUTTONUP = 0x0202,
    kWM_RBUTTONDOWN = 0x0204, kWM_RBUTTONUP = 0x0205,
    kWM_MBUTTONDOWN = 0x0207, kWM_MBUTTONUP = 0x0208,
    kMK_LBUTTON = 0x0001, kMK_RBUTTON = 0x0002, kMK_SHIFT = 0x0004,
    kMK_CONTROL = 0x0008, kMK_MBUTTON = 0x0010,
    kVK_BACK = 0x08, kVK_TAB = 0x09, kVK_RETURN = 0x0D, kVK_ESCAPE = 0x1B,
    kVK_LEFT = 0x25, kVK_UP = 0x26, kVK_RIGHT = 0x27, kVK_DOWN = 0x28, kVK_DELETE = 0x2E,
};

static wkeWebView g_webView = nullptr;
static NSView* g_contentView = nil;
static NSWindow* g_window = nil;

// ---- The content view: blits the wke webview's RGBA buffer ------------------
@interface MbDemoView : NSView
@end

@implementation MbDemoView
- (BOOL)isFlipped { return YES; }  // top-left origin, matching wke/blink

- (void)drawRect:(NSRect)dirtyRect {
    if (!g_webView) return;
    int w = (int)self.bounds.size.width;
    int h = (int)self.bounds.size.height;
    if (w <= 0 || h <= 0) return;

    int pitch = w * 4;
    static std::vector<unsigned char> buf;
    buf.assign((size_t)pitch * h, 0);
    wkePaint(g_webView, buf.data(), pitch);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    CGContextRef bmp = CGBitmapContextCreate(buf.data(), w, h, 8, pitch, cs,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGImageRef img = CGBitmapContextCreateImage(bmp);
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
    NSString* s = [e characters];
    for (NSUInteger i = 0; i < [s length]; ++i) {
        unichar c = [s characterAtIndex:i];
        if (c >= 0x20 && c != 0x7f) wkeFireKeyPressEvent(g_webView, c, 0, false);
    }
    [self setNeedsDisplay:YES];
}
- (void)keyUp:(NSEvent*)e {
    if (!g_webView) return;
    unsigned int vk = vkFromKeyCode([e keyCode]);
    if (vk) wkeFireKeyUpEvent(g_webView, vk, 0, false);
}
@end

// Borderless windows must opt in to becoming key/main for input + focus.
@interface MbDemoWindow : NSWindow
@end
@implementation MbDemoWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

// wke paint callback -> request a redraw on the main thread.
static void onPaintUpdated(wkeWebView, void*, const HDC, int, int, int, int) {
    dispatch_async(dispatch_get_main_queue(), ^{ [g_contentView setNeedsDisplay:YES]; });
}

// Route page console.log / errors to stdout (useful while bringing up the demo).
static void onConsole(wkeWebView, void*, wkeConsoleLevel level, const wkeString message,
    const wkeString sourceName, unsigned sourceLine, const wkeString /*stack*/) {
    const utf8* m = message ? wkeGetString(message) : "";
    const utf8* s = sourceName ? wkeGetString(sourceName) : "";
    NSLog(@"[console:%d] %s  (%s:%u)", (int)level, m ? m : "", s ? s : "", sourceLine);
}

// Page title -> window title (harmless on a borderless window).
static void onTitleChanged(wkeWebView, void*, const wkeString title) {
    const utf8* t = title ? wkeGetString(title) : "";
    if (!t) return;
    NSString* s = [NSString stringWithUTF8String:t];
    dispatch_async(dispatch_get_main_queue(), ^{ [g_window setTitle:s]; });
}

// ---- Demo JS<->C++ bindings (ported from mb-demo/src/main.cpp) --------------

// Helper: read a string argument as a std::string.
static std::string jsArgStr(jsExecState es, int i) {
    if (i >= jsArgCount(es)) return std::string();
    const utf8* s = jsToTempString(es, jsArg(es, i));
    return s ? std::string(s) : std::string();
}

// eMsg("msg", [plan, index]) — window control from the page's title bar.
//   move / close / max / min / menu / itemMenu
static jsValue onEMsg(jsExecState es, void* /*param*/) {
    if (jsArgCount(es) < 1) return jsUndefined();
    std::string msg = jsArgStr(es, 0);
    dispatch_async(dispatch_get_main_queue(), ^{
        if (msg == "close") {
            [g_window performClose:nil];
        } else if (msg == "min") {
            [g_window miniaturize:nil];
        } else if (msg == "max") {
            [g_window zoom:nil];               // toggles zoomed/restored
        } else if (msg == "move") {
            NSEvent* cur = [NSApp currentEvent];
            if (cur && [cur type] == NSEventTypeLeftMouseDown)
                [g_window performWindowDragWithEvent:cur];
        }
        // "menu" / "itemMenu" — no native context menu in this demo; ignore.
    });
    NSLog(@"[mb_demo] eMsg('%s')", msg.c_str());
    return jsUndefined();
}

// eShellExec("command", [args, dir]) — Win32 ShellExecute in the original.
// On macOS: "runEchars" loads the bundled ECharts page; otherwise hand the path
// to `open` (works for URLs/apps/files; Windows .exe items simply no-op + log).
static jsValue onEShellExec(jsExecState es, void* /*param*/) {
    if (jsArgCount(es) < 1) return jsUndefined();
    std::string path = jsArgStr(es, 0);
    if (path == "runEchars") {
        if (g_webView) wkeLoadURL(g_webView, "http://hook.test/map.html");
    } else {
        NSString* p = [NSString stringWithUTF8String:path.c_str()];
        dispatch_async(dispatch_get_main_queue(), ^{
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:p]];
        });
    }
    NSLog(@"[mb_demo] eShellExec('%s')", path.c_str());
    return jsUndefined();
}

// eSetDir("dir") — set the working directory for quick-run commands. Logged only.
static jsValue onESetDir(jsExecState es, void* /*param*/) {
    NSLog(@"[mb_demo] eSetDir('%s')", jsArgStr(es, 0).c_str());
    return jsUndefined();
}

// ---- Local resource hook ----------------------------------------------------
// The demo's pages reference resources by relative URL. We load them through a
// fake "http://hook.test/" origin and serve the bytes from MB_DEMO_DIR via
// wkeOnLoadUrlBegin -> wkeNetSetData (the same approach as the original Win32
// demo; miniblink's file:// sub-resource loading is unreliable).
static const char kHookHost[] = "http://hook.test/";

static const char* mimeForPath(const std::string& p) {
    auto ends = [&](const char* s){ size_t n = strlen(s); return p.size() >= n && p.compare(p.size()-n, n, s) == 0; };
    if (ends(".html") || ends(".htm")) return "text/html";
    if (ends(".css"))  return "text/css";
    if (ends(".js"))   return "application/javascript";
    if (ends(".json")) return "application/json";
    if (ends(".png"))  return "image/png";
    if (ends(".gif"))  return "image/gif";
    if (ends(".jpg") || ends(".jpeg")) return "image/jpeg";
    if (ends(".svg"))  return "image/svg+xml";
    if (ends(".woff2"))return "font/woff2";
    if (ends(".woff")) return "font/woff";
    if (ends(".ttf"))  return "font/ttf";
    return "application/octet-stream";
}

// Percent-decode a URL path (e.g. %20 -> space, and percent-encoded UTF-8 bytes
// back to the raw bytes that match on-disk filenames).
static std::string urlDecode(const std::string& s) {
    std::string out; out.reserve(s.size());
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int hi = hex(s[i+1]), lo = hex(s[i+2]);
            if (hi >= 0 && lo >= 0) { out.push_back((char)((hi << 4) | lo)); i += 2; continue; }
        }
        out.push_back(s[i]);
    }
    return out;
}

static bool readWholeFile(const std::string& path, std::vector<char>* out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return false; }
    out->resize((size_t)n);
    size_t rd = n ? fread(out->data(), 1, (size_t)n, f) : 0;
    fclose(f);
    return rd == (size_t)n;
}

static bool onLoadUrlBegin(wkeWebView, void* /*param*/, const char* url, void* job) {
    if (!url || strncmp(url, kHookHost, sizeof(kHookHost) - 1) != 0)
        return false;                               // not ours: load normally
    std::string rel = url + (sizeof(kHookHost) - 1);
    size_t q = rel.find_first_of("?#");             // strip query/fragment
    if (q != std::string::npos) rel.resize(q);
    rel = urlDecode(rel);                           // %20 / encoded UTF-8 -> raw bytes
    const char* base = getenv("MB_DEMO_DIR");
    std::string path = std::string(base ? base : "") + "/" + rel;

    std::vector<char> buf;
    if (!readWholeFile(path, &buf)) {
        NSLog(@"[mb_demo] hook MISS: %s", path.c_str());
        return false;
    }
    wkeNetSetMIMEType(job, (char*)mimeForPath(rel));
    wkeNetSetData(job, buf.empty() ? (void*)"" : buf.data(), (int)buf.size());
    return true;                                     // served from disk
}

static const char* kModernUA =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36";

@interface MbDemoAppDelegate : NSObject <NSApplicationDelegate>
@end
@implementation MbDemoAppDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)s { return YES; }
@end

int main(int argc, const char** argv) {
    @autoreleasepool {
        // Resource directory holding the demo's view/ assets. Override with arg 1.
        const char* dir = (argc > 1)
            ? argv[1]
            : "/Users/yangxin/build/mb-demo/src/resources/view";
        setenv("MB_DEMO_DIR", dir, 1);
        const char* indexUrl = "http://hook.test/index.html";   // served from disk via the hook
        const int W = 640, H = 480;            // matches the original demo window

        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        MbDemoAppDelegate* del = [[MbDemoAppDelegate alloc] init];
        [NSApp setDelegate:del];

        NSRect frame = NSMakeRect(0, 0, W, H);
        g_window = [[MbDemoWindow alloc]
            initWithContentRect:frame
            styleMask:NSWindowStyleMaskBorderless          // demo draws its own chrome
            backing:NSBackingStoreBuffered defer:NO];
        [g_window setMovableByWindowBackground:YES];        // draggable even via background
        [g_window setTitle:@"miniblink demo"];

        g_contentView = [[MbDemoView alloc] initWithFrame:frame];
        [g_window setContentView:g_contentView];
        [g_window setAcceptsMouseMovedEvents:YES];
        [g_window center];
        [g_window makeKeyAndOrderFront:nil];
        [g_window makeFirstResponder:g_contentView];
        [NSApp activateIgnoringOtherApps:YES];

        // Bring up the engine.
        wkeInitialize();
        g_webView = wkeCreateWebView();
        wkeResize(g_webView, W, H);
        wkeOnPaintUpdated(g_webView, onPaintUpdated, nullptr);
        wkeOnTitleChanged(g_webView, onTitleChanged, nullptr);
        wkeOnConsole(g_webView, onConsole, nullptr);
        wkeOnLoadUrlBegin(g_webView, onLoadUrlBegin, nullptr);  // serve hook.test from disk
        wkeSetUserAgent(g_webView, kModernUA);

        // Register the demo's JS<->C++ bindings before loading the page.
        wkeJsBindFunction("eMsg", &onEMsg, nullptr, 5);
        wkeJsBindFunction("eShellExec", &onEShellExec, nullptr, 3);
        wkeJsBindFunction("eSetDir", &onESetDir, nullptr, 1);

        wkeLoadURL(g_webView, indexUrl);
        NSLog(@"[mb_demo] loading %s", indexUrl);

        // Pump the engine each frame so the offscreen page composites.
        [NSTimer scheduledTimerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer*) {
            wkeRepaintIfNeeded(g_webView);
            [g_contentView setNeedsDisplay:YES];
        }];

        [NSApp run];
    }
    return 0;
}
