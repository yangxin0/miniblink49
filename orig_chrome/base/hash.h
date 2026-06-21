// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_HASH_H_
#define BASE_HASH_H_

#include <limits>
#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/logging.h"

#undef max
#undef min

namespace base {

// WARNING: This hash function should not be used for any cryptographic purpose.
BASE_EXPORT uint32 SuperFastHash(const char* data, int len);

// Computes a hash of a memory buffer |data| of a given |length|.
// WARNING: This hash function should not be used for any cryptographic purpose.
inline uint32 Hash(const char* data, size_t length)
{
    if (length > static_cast<size_t>(std::numeric_limits<int>::max())) {
        NOTREACHED();
        return 0;
    }
    return SuperFastHash(data, static_cast<int>(length));
}

// Computes a hash of a string |str|.
// WARNING: This hash function should not be used for any cryptographic purpose.
inline uint32 Hash(const std::string& str)
{
    return Hash(str.data(), str.size());
}

#if defined(_WIN32)
// Windows build never instantiates the std::pair<GenericSharedMemoryId, ...>
// hash specialization that needs base::HashInts, so it is intentionally absent
// here to keep the Windows path byte-for-byte unchanged.
#else
// On macOS (case-insensitive FS / clang) the gfx GenericSharedMemoryId pair-hash
// specialization is instantiated and requires base::HashInts. Chromium upstream
// defines it in base/hash.h; this codebase only has HashInts32/HashInts64 in
// base/containers/hash_tables.h. Provide a minimal forwarding HashInts here.
// Computes a hash of two integers, dispatching on operand size.
template <typename T1, typename T2>
inline size_t HashInts(T1 value1, T2 value2)
{
    // Mix the two values; sufficient for hash-table bucketing (non-crypto).
    uint64 a = static_cast<uint64>(value1);
    uint64 b = static_cast<uint64>(value2);
    uint64 combined = a * 0x9E3779B97F4A7C15ULL + (b + 0x9E3779B97F4A7C15ULL);
    combined ^= (combined >> 32);
    return static_cast<size_t>(combined);
}
#endif // _WIN32

} // namespace base

#endif // BASE_HASH_H_
