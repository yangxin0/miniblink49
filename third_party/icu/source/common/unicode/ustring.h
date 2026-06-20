// Minimal ustring.h for the in-tree ICU (macOS); symbols come from libicucore.
#ifndef USTRING_H
#define USTRING_H
#include "unicode/utypes.h"
#ifdef __cplusplus
extern "C" {
#endif
int32_t u_countChar32(const UChar* s, int32_t length);
UChar32* u_strToUTF32(UChar32* dest, int32_t destCapacity, int32_t* pDestLength,
                      const UChar* src, int32_t srcLength, UErrorCode* pErrorCode);
#ifdef __cplusplus
}
#endif
#endif
