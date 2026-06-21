#include "base/macros.h"
#include <string>

namespace content {

#if defined(_WIN32)
// On Windows wchar_t is 16-bit == WebUChar, so the L"..." literal is a UTF-16 buffer.
#define MAKE_UCHAR_TO_WEBSTRING(s) \
    blink::WebString(s, sizeof(s)/sizeof(WebUChar))
#else
// On macOS wchar_t is 32-bit (UTF-32) but WebUChar is 16-bit, so the WebString(UChar*,
// len) ctor never matches. Convert the wide literal to UTF-8 and use fromUTF8.
static inline blink::WebString wcharToWebString(const wchar_t* s)
{
    std::string utf8;
    for (; *s; ++s) {
        unsigned int cp = (unsigned int)*s;
        if (cp < 0x80) {
            utf8.push_back((char)cp);
        } else if (cp < 0x800) {
            utf8.push_back((char)(0xC0 | (cp >> 6)));
            utf8.push_back((char)(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            utf8.push_back((char)(0xE0 | (cp >> 12)));
            utf8.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            utf8.push_back((char)(0x80 | (cp & 0x3F)));
        } else {
            utf8.push_back((char)(0xF0 | (cp >> 18)));
            utf8.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
            utf8.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            utf8.push_back((char)(0x80 | (cp & 0x3F)));
        }
    }
    return blink::WebString::fromUTF8(utf8.data(), utf8.length());
}
#define MAKE_UCHAR_TO_WEBSTRING(s) content::wcharToWebString(s)
#endif

blink::WebString queryLocalizedStringFromResources(blink::WebLocalizedString::Name name)
{
    blink::WebString out;
    switch (name) {
    case blink::WebLocalizedString::BlockedPluginText:
        out = MAKE_UCHAR_TO_WEBSTRING(L"阻塞插件");
        break;
    case blink::WebLocalizedString::FileButtonChooseFileLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"选择文件");
        break;
    case blink::WebLocalizedString::FileButtonChooseMultipleFilesLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"选择多个文件");
        break;
    case blink::WebLocalizedString::FileButtonNoFileSelectedLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"没有文件被选中");
        break;
    case blink::WebLocalizedString::InputElementAltText:
        out = MAKE_UCHAR_TO_WEBSTRING(L"AltText");
        break;
    case blink::WebLocalizedString::MissingPluginText:
        out = MAKE_UCHAR_TO_WEBSTRING(L"缺少插件");
        break;
    case blink::WebLocalizedString::MultipleFileUploadText:
        out = MAKE_UCHAR_TO_WEBSTRING(L"上传文件");
        break;
    case blink::WebLocalizedString::OtherColorLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"其他颜色");
        break;
    case blink::WebLocalizedString::OtherDateLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"其他日期");
        break;
    case blink::WebLocalizedString::OtherMonthLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"其他月份");
        break;
    case blink::WebLocalizedString::ResetButtonDefaultLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"重置");
        break;
    case blink::WebLocalizedString::SearchableIndexIntroduction:
        out = MAKE_UCHAR_TO_WEBSTRING(L"SearchableIndexIntroduction");
        break;
    case blink::WebLocalizedString::SearchMenuClearRecentSearchesText:
        out = MAKE_UCHAR_TO_WEBSTRING(L"SearchMenuClearRecentSearchesText");
        break;
    case blink::WebLocalizedString::SelectMenuListText:
        out = MAKE_UCHAR_TO_WEBSTRING(L"选择菜单");
        break;
    case blink::WebLocalizedString::SubmitButtonDefaultLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"提交");
        break;
    case blink::WebLocalizedString::ThisMonthButtonLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"本月");
        break;
    case blink::WebLocalizedString::ThisWeekButtonLabel:
        out = MAKE_UCHAR_TO_WEBSTRING(L"本周");
        break;
    }

    return out;
}

#undef MAKE_UCHAR_TO_WEBSTRING

} // namespace content