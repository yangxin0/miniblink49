// utf16_to_utf8_stringpiece.cc — base::UTF16ToUTF8(StringPiece16) overload.
//
// Chromium ships this overload in base/strings/utf_string_conversions.cc, but
// that file (from orig_chrome) does not compile against the trimmed in-tree
// base/strings/string_util.h, which lacks the StringPiece16 IsStringASCII
// overload. The trimmed base already provides UTF16ToUTF8(const string16&)
// (utf_offset_string_conversions.cc / string16 helpers), so we implement the
// StringPiece16 overload here by materializing the piece into a string16 and
// delegating. Keeps Windows untouched (this file is posix-only).

#if !defined(_WIN32)

#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"

namespace base {

std::string UTF16ToUTF8(StringPiece16 utf16)
{
    return UTF16ToUTF8(utf16.as_string());
}

} // namespace base

#endif // !defined(_WIN32)
