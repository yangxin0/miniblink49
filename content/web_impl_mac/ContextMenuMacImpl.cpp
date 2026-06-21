// C++ logic for content::ContextMenu on macOS (sibling of the Win32
// content/ui/ContextMeun.h). This file is plain C++ and must NOT be compiled as
// Objective-C++: it pulls in WebPage/WebViewImpl (and thus KURL.h), whose
// unqualified use of WTF::TextEncoding collides with Carbon's global
// `TextEncoding`. The Cocoa NSMenu construction lives in ContextMenuMac.mm and
// is reached through the plain-C bridge declared in ContextMenuMac.h.

#include "content/web_impl_mac/ContextMenuMac.h"

#include "content/browser/WebPage.h"
#include "content/browser/WebPageImpl.h"
#include "third_party/WebKit/Source/web/WebViewImpl.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebContextMenuData.h"
#include "wke/wkeGlobalVar.h"
#include "wke/wkeWebView.h"
#include "wke/wkedefine.h"

#include <string>
#include <vector>

namespace content {
// Defined in WebClipboardImplMac.mm; used by the "Save image as" action, exactly
// as the Win32 backend does (content/ui/ContextMeun.h references this global).
extern WebPageImpl* g_saveImageingWebPage;
}

namespace content {

ContextMenu::ContextMenu(WebPage* webPage)
    : m_webPage(webPage)
    , m_frameId(0)
{
}

ContextMenu::~ContextMenu()
{
}

// Returns true if the item is allowed both by the data-driven action flags and
// by the wke global item mask (mirrors ContextMenu::canShowItem on Windows).
static bool canShowItem(unsigned int actionFlags, ContextMenu::MenuId id)
{
    if ((actionFlags & id) && (wke::g_contextMenuItemMask & id))
        return true;
    return false;
}

unsigned int ContextMenu::computeActionFlags(const blink::WebContextMenuData& data)
{
    unsigned int actionFlags = 0;

    if (!data.selectedText.isNull() && !data.selectedText.isEmpty())
        actionFlags |= kCopyTextId;

    if (data.hasImageContents) {
        actionFlags |= kCopyImageId;
        actionFlags |= kSaveImageId;
        m_imagePos = blink::IntPoint(data.mousePosition);
    }

    if (m_webPage->isDevtoolsConneted())
        actionFlags |= kInspectElementAtId;

    if (data.isEditable) {
        actionFlags |= kCutId;
        actionFlags |= kPasteId;
        actionFlags |= kSelectedAllId;
        actionFlags |= kUndoId;
    }

    if (m_webPage->canGoForward())
        actionFlags |= kGoForwardId;
    if (m_webPage->canGoBack())
        actionFlags |= kGoBackId;
    actionFlags |= kReloadId;

    // Ask the embedder whether a Print item should appear, like Windows does.
    wkeOnContextMenuItemClickCallback clickCallback = m_webPage->wkeHandler().contextMenuItemClickCallback;
    void* callbackParam = m_webPage->wkeHandler().contextMenuItemClickCallbackParam;
    if (clickCallback) {
        bool needPrint = clickCallback(m_webPage->wkeWebView(), callbackParam,
            kWkeContextMenuItemClickTypePrint, kWkeContextMenuItemClickStepShow,
            (wkeWebFrameHandle)m_frameId, nullptr);
        if (needPrint)
            actionFlags |= kPrintId;
    }

    return actionFlags;
}

// Returns the localized UTF-8 title for a menu id (zh-cn vs. en), matching the
// two appendMenuText* tables in the Win32 backend.
static const char* titleForItem(ContextMenu::MenuId id)
{
    bool zhcn = true;
    if (wke::g_language.get())
        zhcn = (std::string::npos != wke::g_language->find("zh-cn"));

    switch (id) {
    case ContextMenu::kCopyTextId:         return zhcn ? "\xE5\xA4\x8D\xE5\x88\xB6" : "Copy";                          // 复制
    case ContextMenu::kCopyImageId:        return zhcn ? "\xE5\xA4\x8D\xE5\x88\xB6\xE5\x9B\xBE\xE7\x89\x87" : "CopyImage"; // 复制图片
    case ContextMenu::kSaveImageId:        return zhcn ? "\xE5\x9B\xBE\xE7\x89\x87\xE5\x8F\xA6\xE5\xAD\x98\xE4\xB8\xBA" : "Save as.."; // 图片另存为
    case ContextMenu::kInspectElementAtId: return zhcn ? "\xE6\xA3\x80\xE6\x9F\xA5" : "InspectElementAt";              // 检查
    case ContextMenu::kCutId:              return zhcn ? "\xE5\x89\xAA\xE5\x88\x87" : "Cut";                            // 剪切
    case ContextMenu::kPasteId:            return zhcn ? "\xE7\xB2\x98\xE8\xB4\xB4" : "Paste";                         // 粘贴
    case ContextMenu::kSelectedAllId:      return zhcn ? "\xE5\x85\xA8\xE9\x80\x89" : "SelectedAll";                  // 全选
    case ContextMenu::kUndoId:             return zhcn ? "\xE6\x92\xA4\xE9\x94\x80" : "Undo";                          // 撤销
    case ContextMenu::kGoForwardId:        return zhcn ? "\xE5\x89\x8D\xE8\xBF\x9B" : "GoForward";                    // 前进
    case ContextMenu::kGoBackId:           return zhcn ? "\xE5\x90\x8E\xE9\x80\x80" : "GoBack";                        // 后退
    case ContextMenu::kReloadId:           return zhcn ? "\xE5\x88\xB7\xE6\x96\xB0" : "Reload";                        // 刷新
    case ContextMenu::kPrintId:            return zhcn ? "\xE6\x89\x93\xE5\x8D\xB0" : "Print";                         // 打印
    default:                               return "";
    }
}

void ContextMenu::onItemClicked(void* context, int itemId)
{
    ContextMenu* self = static_cast<ContextMenu*>(context);
    if (self)
        self->onCommand((unsigned int)itemId);
}

void ContextMenu::show(const blink::WebContextMenuData& data, int64_t frameId)
{
    m_data = data;
    m_frameId = frameId;

    unsigned int actionFlags = computeActionFlags(data);

    // Items are appended in the same order as the Win32 appendMenuText tables.
    static const ContextMenu::MenuId kOrder[] = {
        kCopyTextId, kCopyImageId, kSaveImageId, kInspectElementAtId,
        kCutId, kPasteId, kSelectedAllId, kUndoId,
        kGoForwardId, kGoBackId, kReloadId, kPrintId,
    };

    std::vector<MbContextMenuItem> items;
    for (size_t i = 0; i < sizeof(kOrder) / sizeof(kOrder[0]); ++i) {
        ContextMenu::MenuId id = kOrder[i];
        if (!canShowItem(actionFlags, id))
            continue;
        MbContextMenuItem item;
        item.id = (int)id;
        item.title = titleForItem(id);
        items.push_back(item);
    }

    if (items.empty())
        return;

    MbContextMenuShowNative(items.data(), (int)items.size(), &ContextMenu::onItemClicked, this);
}

void ContextMenu::onCommand(unsigned int itemID)
{
    // Action dispatch mirrors the Win32 ContextMenu::onCommand exactly.
    if (kCopyTextId == itemID) {
        m_webPage->webViewImpl()->focusedFrame()->executeCommand("Copy");
    } else if (kSelectedAllId == itemID) {
        m_webPage->webViewImpl()->focusedFrame()->executeCommand("SelectAll");
    } else if (kUndoId == itemID) {
        m_webPage->webViewImpl()->focusedFrame()->executeCommand("Undo");
    } else if (kCopyImageId == itemID) {
        m_webPage->webViewImpl()->copyImageAt(m_imagePos);
    } else if (kSaveImageId == itemID) {
        if (!g_saveImageingWebPage) {
            g_saveImageingWebPage = m_webPage->webPageImpl();
            m_webPage->webViewImpl()->copyImageAt(m_imagePos);
        }
    } else if (kInspectElementAtId == itemID) {
        m_webPage->inspectElementAt(m_data.mousePosition.x, m_data.mousePosition.y);
    } else if (kCutId == itemID) {
        m_webPage->webViewImpl()->focusedFrame()->executeCommand("Cut");
    } else if (kPasteId == itemID) {
        m_webPage->webViewImpl()->focusedFrame()->executeCommand("Paste");
    } else if (kGoForwardId == itemID) {
        m_webPage->goForward();
    } else if (kGoBackId == itemID) {
        m_webPage->goBack();
    } else if (kReloadId == itemID) {
        m_webPage->mainFrame()->reload();
    } else if (kPrintId == itemID) {
        wkeOnContextMenuItemClickCallback clickCallback = m_webPage->wkeHandler().contextMenuItemClickCallback;
        void* callbackParam = m_webPage->wkeHandler().contextMenuItemClickCallbackParam;
        if (clickCallback)
            clickCallback(m_webPage->wkeWebView(), callbackParam,
                kWkeContextMenuItemClickTypePrint, kWkeContextMenuItemClickStepClick,
                (wkeWebFrameHandle)m_frameId, nullptr);
    }
}

} // namespace content
