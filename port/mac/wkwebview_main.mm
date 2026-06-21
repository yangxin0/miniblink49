// WKWebView mini-browser — uses macOS's native (latest stable) WebKit engine.
// Zero engine build, zero extra disk: links the system WebKit.framework.
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

@interface AppDel : NSObject <NSApplicationDelegate>
@end
@implementation AppDel
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)s { return YES; }
@end

int main(int argc, const char** argv) {
    @autoreleasepool {
        const char* url = (argc > 1) ? argv[1] : "https://www.youtube.com";
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        AppDel* d = [[AppDel alloc] init]; [NSApp setDelegate:d];
        NSRect f = NSMakeRect(0, 0, 1200, 820);
        NSWindow* w = [[NSWindow alloc] initWithContentRect:f
            styleMask:(NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable|NSWindowStyleMaskMiniaturizable)
            backing:NSBackingStoreBuffered defer:NO];
        [w setTitle:@"WKWebView — native macOS WebKit"];
        WKWebView* wv = [[WKWebView alloc] initWithFrame:f];
        [wv setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
        [w setContentView:wv];
        [w center]; [w makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        [wv loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]]];
        NSLog(@"[wkwv] loading %s", url);
        [NSApp run];
    }
    return 0;
}
