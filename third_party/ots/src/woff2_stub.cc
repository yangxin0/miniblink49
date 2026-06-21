// WOFF2 stub for the macOS port: the real woff2.cc needs the brotli decompressor
// (deferred). ComputeWOFF2FinalSize returns 0 (== error), so ots rejects WOFF2
// fonts gracefully; TTF/OTF/WOFF still sanitize normally.
#include <cstddef>
#include <cstdint>
#include "woff2.h"

namespace ots {

size_t ComputeWOFF2FinalSize(const uint8_t* /*data*/, size_t /*length*/) {
    return 0;  // 0 => error; ots treats the font as not-WOFF2/invalid.
}

bool ConvertWOFF2ToTTF(uint8_t* /*result*/, size_t /*result_length*/,
                       const uint8_t* /*data*/, size_t /*length*/) {
    return false;
}

}  // namespace ots
