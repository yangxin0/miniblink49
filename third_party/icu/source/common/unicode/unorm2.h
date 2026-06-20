// Minimal unorm2.h for the in-tree ICU (macOS): modern (ICU>=49) normalizer2 subset
// harfbuzz's hb-icu.cc uses; symbols come from libicucore.
#ifndef UNORM2_H
#define UNORM2_H
#include "unicode/utypes.h"
struct UNormalizer2;
typedef struct UNormalizer2 UNormalizer2;
#ifdef __cplusplus
extern "C" {
#endif
const UNormalizer2* unorm2_getNFCInstance(UErrorCode* pErrorCode);
UChar32 unorm2_composePair(const UNormalizer2* norm2, UChar32 a, UChar32 b);
int32_t unorm2_getRawDecomposition(const UNormalizer2* norm2, UChar32 c,
                                   UChar* decomposition, int32_t capacity, UErrorCode* pErrorCode);
#ifdef __cplusplus
}
#endif
#endif
