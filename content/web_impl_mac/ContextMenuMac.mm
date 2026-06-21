// Cocoa backend for content::ContextMenu (sibling of the Win32
// content/ui/ContextMeun.h). This translation unit is Objective-C++ and links
// against -framework AppKit.
//
// IMPORTANT: it includes ONLY ContextMenuMac.h, which does not pull in KURL.h.
// The heavy blink headers (WebPage/WebViewImpl) live in ContextMenuMacImpl.cpp
// instead, because they reference WTF::TextEncoding unqualified, which collides
// with the global `TextEncoding` that Carbon/CoreServices (dragged in by AppKit)
// defines. Keeping the two apart avoids that name-lookup ambiguity entirely.

#include "content/web_impl_mac/ContextMenuMac.h"

#import <AppKit/AppKit.h>

// Objective-C target that owns the live NSMenu for one popup and forwards item
// clicks back into the C++ side via the plain-C bridge callback.
@interface MbContextMenuTarget : NSObject {
@public
    content::MbContextMenuClickCallback _callback;
    void* _context;
}
- (void)onItemClicked:(id)sender;
@end

@implementation MbContextMenuTarget
- (void)onItemClicked:(id)sender
{
    NSMenuItem* item = (NSMenuItem*)sender;
    if (_callback)
        _callback(_context, (int)[item tag]);
}
@end

namespace content {

void MbContextMenuShowNative(const MbContextMenuItem* items,
                            int itemCount,
                            MbContextMenuClickCallback callback,
                            void* context)
{
    if (!items || itemCount <= 0)
        return;

    MbContextMenuTarget* target = [[[MbContextMenuTarget alloc] init] autorelease];
    target->_callback = callback;
    target->_context = context;

    NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"MbContextMenu"] autorelease];
    [menu setAutoenablesItems:NO];

    for (int i = 0; i < itemCount; ++i) {
        NSString* title = [NSString stringWithUTF8String:(items[i].title ? items[i].title : "")];
        NSMenuItem* menuItem = [[[NSMenuItem alloc] initWithTitle:title
                                                           action:@selector(onItemClicked:)
                                                    keyEquivalent:@""] autorelease];
        [menuItem setTag:(NSInteger)items[i].id];
        [menuItem setTarget:target];
        [menuItem setEnabled:YES];
        [menu addItem:menuItem];
    }

    if ([menu numberOfItems] == 0)
        return;

    // Pop up at the current mouse location. popUpMenuPositioningItem runs a
    // modal tracking loop and invokes the item's action synchronously, so the
    // autoreleased target stays alive for the duration of the click dispatch.
    NSPoint mouseLoc = [NSEvent mouseLocation];
    NSWindow* keyWindow = [NSApp keyWindow];
    if (keyWindow) {
        NSRect frameRect = [keyWindow convertRectFromScreen:NSMakeRect(mouseLoc.x, mouseLoc.y, 0, 0)];
        NSView* contentView = [keyWindow contentView];
        NSPoint viewPoint = [contentView convertPoint:frameRect.origin fromView:nil];
        [menu popUpMenuPositioningItem:nil atLocation:viewPoint inView:contentView];
    } else {
        [menu popUpMenuPositioningItem:nil atLocation:mouseLoc inView:nil];
    }
}

} // namespace content
