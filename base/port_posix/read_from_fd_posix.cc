// read_from_fd_posix.cc — base::ReadFromFD for the macOS/POSIX port.
//
// Chromium defines this in base/files/file_util_posix.cc, but the trimmed
// in-tree base/ does not ship that (large) file, and pulling it would drag in a
// long tail of posix file-util dependencies. base::ReadFromFD itself is a tiny
// self-contained posix read loop, so we provide it directly here. This matches
// the existing pattern of providing posix companions under base/ rather than
// editing orig_chrome.

#if !defined(_WIN32)

#include "base/files/file_util.h"

#include <errno.h>
#include <unistd.h>

namespace base {

bool ReadFromFD(int fd, char* buffer, size_t bytes)
{
    size_t total_read = 0;
    while (total_read < bytes) {
        ssize_t bytes_read;
        do {
            bytes_read = read(fd, buffer + total_read, bytes - total_read);
        } while (bytes_read < 0 && errno == EINTR);
        if (bytes_read <= 0)
            break;
        total_read += static_cast<size_t>(bytes_read);
    }
    return total_read == bytes;
}

} // namespace base

#endif // !defined(_WIN32)
