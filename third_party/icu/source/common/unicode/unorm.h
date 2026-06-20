// Minimal unorm.h for the in-tree ICU (macOS). harfbuzz includes this but on ICU>=49
// uses unorm2 (below); the legacy UNORM_* path is not compiled.
#ifndef UNORM_H
#define UNORM_H
#include "unicode/utypes.h"
#include "unicode/unorm2.h"
typedef enum {
    UNORM_NONE = 1, UNORM_NFD = 2, UNORM_NFKD = 3, UNORM_NFC = 4,
    UNORM_DEFAULT = UNORM_NFC, UNORM_NFKC = 5, UNORM_FCD = 6
} UNormalizationMode;
#ifdef __cplusplus
extern "C" {
#endif
int32_t unorm_normalize(const UChar* source, int32_t sourceLength, UNormalizationMode mode,
                        int32_t options, UChar* result, int32_t resultLength, UErrorCode* status);
#ifdef __cplusplus
}
#endif
#endif
