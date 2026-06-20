// Minimal UTF-16 macros for the in-tree ICU (macOS), self-contained (no SDK deps).
// Standard ICU definitions; only the subset harfbuzz's hb-icu.cc uses.
#ifndef UTF16_H
#define UTF16_H
#include "unicode/umachine.h"

#define U16_IS_LEAD(c)             (((c)&0xfffffc00)==0xd800)
#define U16_IS_TRAIL(c)            (((c)&0xfffffc00)==0xdc00)
#define U16_IS_SURROGATE(c)        (((c)&0xfffff800)==0xd800)
#define U16_IS_SURROGATE_LEAD(c)   (((c)&0x400)==0)
#define U16_SURROGATE_OFFSET       ((0xd800<<10UL)+0xdc00-0x10000)
#define U16_GET_SUPPLEMENTARY(lead, trail) \
    (((UChar32)(lead)<<10UL)+(UChar32)(trail)-U16_SURROGATE_OFFSET)

#define U16_GET_UNSAFE(s, i, c) UPRV_BLOCK_MACRO_BEGIN { \
    (c)=(s)[i]; \
    if(U16_IS_SURROGATE(c)) { \
        if(U16_IS_SURROGATE_LEAD(c)) { (c)=U16_GET_SUPPLEMENTARY((c), (s)[(i)+1]); } \
        else { (c)=U16_GET_SUPPLEMENTARY((s)[(i)-1], (c)); } \
    } } UPRV_BLOCK_MACRO_END

#define U16_NEXT_UNSAFE(s, i, c) UPRV_BLOCK_MACRO_BEGIN { \
    (c)=(s)[(i)++]; \
    if(U16_IS_LEAD(c)) { (c)=U16_GET_SUPPLEMENTARY((c), (s)[(i)++]); } \
    } UPRV_BLOCK_MACRO_END

#define U16_PREV_UNSAFE(s, i, c) UPRV_BLOCK_MACRO_BEGIN { \
    (c)=(s)[--(i)]; \
    if(U16_IS_TRAIL(c)) { (c)=U16_GET_SUPPLEMENTARY((s)[--(i)], (c)); } \
    } UPRV_BLOCK_MACRO_END

#define U16_APPEND(s, i, capacity, c, isError) UPRV_BLOCK_MACRO_BEGIN { \
    if((uint32_t)(c)<=0xffff) { (s)[(i)++]=(uint16_t)(c); } \
    else if((uint32_t)(c)<=0x10ffff && (i)+1<(capacity)) { \
        (s)[(i)++]=(uint16_t)(((c)>>10)+0xd7c0); \
        (s)[(i)++]=(uint16_t)(((c)&0x3ff)|0xdc00); \
    } else { (isError)=true; } } UPRV_BLOCK_MACRO_END

#ifndef UPRV_BLOCK_MACRO_BEGIN
#define UPRV_BLOCK_MACRO_BEGIN do {
#define UPRV_BLOCK_MACRO_END } while (0)
#endif

#endif
