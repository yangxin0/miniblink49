// FileSystemPosix.cpp — POSIX/macOS implementation of the cross-platform net::
// file API declared in net/FileSystem.h. Mirrors net/FileSystemWin.cpp (the
// Windows backend, excluded from this build) using posix open/read/write/lseek/
// unlink/access/mkdir/stat.
//
// Only the file primitives that have no portable sibling elsewhere in net/ are
// implemented here, matching the exact set of symbols the macOS link needs:
// openFile, openFileEx, closeFile, readFromFile (char*), writeToFile, seekFile,
// deleteFile, fileExists, createDirectory, makeAllDirectories,
// getFileSize (path + handle), getFileModificationTime.
//
// PlatformFileHandle is a void* (HMODULE/HANDLE on Windows). On POSIX we encode
// the integer fd inside the void*: handle = (void*)(intptr_t)(fd + 1) so that a
// valid fd 0 (stdin is never returned here, but keep it distinct from the
// invalid sentinel) maps to a non-(-1) pointer. invalidPlatformFileHandle is
// (void*)-1 (see FileSystem.h), so we keep the +1 bias to stay clear of it.

#if !defined(_WIN32)

#include "net/FileSystem.h"

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace net {

// --- fd <-> PlatformFileHandle (void*) encoding ---------------------------

static inline PlatformFileHandle fdToHandle(int fd)
{
    if (fd < 0)
        return invalidPlatformFileHandle;
    // +1 bias so fd 0 is distinguishable from a zero pointer, and never -1.
    return reinterpret_cast<PlatformFileHandle>(static_cast<intptr_t>(fd) + 1);
}

static inline int handleToFd(PlatformFileHandle handle)
{
    if (handle == invalidPlatformFileHandle || !handle)
        return -1;
    return static_cast<int>(reinterpret_cast<intptr_t>(handle) - 1);
}

// Convert a WTF::String path to a UTF-8 filesystem byte string.
static CString fileSystemPath(const String& path)
{
    return path.utf8();
}

// --- open / close ---------------------------------------------------------

PlatformFileHandle openFile(const String& path, FileOpenMode mode)
{
    int flags = 0;
    switch (mode) {
    case OpenForRead:
        flags = O_RDONLY;
        break;
    case OpenForWrite:
        flags = O_WRONLY | O_CREAT | O_TRUNC;
        break;
    default:
        return invalidPlatformFileHandle;
    }

    CString fsPath = fileSystemPath(path);
    int fd = ::open(fsPath.data(), flags, 0644);
    return fdToHandle(fd);
}

PlatformFileHandle openFileEx(const String& path, FileOpenMode mode, FileCreateMode createMode)
{
    int flags = 0;
    switch (mode) {
    case OpenForRead:
        flags = O_RDONLY;
        break;
    case OpenForWrite:
        flags = O_WRONLY;
        break;
    default:
        return invalidPlatformFileHandle;
    }

    // Map the Win32-style FileCreateMode onto posix open flags.
    switch (createMode) {
    case kCreateNew:
        flags |= O_CREAT | O_EXCL;
        break;
    case kCreateAlways:
        flags |= O_CREAT | O_TRUNC;
        break;
    case kOpenExisting:
        // no extra flags
        break;
    case kOpenAlways:
        flags |= O_CREAT;
        break;
    case kTruncateExisting:
        flags |= O_TRUNC;
        break;
    default:
        break;
    }

    CString fsPath = fileSystemPath(path);
    int fd = ::open(fsPath.data(), flags, 0644);
    return fdToHandle(fd);
}

void closeFile(PlatformFileHandle& handle)
{
    int fd = handleToFd(handle);
    if (fd >= 0)
        ::close(fd);
    handle = invalidPlatformFileHandle;
}

// --- seek / read / write --------------------------------------------------

long long seekFile(PlatformFileHandle handle, long long offset, FileSeekOrigin origin)
{
    int fd = handleToFd(handle);
    if (fd < 0)
        return -1;

    int whence = SEEK_SET;
    if (origin == SeekFromCurrent)
        whence = SEEK_CUR;
    else if (origin == SeekFromEnd)
        whence = SEEK_END;

    off_t result = ::lseek(fd, static_cast<off_t>(offset), whence);
    if (result == static_cast<off_t>(-1))
        return -1;
    return static_cast<long long>(result);
}

int writeToFile(PlatformFileHandle handle, const char* data, int length)
{
    int fd = handleToFd(handle);
    if (fd < 0)
        return -1;

    int totalWritten = 0;
    while (totalWritten < length) {
        ssize_t written = ::write(fd, data + totalWritten, length - totalWritten);
        if (written < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (written == 0)
            break;
        totalWritten += static_cast<int>(written);
    }
    return totalWritten;
}

int readFromFile(PlatformFileHandle handle, char* data, int length)
{
    int fd = handleToFd(handle);
    if (fd < 0)
        return -1;

    ssize_t bytesRead;
    do {
        bytesRead = ::read(fd, data, length);
    } while (bytesRead < 0 && errno == EINTR);

    if (bytesRead < 0)
        return -1;
    return static_cast<int>(bytesRead);
}

// --- existence / delete / size / mtime ------------------------------------

bool fileExists(const String& path)
{
    CString fsPath = fileSystemPath(path);
    return ::access(fsPath.data(), F_OK) == 0;
}

bool deleteFile(const String& path)
{
    CString fsPath = fileSystemPath(path);
    return ::unlink(fsPath.data()) == 0;
}

bool getFileSize(const String& path, long long& size)
{
    CString fsPath = fileSystemPath(path);
    struct stat fileInfo;
    if (::stat(fsPath.data(), &fileInfo) != 0)
        return false;
    size = static_cast<long long>(fileInfo.st_size);
    return true;
}

bool getFileSize(PlatformFileHandle handle, long long& size)
{
    int fd = handleToFd(handle);
    if (fd < 0)
        return false;
    struct stat fileInfo;
    if (::fstat(fd, &fileInfo) != 0)
        return false;
    size = static_cast<long long>(fileInfo.st_size);
    return true;
}

bool getFileModificationTime(const String& path, time_t& time)
{
    CString fsPath = fileSystemPath(path);
    struct stat fileInfo;
    if (::stat(fsPath.data(), &fileInfo) != 0)
        return false;
    time = fileInfo.st_mtime;
    return true;
}

// --- directories ----------------------------------------------------------

bool createDirectory(const String& path)
{
    CString fsPath = fileSystemPath(path);
    return ::mkdir(fsPath.data(), 0777) == 0;
}

bool makeAllDirectories(const String& path)
{
    CString fsPath = fileSystemPath(path);
    const char* cpath = fsPath.data();
    if (!cpath || !cpath[0])
        return false;

    // Walk each path component, creating directories as needed.
    String working(cpath);
    Vector<char> buffer(fsPath.length() + 1);
    memcpy(buffer.data(), cpath, fsPath.length() + 1);

    for (size_t i = 1; i < fsPath.length(); ++i) {
        if (buffer[i] == '/') {
            buffer[i] = '\0';
            if (::mkdir(buffer.data(), 0777) != 0 && errno != EEXIST)
                return false;
            buffer[i] = '/';
        }
    }
    if (::mkdir(buffer.data(), 0777) != 0 && errno != EEXIST)
        return false;
    return true;
}

} // namespace net

#endif // !defined(_WIN32)
