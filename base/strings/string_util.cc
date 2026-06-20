
#include "base/strings/string_util.h"

#if defined(_WIN32)
#include <windows.h>
#endif
#include <vector>
#include <stdint.h>

namespace base {

namespace {
// Portable UTF-8 <-> UTF-16/UTF-32 conversion helpers (replace the Win32
// MultiByteToWideChar/WideCharToMultiByte calls, which don't exist on macOS,
// and correctly handle wchar_t being UTF-32 on macOS vs UTF-16 on Windows).
inline uint32_t decodeUtf8(const std::string& s, size_t& i) {
    unsigned char c = static_cast<unsigned char>(s[i++]);
    if (c < 0x80) return c;
    uint32_t cp; int extra;
    if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
    else return 0xFFFD;
    for (int k = 0; k < extra; ++k) {
        if (i >= s.size()) return 0xFFFD;
        unsigned char cc = static_cast<unsigned char>(s[i]);
        if ((cc & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (cc & 0x3F); ++i;
    }
    return cp;
}
inline void appendUtf8(std::string& out, uint32_t cp) {
    if (cp < 0x80) { out.push_back(static_cast<char>(cp)); }
    else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}
}  // namespace

const wchar_t* kWhitespaceWide = L" ";
static const char16 kWhitespaceUTF16Storage[] = { static_cast<char16>(' '), 0 };
const char16* kWhitespaceUTF16 = kWhitespaceUTF16Storage;
const char* kWhitespaceASCII = " ";

template<typename STR>
TrimPositions TrimStringT(const STR& input,
    const STR& trim_chars,
    TrimPositions positions,
    STR* output) {
    // Find the edges of leading/trailing whitespace as desired.
    const size_t last_char = input.length() - 1;
    const size_t first_good_char = (positions & TRIM_LEADING) ?
        input.find_first_not_of(trim_chars) : 0;
    const size_t last_good_char = (positions & TRIM_TRAILING) ?
        input.find_last_not_of(trim_chars) : last_char;

    // When the string was all whitespace, report that we stripped off whitespace
    // from whichever position the caller was interested in.  For empty input, we
    // stripped no whitespace, but we still need to clear |output|.
    if (input.empty() ||
        (first_good_char == STR::npos) || (last_good_char == STR::npos)) {
        bool input_was_empty = input.empty();  // in case output == &input
        output->resize(0);
        return input_was_empty ? TRIM_NONE : positions;
    }

    // Trim the whitespace.
    *output = input.substr(first_good_char, last_good_char - first_good_char + 1);

    // Return where we trimmed from.
    return static_cast<TrimPositions>(((first_good_char == 0) ? TRIM_NONE : TRIM_LEADING) | ((last_good_char == last_char) ? TRIM_NONE : TRIM_TRAILING));
}

TrimPositions TrimWhitespaceASCII(const std::string& input, TrimPositions positions, std::string* output) {
    std::string whitespaceASCII(" ");
    return TrimStringT(input, whitespaceASCII, positions, output);
}

TrimPositions TrimWhitespace(const string16& input, TrimPositions positions, string16* output) {
    return TrimStringT(input, base::string16(kWhitespaceUTF16), positions, output);
}

// This function is only for backward-compatibility.
// To be removed when all callers are updated.
TrimPositions TrimWhitespace(const std::string& input, TrimPositions positions, std::string* output) {
    return TrimWhitespaceASCII(input, positions, output);
}

std::string ToLowerASCII(const std::string& str) {
    std::string ret;
    ret.reserve(str.size());
    for (size_t i = 0; i < str.size(); i++)
        ret += (ToLowerASCII(str[i]));
    return ret;
}

std::string ToUpperASCII(const std::string& str) {
    std::string ret;
    ret.reserve(str.size());
    for (size_t i = 0; i < str.size(); i++)
        ret += (ToUpperASCII(str[i]));
    return ret;
}

std::wstring UTF8ToWide(const std::string& utf8) {
    std::wstring out;
    out.reserve(utf8.size());
    for (size_t i = 0; i < utf8.size(); ) {
        uint32_t cp = decodeUtf8(utf8, i);
        if (sizeof(wchar_t) >= 4) {
            out.push_back(static_cast<wchar_t>(cp));      // UTF-32 (macOS/Linux)
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<wchar_t>(cp));       // UTF-16 (Windows)
        } else {
            cp -= 0x10000;
            out.push_back(static_cast<wchar_t>(0xD800 | (cp >> 10)));
            out.push_back(static_cast<wchar_t>(0xDC00 | (cp & 0x3FF)));
        }
    }
    return out;
}

std::wstring ASCIIToWide(const std::string& ascii) {
    return UTF8ToWide(ascii);
}

// Decode a UTF-16 string16 (with surrogate pairs) to UTF-8.
std::string WideToUTF8(const string16& utf16) {
    std::string out;
    out.reserve(utf16.size());
    for (size_t i = 0; i < utf16.size(); ) {
        uint32_t cp = static_cast<uint16_t>(utf16[i++]);
        if (cp >= 0xD800 && cp <= 0xDBFF && i < utf16.size()) {
            uint32_t lo = static_cast<uint16_t>(utf16[i]);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                ++i;
            }
        }
        appendUtf8(out, cp);
    }
    return out;
}

std::string UTF16ToASCII(const string16& utf16) {
    return WideToUTF8(utf16);
}

std::string UTF16ToUTF8(const string16& utf16) {
    return WideToUTF8(utf16);
}

string16 ASCIIToUTF16(const std::string& ascii) {
    string16 out;
    out.reserve(ascii.size());
    for (size_t i = 0; i < ascii.size(); ++i)
        out.push_back(static_cast<char16>(static_cast<unsigned char>(ascii[i])));
    return out;
}

}