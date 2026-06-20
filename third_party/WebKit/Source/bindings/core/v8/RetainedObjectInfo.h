/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RetainedObjectInfo_h
#define RetainedObjectInfo_h

#include <v8-profiler.h>

#if V8_MAJOR_VERSION >= 8
// V8 8.7 removed the legacy heap-snapshot v8::RetainedObjectInfo API. blink still
// subclasses it (for DOM heap-snapshot grouping). Provide the old interface as a
// compat base so this builds; the devtools heap profiler that drives it is not
// brought up on macOS.
namespace v8 {
class RetainedObjectInfo {
public:
    virtual void Dispose() = 0;
    virtual bool IsEquivalent(RetainedObjectInfo* other) = 0;
    virtual intptr_t GetHash() = 0;
    virtual const char* GetLabel() = 0;
    virtual const char* GetGroupLabel() { return GetLabel(); }
    virtual intptr_t GetElementCount() { return -1; }
    virtual intptr_t GetSizeInBytes() { return -1; }
protected:
    RetainedObjectInfo() {}
    virtual ~RetainedObjectInfo() {}
};
} // namespace v8
#endif

namespace blink {

class RetainedObjectInfo : public v8::RetainedObjectInfo {
public:
    virtual intptr_t GetEquivalenceClass() = 0;
};

} // namespace blink

#endif // RetainedObjectInfo_h
