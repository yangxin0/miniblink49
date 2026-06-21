// macOS sibling of content/ui/ContextMeun.h (Win32 popup menu).
//
// WebFrameClientImpl on Windows includes content/ui/ContextMeun.h, which defines
// a Win32 popup-menu window. That header is hard-wired to <windows.h>
// (WNDCLASSEX / CreateWindowExW / AppendMenu / TrackPopupMenuEx) and cannot be
// compiled on macOS. This file declares a class with the SAME name
// (content::ContextMenu) and the SAME public interface that WebFrameClientImpl
// actually uses, so the call sites in WebFrameClientImpl.cpp compile unchanged:
//
//     m_menu = new ContextMenu(m_webPage);   // ctor(WebPage*)
//     m_menu->show(data, frameId);           // show(const WebContextMenuData&, int64_t)
//     delete m_menu;                         // dtor
//
// Implementation is intentionally split across two translation units:
//
//   * ContextMenuMacImpl.cpp -- plain C++. Holds the ContextMenu class logic
//     (ctor/dtor/show/onCommand). It includes the heavy blink headers
//     (WebPage / WebViewImpl, which transitively pull KURL.h). It must NOT be
//     compiled as Objective-C++ / pull in any Cocoa framework, because Carbon's
//     CoreServices defines a global `TextEncoding` that collides with blink's
//     WTF::TextEncoding used unqualified in KURL.h.
//
//   * ContextMenuMac.mm -- Objective-C++. Builds the real Cocoa NSMenu and pops
//     it up. It includes ONLY this header (which does not pull KURL.h), so the
//     Carbon/WTF TextEncoding collision never arises. It talks to the C++ side
//     through the plain-C bridge declared at the bottom of this header.
//
// Linking ContextMenuMac.mm requires -framework AppKit.

#ifndef content_web_impl_mac_ContextMenuMac_h
#define content_web_impl_mac_ContextMenuMac_h

#include "third_party/WebKit/public/web/WebContextMenuData.h"
#include "third_party/WebKit/Source/platform/geometry/IntPoint.h"
#include <stdint.h>

namespace content {

class WebPage;

// Mirrors the public surface of the Win32 content::ContextMenu used by
// WebFrameClientImpl. Item ids match the Windows MenuId enum so behavior maps
// 1:1 once action dispatch is wired through onCommand().
class ContextMenu {
public:
    // Same item ids as the Win32 ContextMenu::MenuId enum, kept in sync so the
    // dispatch logic mirrors the Windows backend exactly.
    enum MenuId {
        kSelectedAllId = 1 << 1,
        kCopyTextId = 1 << 2,
        kUndoId = 1 << 3,
        kCopyImageId = 1 << 4,
        kInspectElementAtId = 1 << 5,
        kCutId = 1 << 6,
        kPasteId = 1 << 7,
        kPrintId = 1 << 8,
        kGoForwardId = 1 << 9,
        kGoBackId = 1 << 10,
        kReloadId = 1 << 11,
        kSaveImageId = 1 << 12,
    };

    // Same constructor signature as the Win32 backend: takes the owning WebPage.
    explicit ContextMenu(WebPage* webPage);

    // Same destructor: tears down the native menu.
    ~ContextMenu();

    // Same signature as the Win32 backend. Builds and shows a native NSMenu.
    void show(const blink::WebContextMenuData& data, int64_t frameId);

    // Executes the blink/wke action for a clicked menu item. Shared with the
    // Cocoa target's action handler.
    void onCommand(unsigned int itemID);

private:
    // Computes which items are enabled for the given context menu data,
    // mirroring ContextMenu::show() on Windows.
    unsigned int computeActionFlags(const blink::WebContextMenuData& data);

    WebPage* m_webPage;
    blink::WebContextMenuData m_data;
    blink::IntPoint m_imagePos;
    int64_t m_frameId;

    // Trampoline used as the C bridge callback; forwards to onCommand().
    static void onItemClicked(void* context, int itemId);
};

// ---------------------------------------------------------------------------
// Plain-C bridge between the C++ logic (ContextMenuMacImpl.cpp) and the Cocoa
// backend (ContextMenuMac.mm). Kept free of any blink/Cocoa types so it can be
// shared across both translation units without dragging KURL.h into the .mm.
// ---------------------------------------------------------------------------

// A single menu entry passed from C++ to the Cocoa builder.
struct MbContextMenuItem {
    int id;             // ContextMenu::MenuId value, surfaced back on click.
    const char* title;  // UTF-8 localized title.
};

// Callback invoked (on the main thread) when a menu item is clicked.
typedef void (*MbContextMenuClickCallback)(void* context, int itemId);

// Builds an NSMenu from |items| and pops it up at the current mouse location.
// On click, |callback| is invoked with |context| and the clicked item id.
// Implemented in ContextMenuMac.mm. Safe to declare here because it uses only
// plain-C types.
void MbContextMenuShowNative(const MbContextMenuItem* items,
                            int itemCount,
                            MbContextMenuClickCallback callback,
                            void* context);

} // namespace content

#endif // content_web_impl_mac_ContextMenuMac_h
