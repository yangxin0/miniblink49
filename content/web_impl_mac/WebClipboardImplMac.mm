// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// macOS Cocoa sibling of content/web_impl_win/WebClipboardImpl.cpp.
//
// The Windows variant (WebClipboardImpl.cpp) is built entirely on the Win32
// clipboard API (OpenClipboard/GetClipboardData/SetClipboardData/GlobalLock),
// GDI (HDC/CreateDIBSection/GdiAlphaBlend/SetDIBitsToDevice), a Win32
// message-only window for clipboard ownership, and COM IDataObject pulled in
// through the shared header content/ui/ClipboardUtil.h (<ShlObj.h>). None of
// that exists on macOS, so the Windows file is excluded on this platform and
// this Cocoa NSPasteboard implementation takes its place.
//
// This is the real Cocoa equivalent: read/write plain text and HTML go through
// NSPasteboard, mirroring the behavior of the Windows clipboard paths.

#include "config.h"
#include "content/web_impl_win/WebClipboardImpl.h"

#include "third_party/WebKit/public/platform/WebData.h"
#include "third_party/WebKit/public/platform/WebDragData.h"
#include "third_party/WebKit/public/platform/WebImage.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "third_party/WebKit/Source/platform/geometry/IntSize.h"
#include "wtf/text/WTFStringUtil.h"

#import <AppKit/AppKit.h>

using blink::WebClipboard;
using blink::WebData;
using blink::WebDragData;
using blink::WebImage;
using blink::WebString;
using blink::WebURL;
using blink::WebVector;

namespace {

// Convert a blink::WebString to an autoreleased NSString.
static NSString* webStringToNSString(const WebString& text)
{
    WTF::String s = text;
    if (s.isNull() || s.isEmpty())
        return @"";
    if (s.is8Bit()) {
        std::string utf8 = WTF::WTFStringToStdString(s);
        return [NSString stringWithUTF8String:utf8.c_str()];
    }
    return [NSString stringWithCharacters:reinterpret_cast<const unichar*>(s.characters16())
                                   length:s.length()];
}

static WebString nsStringToWebString(NSString* s)
{
    if (!s)
        return WebString();
    NSUInteger len = [s length];
    if (0 == len)
        return WebString();
    std::vector<unichar> buf(len);
    [s getCharacters:buf.data() range:NSMakeRange(0, len)];
    return WebString(reinterpret_cast<const UChar*>(buf.data()), len);
}

} // namespace

namespace content {

// Forward declaration only: this Cocoa TU must NOT include WebPageImpl.h,
// because that header pulls in KURL.h whose unqualified WTF::TextEncoding use
// collides with Carbon/CoreServices' global `TextEncoding` (dragged in by
// AppKit). This global is merely a pointer definition (no member access), so a
// forward declaration is sufficient. The matching extern declaration lives in
// ContextMenuMacImpl.cpp, which is a pure C++ TU and does include WebPageImpl.h.
class WebPageImpl;

WebPageImpl* g_saveImageingWebPage = nullptr;

WebClipboardImpl::WebClipboardImpl()
    : m_clipboardOwner(nullptr)
{
}

WebClipboardImpl::~WebClipboardImpl()
{
}

uint64 WebClipboardImpl::sequenceNumber(Buffer buffer)
{
    return (uint64)[[NSPasteboard generalPasteboard] changeCount];
}

bool WebClipboardImpl::isFormatAvailable(Format format, Buffer buffer)
{
    ClipboardType clipboardType = CLIPBOARD_TYPE_COPY_PASTE;
    if (!convertBufferType(buffer, &clipboardType))
        return false;

    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    switch (format) {
    case FormatPlainText:
        return [pasteboard availableTypeFromArray:@[ NSPasteboardTypeString ]] != nil;
    case FormatHTML:
        return [pasteboard availableTypeFromArray:@[ NSPasteboardTypeHTML ]] != nil;
    case FormatSmartPaste:
    case FormatBookmark:
    default:
        break;
    }
    return false;
}

blink::WebVector<blink::WebString> WebClipboardImpl::readAvailableTypes(Buffer buffer, bool* containsFilenames)
{
    ClipboardType clipboardType;
    Vector<WebString> types;
    if (convertBufferType(buffer, &clipboardType))
        readAvailableTypes(clipboardType, &types, containsFilenames);
    return types;
}

void WebClipboardImpl::readAvailableTypes(ClipboardType type, Vector<WebString>* types, bool* containsFilenames) const
{
    if (!types || !containsFilenames)
        return;
    *containsFilenames = false;
    types->clear();

    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    if ([pasteboard availableTypeFromArray:@[ NSPasteboardTypeString ]])
        types->append(WebString::fromUTF8("text/plain"));
    if ([pasteboard availableTypeFromArray:@[ NSPasteboardTypeHTML ]])
        types->append(WebString::fromUTF8("text/html"));
    if ([pasteboard availableTypeFromArray:@[ NSPasteboardTypePNG ]])
        types->append(WebString::fromUTF8("image/png"));
}

blink::WebString WebClipboardImpl::readPlainText(Buffer buffer)
{
    ClipboardType clipboardType;
    if (!convertBufferType(buffer, &clipboardType))
        return WebString();
    NSString* s = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
    return nsStringToWebString(s);
}

blink::WebString WebClipboardImpl::readHTML(Buffer buffer, WebURL* sourceUrl, unsigned* fragmentStart, unsigned* fragmentEnd)
{
    ClipboardType clipboardType;
    if (!convertBufferType(buffer, &clipboardType))
        return WebString();

    if (fragmentStart)
        *fragmentStart = 0;
    NSString* html = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeHTML];
    WebString result = nsStringToWebString(html);
    if (fragmentEnd)
        *fragmentEnd = result.length();
    return result;
}

WebData WebClipboardImpl::readImage(Buffer buffer)
{
    ClipboardType clipboardType;
    if (!convertBufferType(buffer, &clipboardType))
        return WebData(" ", 1);

    NSData* png = [[NSPasteboard generalPasteboard] dataForType:NSPasteboardTypePNG];
    if (!png || 0 == [png length])
        return WebData(" ", 1);
    return WebData(reinterpret_cast<const char*>([png bytes]), [png length]);
}

WebString WebClipboardImpl::readCustomData(Buffer buffer, const WebString& type)
{
    return WebString();
}

void WebClipboardImpl::clearClipboard()
{
    [[NSPasteboard generalPasteboard] clearContents];
}

void WebClipboardImpl::writePlainText(const WebString& plainText)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard setString:webStringToNSString(plainText) forType:NSPasteboardTypeString];
}

void WebClipboardImpl::writeHTML(const WebString& htmlText, const WebURL& sourceUrl, const WebString& plainText, bool writeSmartPaste)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard declareTypes:@[ NSPasteboardTypeHTML, NSPasteboardTypeString ] owner:nil];
    [pasteboard setString:webStringToNSString(htmlText) forType:NSPasteboardTypeHTML];
    [pasteboard setString:webStringToNSString(plainText) forType:NSPasteboardTypeString];
}

void WebClipboardImpl::writeHTMLInternal(const WebString& htmlText, const WebURL& sourceUrl, const WebString& plainText, bool writeSmartPaste)
{
    writeHTML(htmlText, sourceUrl, plainText, writeSmartPaste);
}

void WebClipboardImpl::writeBitmapFromHandle(HBITMAP sourceHBitmap, const blink::IntSize& size)
{
    // Win32 GDI specific; no macOS equivalent path through HBITMAP.
}

bool WebClipboardImpl::writeBitmapInternal(const SkBitmap& bitmap)
{
    // Image writing on macOS would go through NSPasteboardTypePNG using an
    // encoded PNG of the SkBitmap; deferred until the image pipeline is wired.
    return false;
}

void WebClipboardImpl::writeBookmark(const String& titleData, const String& urlData)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    NSString* url = webStringToNSString(WebString(urlData));
    [pasteboard setString:url forType:NSPasteboardTypeString];
}

void WebClipboardImpl::writeImage(const WebImage& image, const WebURL& url, const WebString& title)
{
    // Deferred: encode SkBitmap to PNG and place on NSPasteboardTypePNG.
}

void WebClipboardImpl::writeDataObject(const WebDragData& data)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];

    WebVector<WebDragData::Item> items = data.items();
    for (size_t i = 0; i < items.size(); ++i) {
        WebDragData::Item& it = items[i];
        if (WebDragData::Item::StorageTypeString == it.storageType)
            [pasteboard setString:webStringToNSString(it.stringData) forType:NSPasteboardTypeString];
    }
}

bool WebClipboardImpl::convertBufferType(Buffer buffer, ClipboardType* result)
{
    *result = CLIPBOARD_TYPE_COPY_PASTE;
    switch (buffer) {
    case BufferStandard:
        break;
    case BufferSelection:
        return false;
    default:
        return false;
    }
    return true;
}

void WebClipboardImpl::writeToClipboardInternal(unsigned int format, HANDLE handle)
{
    // Win32 SetClipboardData path; no-op on macOS (handled by NSPasteboard).
}

void WebClipboardImpl::writeTextInternal(const String& string)
{
    [[NSPasteboard generalPasteboard] setString:webStringToNSString(WebString(string))
                                        forType:NSPasteboardTypeString];
}

HWND WebClipboardImpl::getClipboardWindow()
{
    // No clipboard-owner message window on macOS.
    return nullptr;
}

}  // namespace content
