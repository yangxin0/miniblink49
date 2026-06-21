// PopupMenuStubsMac.cpp — link-only stubs for content::PopupMenuWin on macOS.
//
// content/ui/PopupMenuWin.cpp is a Win32 popup-window implementation (810 lines
// of CreateWindowEx / window hooks / GBK-encoded comments) used for <select>
// dropdowns and other native popups. The macOS browser will drive popups via
// NSMenu/NSPopUpButton later; until then we provide link-satisfying stubs for
// exactly the symbols the build references: create() (returns null -> no native
// popup is shown, callers fall back), hide()/fireWheelEvent()/fireKeyUpEvent()
// (no-op), the static m_hPopup handle, and the two Oilpan trace() overloads.
//
// trace() is GC tracing and is kept (empty) to match the upstream
// DEFINE_TRACE(PopupMenuWin) body, which traces nothing. Windows builds the real
// PopupMenuWin.cpp instead and is unaffected.

#if !defined(_WIN32)

#include "content/ui/PopupMenuWin.h"

namespace content {

// Static popup-window handle. No native popup window exists on macOS yet.
HWND PopupMenuWin::m_hPopup = NULL;

blink::WebWidget* PopupMenuWin::create(PopupMenuWinClient* /*client*/, HWND /*hWnd*/,
    blink::IntPoint /*offset*/, blink::WebViewImpl* /*webViewImpl*/,
    blink::WebPopupType /*type*/, PopupMenuWin** result)
{
    // No native popup support on macOS yet; report "not created".
    if (result)
        *result = nullptr;
    return nullptr;
}

void PopupMenuWin::hide()
{
}

LRESULT PopupMenuWin::fireWheelEvent(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return 0;
}

bool PopupMenuWin::fireKeyUpEvent(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return false;
}

// Oilpan GC tracing — mirrors the upstream empty DEFINE_TRACE(PopupMenuWin).
void PopupMenuWin::trace(blink::Visitor*)
{
}

void PopupMenuWin::trace(blink::InlinedGlobalMarkingVisitor)
{
}

} // namespace content

#endif // !defined(_WIN32)
