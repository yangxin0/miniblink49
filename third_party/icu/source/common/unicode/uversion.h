// Minimal uversion.h for the in-tree (renaming-disabled) ICU used on macOS.
// harfbuzz's hb-icu.cc only needs the ICU major version (>=49 -> modern unorm2 path).
#ifndef UVERSION_H
#define UVERSION_H
#include "unicode/umachine.h"
#define U_ICU_VERSION_MAJOR_NUM 56
#define U_ICU_VERSION_MINOR_NUM 0
#define U_MAX_VERSION_LENGTH 4
typedef uint8_t UVersionInfo[U_MAX_VERSION_LENGTH];
#endif
