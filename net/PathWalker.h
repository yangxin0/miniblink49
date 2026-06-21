/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <wtf/Noncopyable.h>

#if defined(_WIN32)
#include <Windows.h>
#else
// macOS: no Win32 directory-enumeration API. Provide a posix-backed
// equivalent that mirrors the WIN32_FIND_DATAW fields PathWalker exposes.
#include <dirent.h>
#include <wtf/text/WTFString.h>
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif
struct PathWalkerFindDataPosix {
    unsigned long dwFileAttributes;
    WTF::Vector<UChar> cFileNameBuf; // UTF-16, NUL-terminated
    const UChar* cFileName;
};
#endif

namespace WTF {
    class String;
}

namespace net {

class PathWalker {
    WTF_MAKE_NONCOPYABLE(PathWalker);
public:
    PathWalker(const WTF::String& directory, const WTF::String& pattern);
    ~PathWalker();

#if defined(_WIN32)
    bool isValid() const { return m_handle != INVALID_HANDLE_VALUE; }
    const WIN32_FIND_DATAW& data() const { return m_data; }

    bool step();

private:
    HANDLE m_handle;
    WIN32_FIND_DATAW m_data;
#else
    bool isValid() const { return m_dir != nullptr; }
    const PathWalkerFindDataPosix& data() const { return m_data; }

    bool step();

private:
    void fill(struct dirent* entry);

    DIR* m_dir;
    WTF::String m_directory;
    WTF::String m_pattern;
    PathWalkerFindDataPosix m_data;
#endif
};

} // namespace WebCore
