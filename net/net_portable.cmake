# net (macOS port) — the portable subset of miniblink's net/ layer (storage,
# file streams, data: URLs, multipart, curl cache, shared-memory data handles).
# Compile-verified static archive on macOS arm64.
#
# Deferred (Windows backend / per-file Win32 port still to do): the curl HTTP
# manager (WebURLLoaderManager* — needs the shlobj/shlwapi path helpers + exe-
# path logic ported), WinINet HTTP (WebURLLoaderWinINet), Win32 file enumeration
# (FileSystemWin/PathWalker -> dirent), Shlwapi path defaults (DefaultFullPath/
# PageNetExtraData), the long-width Interlocked cast (ActivatingObjCheck), the
# content/ shared-memory factory, and SSLHandle (missing header). The blink-
# dependent net pieces ride on blink_platform's flags; net uses Win32 types via
# the win_compat shim (force-included).

set(NETSRC "${CMAKE_SOURCE_DIR}/net")

file(GLOB NET_PORTABLE_SRC "${NETSRC}/*.cpp")
list(FILTER NET_PORTABLE_SRC EXCLUDE REGEX "Test\\.cpp$")
list(FILTER NET_PORTABLE_SRC EXCLUDE REGEX "ActivatingObjCheck|BlobResourceLoader|CurlCacheEntry|DefaultFullPath|FileSystemWin|PageNetExtraData|PathWalker|SharedMemoryReceivedDataFactory|SSLHandle|WebURLLoaderManager|WebURLLoaderWinINet")

add_library(net_portable STATIC ${NET_PORTABLE_SRC})
target_link_libraries(net_portable PUBLIC blink_platform)
target_include_directories(net_portable PUBLIC "${CMAKE_SOURCE_DIR}/win_compat" "${CMAKE_SOURCE_DIR}/third_party/libcurl_7.69/include")
# net uses Win32 types but relies on Windows PCH inclusion; force-include the shim.
# SHELL: keeps the "-include <path>" pair together — CMake otherwise de-duplicates
# the bare "-include" token against the inherited "-include config.h", orphaning
# config.h as a stray input ("cannot specify -o when generating multiple files").
target_compile_options(net_portable PRIVATE
    "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
set_target_properties(net_portable PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(net_portable PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
