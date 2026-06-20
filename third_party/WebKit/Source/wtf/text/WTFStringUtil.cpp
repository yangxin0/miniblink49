
#include "wtf/text/WTFStringUtil.h"
#include "wtf/text/UTF8.h"

namespace WTF {

static Vector<UChar> iso88959ToUtf16(const char* str, int length)
{
    if (0 == length)
        return Vector<UChar>();

    // https://codereview.stackexchange.com/questions/40780/function-to-convert-iso-8859-1-to-utf-8
    Vector<char, 1024> bufferVector(length * 3);
    char* buffer = bufferVector.data();
    const LChar* characters = (const LChar*)str;
    Unicode::ConversionResult result = Unicode::convertLatin1ToUTF8(&characters, characters + length, &buffer, buffer + bufferVector.size());
    if (Unicode::conversionOK != result)
        return Vector<UChar>();

    String retVal = String::fromUTF8(bufferVector.data(), buffer - bufferVector.data());
    ASSERT(!retVal.is8Bit());
    return retVal.charactersWithNullTermination();
}
    
Vector<UChar> ensureUTF16UChar(const String& string, bool isNullTermination)
{
    if (string.isNull() || string.isEmpty())
        return Vector<UChar>();

    Vector<UChar> out;
    if (!string.is8Bit()) {
        if (isNullTermination)
            return string.charactersWithNullTermination();
        out.append(string.characters16(), string.length());
        return out;
    }

    //RELEASE_ASSERT(isLegalUTF8(string.characters8(), string.length()));
    if (string.containsOnlyASCII()) {
        out = string.charactersWithNullTermination();
        if (!isNullTermination)
            out.removeLast();
        return out;
    }

    String retVal = String::fromUTF8(string.characters8(), string.length());
    if (retVal.isNull() || retVal.isEmpty()) {
        std::vector<UChar> wbuf;
        out = iso88959ToUtf16((LPCSTR)string.characters8(), string.length());
    } else {
        ASSERT(!retVal.is8Bit());
        out = retVal.charactersWithNullTermination();
    }

    if (!isNullTermination)
        out.removeLast();
    return out;
}

String ensureUTF16String(const String& string)
{
    if (string.isNull() || string.isEmpty())
        return String("");  // L"" (wchar_t) isn't a valid String ctor on macOS
    if (!string.is8Bit())
        return String(string.characters16(), string.length());

    const LChar* stringStart = string.characters8();
    size_t length = string.length();
    if (charactersAreAllASCII(stringStart, length))
        return String::make16BitFrom8BitSource(stringStart, length);

    Vector<UChar, 1024> buffer(length);
    UChar* bufferStart = buffer.data();

    UChar* bufferCurrent = bufferStart;
    const char* stringCurrent = reinterpret_cast<const char*>(stringStart);
    if (WTF::Unicode::convertUTF8ToUTF16(&stringCurrent, reinterpret_cast<const char *>(stringStart + length),
        &bufferCurrent, bufferCurrent + buffer.size()) != WTF::Unicode::conversionOK)
        return "";

    unsigned utf16Length = bufferCurrent - bufferStart;
    ASSERT(utf16Length < length);
    return StringImpl::create(bufferStart, utf16Length);
}

Vector<char> ensureStringToUTF8(const String& string, bool isNullTermination)
{
    Vector<char> out;
    if (string.isNull() || string.isEmpty()) {
        if (isNullTermination)
            out.append('\0');
        return out;
    }

    if (string.is8Bit()) {
        out.resize(string.length());
        memcpy(out.data(), string.characters8(), string.length());
    } else {
        CString utf8 = string.utf8();
        size_t len = utf8.length();
        if (0 == len)
            return out;
        
        out.resize(len);
        memcpy(out.data(), utf8.data(), len);
    }

    if (isNullTermination && '\0' != out[out.size() - 1])
        out.append('\0');

    return out;
}

String ensureStringToUTF8String(const String& string)
{
    Vector<char> out = ensureStringToUTF8(string, false);
    return String(out.data(), out.size());
}

static bool isWhiteSpace(UChar c)
{
    return L' ' == c || L'\r' == c || L'\n' == c;
}

void stringTrim(String& stringInOut, bool leftTrim, bool rightTrim)
{
    if (stringInOut.isNull() || stringInOut.isEmpty())
        return;

    if (leftTrim) {
        while (!stringInOut.isNull() && !stringInOut.isEmpty()) {
            UChar c = stringInOut[0];
            if (!isWhiteSpace(c))
                break;

            stringInOut.remove(0);
        }
    }

    if (rightTrim) {
        while (!stringInOut.isNull() && !stringInOut.isEmpty()) {
            UChar c = stringInOut[stringInOut.length() - 1];
            if (!isWhiteSpace(c))
                break;

            stringInOut.remove(stringInOut.length() - 1);
        }
    }
}

#if defined(_WIN32)
void MByteToWChar(const char* lpcszStr, size_t cbMultiByte, std::vector<UChar>* out, UINT codePage)
{
    out->clear();

    DWORD dwMinSize;
    dwMinSize = MultiByteToWideChar(codePage, 0, lpcszStr, cbMultiByte, NULL, 0);
    if (0 == dwMinSize)
        return;

    out->resize(dwMinSize);

    // Convert headers from ASCII to Unicode.
    MultiByteToWideChar(codePage, 0, lpcszStr, cbMultiByte, &out->at(0), dwMinSize);
}

void WCharToMByte(const UChar* lpWideCharStr, size_t cchWideChar, std::vector<char>* out, UINT codePage)
{
    out->clear();

    DWORD dwMinSize;
    dwMinSize = WideCharToMultiByte(codePage, 0, lpWideCharStr, cchWideChar, NULL, 0, NULL, FALSE);
    if (0 == dwMinSize)
        return;

    out->resize(dwMinSize);

    // Convert headers from ASCII to Unicode.
    WideCharToMultiByte(codePage, 0, lpWideCharStr, cchWideChar, &out->at(0), dwMinSize, NULL, FALSE);
}
#else
// POSIX: codepage conversion isn't available. Handle UTF-8 exactly; treat other
// code pages as Latin-1 (1:1) as a best effort. (Note: the wide type here is the
// platform wchar_t to match the Win signatures used by callers.)
static const unsigned kCpUtf8 = 65001;  // CP_UTF8

void MByteToWChar(const char* lpcszStr, size_t cbMultiByte, std::vector<UChar>* out, unsigned codePage)
{
    out->clear();
    const unsigned char* s = reinterpret_cast<const unsigned char*>(lpcszStr);
    if (codePage == kCpUtf8) {
        for (size_t i = 0; i < cbMultiByte; ) {
            unsigned char c = s[i++];
            uint32_t cp; int extra;
            if (c < 0x80) { cp = c; extra = 0; }
            else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
            else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
            else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
            else { cp = 0xFFFD; extra = 0; }
            for (int k = 0; k < extra && i < cbMultiByte; ++k) cp = (cp << 6) | (s[i++] & 0x3F);
            if (cp <= 0xFFFF) { out->push_back((UChar)cp); }
            else { cp -= 0x10000; out->push_back((UChar)(0xD800 | (cp >> 10))); out->push_back((UChar)(0xDC00 | (cp & 0x3FF))); }
        }
    } else {
        for (size_t i = 0; i < cbMultiByte; ++i) out->push_back((UChar)s[i]);  // Latin-1
    }
}

void WCharToMByte(const UChar* lpWideCharStr, size_t cchWideChar, std::vector<char>* out, unsigned codePage)
{
    out->clear();
    for (size_t i = 0; i < cchWideChar; ++i) {
        uint32_t cp = (uint32_t)lpWideCharStr[i];
        if (codePage == kCpUtf8) {
            if (cp < 0x80) out->push_back((char)cp);
            else if (cp < 0x800) { out->push_back((char)(0xC0 | (cp >> 6))); out->push_back((char)(0x80 | (cp & 0x3F))); }
            else if (cp < 0x10000) { out->push_back((char)(0xE0 | (cp >> 12))); out->push_back((char)(0x80 | ((cp >> 6) & 0x3F))); out->push_back((char)(0x80 | (cp & 0x3F))); }
            else { out->push_back((char)(0xF0 | (cp >> 18))); out->push_back((char)(0x80 | ((cp >> 12) & 0x3F))); out->push_back((char)(0x80 | ((cp >> 6) & 0x3F))); out->push_back((char)(0x80 | (cp & 0x3F))); }
        } else {
            out->push_back((char)(cp & 0xFF));  // Latin-1
        }
    }
}
#endif

void Utf8ToMByte(const char* lpUtf8CharStr, size_t cchUtf8Char, std::vector<char>* out, UINT codePage)
{
    out->resize(0);

    std::vector<UChar> tempBuf;
    MByteToWChar(lpUtf8CharStr, cchUtf8Char, &tempBuf, CP_UTF8);
    if (0 == tempBuf.size())
        return;
    WCharToMByte(&tempBuf[0], tempBuf.size(), out, codePage);
}

void MByteToUtf8(const char* lpMCharStr, size_t cchMChar, std::vector<char>* out, UINT codePage)
{
    out->resize(0);

    std::vector<UChar> tempBuf;
    MByteToWChar(lpMCharStr, cchMChar, &tempBuf, codePage);
    if (0 == tempBuf.size())
        return;
    WCharToMByte(&tempBuf[0], tempBuf.size(), out, CP_UTF8);
}

bool splitStringToVector(const String& strData, const char strSplit, bool needTrim, WTF::Vector<String>& out)
{
    ASSERT(strData.is8Bit());
    size_t nIndex = WTF::kNotFound;
    size_t nStartIndex = 0;
    size_t nCount = 0;
    String strItem;
    String strBuf = strData;

    out.clear();
    strBuf = strData;

    do {
        nIndex = strBuf.find(strSplit, nStartIndex);
        if (WTF::kNotFound == nIndex) {
            if (nStartIndex <= strBuf.length()) {
                nCount = strBuf.length() - nStartIndex;
                strItem = strBuf.substring(nStartIndex, nCount);
                if (strItem.isNull() || strItem.isEmpty())
                    continue;
                stringTrim(strItem, needTrim, needTrim);

                out.append(strItem);
            }
            break;
        }

        nCount = nIndex - nStartIndex;
        strItem = strBuf.substring(nStartIndex, nCount);
        stringTrim(strItem, needTrim, needTrim);
        out.append(strItem);

        nStartIndex = nIndex + 1;
        if (nStartIndex > strBuf.length())
            break;
    } while (nIndex != WTF::kNotFound);

    return true;
}

std::string WTFStringToStdString(const WTF::String& str)
{
    CString utf8 = str.utf8();
    std::string result(utf8.data());
    return result;
}

bool isTextUTF8(const char *str, int length)
{
    int i = 0;
    DWORD nBytes = 0; // UFT8����1-6���ֽڱ���,ASCII��һ���ֽ�
    UCHAR chr = 0;
    bool bAllAscii = true; // ���ȫ������ASCII, ˵������UTF-8
    for (i = 0; i < length; i++) {
        chr = (UCHAR)* (str + i);

        if ((chr & 0x80) != 0) // �ж��Ƿ�ASCII����,�������,˵���п�����UTF-8,ASCII��7λ����,����һ���ֽڴ�,���λ���Ϊ0,o0xxxxxxx
            bAllAscii = false;

        if (nBytes == 0) { // �������ASCII��,Ӧ���Ƕ��ֽڷ�,�����ֽ���

            if (chr >= 0x80) {
                if (chr >= 0xFC && chr <= 0xFD)
                    nBytes = 6;
                else if (chr >= 0xF8)
                    nBytes = 5;
                else if (chr >= 0xF0)
                    nBytes = 4;
                else if (chr >= 0xE0)
                    nBytes = 3;
                else if (chr >= 0xC0)
                    nBytes = 2;
                else {
                    return false;
                }
                nBytes--;
            }
        } else { // ���ֽڷ��ķ����ֽ�,ӦΪ 10xxxxxx
            if ((chr & 0xC0) != 0x80)
                return false;
            nBytes--;
        }
    }
    if (nBytes > 0) //Υ������
        return false;

    if (bAllAscii) //���ȫ������ASCII, ˵������UTF-8
        return false;
    return true;
}


} // WTF