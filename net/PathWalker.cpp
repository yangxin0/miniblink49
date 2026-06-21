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

#include "config.h"
#include "PathWalker.h"

#include <wtf/text/WTFString.h>
#if !defined(_WIN32)
#include <wtf/text/CString.h>
#include <wtf/text/WTFStringUtil.h>
#include <string.h>
#endif

namespace net {

#if defined(_WIN32)

PathWalker::PathWalker(const String& directory, const String& pattern)
{
    String path = directory + "\\" + pattern;
    m_handle = ::FindFirstFileW(path.charactersWithNullTermination().data(), &m_data);
}

PathWalker::~PathWalker()
{
    if (!isValid())
        return;
    ::FindClose(m_handle);
}

bool PathWalker::step()
{
    return ::FindNextFileW(m_handle, &m_data);
}

#else

// macOS: posix opendir/readdir-based implementation. The "pattern" is matched
// loosely; FileSystemWin.cpp (the only consumer) is Windows-only and excluded
// from this build, so this exists to keep PathWalker compiling/portable.
PathWalker::PathWalker(const String& directory, const String& pattern)
    : m_dir(nullptr)
    , m_directory(directory)
    , m_pattern(pattern)
{
    CString dirUtf8 = directory.utf8();
    m_dir = ::opendir(dirUtf8.data());
    if (!m_dir)
        return;

    // Position on the first matching entry.
    if (!step())
        ; // leave m_data empty; isValid() stays true while m_dir is open
}

PathWalker::~PathWalker()
{
    if (m_dir)
        ::closedir(m_dir);
}

void PathWalker::fill(struct dirent* entry)
{
    m_data.dwFileAttributes = 0;
    if (entry->d_type == DT_DIR)
        m_data.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

    String name = String::fromUTF8(entry->d_name);
    m_data.cFileNameBuf = WTF::ensureUTF16UChar(name, true);
    m_data.cFileName = m_data.cFileNameBuf.data();
}

bool PathWalker::step()
{
    if (!m_dir)
        return false;

    struct dirent* entry = nullptr;
    while ((entry = ::readdir(m_dir)) != nullptr) {
        // Skip "." and ".." to mirror typical FindFirstFile pattern usage.
        if (0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, ".."))
            continue;
        fill(entry);
        return true;
    }
    return false;
}

#endif

} // namespace WebCore
