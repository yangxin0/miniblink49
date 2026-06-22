// icu_hbicu_stubs_win.cpp — minimal stubs for the ICU normalization/property
// functions that harfbuzz's hb-icu.cc references but the bundled minimal ICU
// (uchar.cpp: u_charType/uscript_getScript/hasScript) does not provide. macOS
// links these from system libicucore; on Windows there is no full ICU in-tree.
//
// These are no-op/identity implementations: they satisfy the link and let basic
// (Latin/CJK) text shape correctly. Normalization-heavy complex-script shaping is
// degraded (no composition/decomposition), which is acceptable for the default
// browser. Compiled inside wtf so it inherits config.h + the WTF Unicode types.
#include <string.h>
#include "third_party/icu/source/common/unicode/uchar.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "third_party/icu/source/common/unicode/unorm.h"
#include "third_party/icu/source/common/unicode/unorm2.h"
#include "third_party/icu/source/common/unicode/ustring.h"

extern "C" {

// These are extern "C", so only the symbol name matters for the link; the trimmed
// in-tree ICU headers don't define UProperty/UNormalizationMode, so use int (ABI-
// compatible with the ICU enums).
uint8_t u_getCombiningClass(UChar32) { return 0; }
UChar32 u_charMirror(UChar32 c) { return c; }
int32_t u_getIntPropertyValue(UChar32, int /*UProperty*/) { return 0; }
const char* uscript_getShortName(int /*UScriptCode*/) { return "Zyyy"; }  // Common

int32_t u_countChar32(const UChar* s, int32_t length) {
    // Count UTF-32 code points in a UTF-16 string (length<0 => NUL-terminated).
    int32_t count = 0;
    int32_t i = 0;
    for (;;) {
        if (length < 0) { if (!s[i]) break; } else if (i >= length) break;
        UChar c = s[i++];
        if (c >= 0xD800 && c <= 0xDBFF) {  // high surrogate: consume the pair
            bool more = (length < 0) ? (s[i] != 0) : (i < length);
            if (more && s[i] >= 0xDC00 && s[i] <= 0xDFFF) ++i;
        }
        ++count;
    }
    return count;
}

UChar32* u_strToUTF32(UChar32* dest, int32_t destCapacity, int32_t* pDestLength,
                      const UChar* src, int32_t srcLength, UErrorCode*) {
    int32_t n = 0, i = 0;
    for (;;) {
        if (srcLength < 0) { if (!src[i]) break; } else if (i >= srcLength) break;
        UChar32 c = src[i++];
        if (c >= 0xD800 && c <= 0xDBFF) {
            bool more = (srcLength < 0) ? (src[i] != 0) : (i < srcLength);
            if (more && src[i] >= 0xDC00 && src[i] <= 0xDFFF)
                c = 0x10000 + ((c - 0xD800) << 10) + (src[i++] - 0xDC00);
        }
        if (n < destCapacity) dest[n] = c;
        ++n;
    }
    if (pDestLength) *pDestLength = n;
    if (n < destCapacity) dest[n] = 0;
    return dest;
}

// Normalizer: no composition/decomposition. Return a non-null sentinel instance
// so callers' null checks pass; the operations below are no-ops.
const UNormalizer2* unorm2_getNFCInstance(UErrorCode*) {
    static int s_dummy = 0;
    return reinterpret_cast<const UNormalizer2*>(&s_dummy);
}
UChar32 unorm2_composePair(const UNormalizer2*, UChar32, UChar32) { return -1; }
int32_t unorm2_getRawDecomposition(const UNormalizer2*, UChar32, UChar*, int32_t, UErrorCode*) {
    return -1;  // no decomposition
}
int32_t unorm_normalize(const UChar* source, int32_t sourceLength, UNormalizationMode,
                        int32_t, UChar* result, int32_t resultLength, UErrorCode*) {
    // Identity: copy source to result unchanged.
    int32_t len = sourceLength;
    if (len < 0) { len = 0; while (source[len]) ++len; }
    if (result && resultLength >= len) memcpy(result, source, len * sizeof(UChar));
    return len;
}

}  // extern "C"
