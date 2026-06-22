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

# Full net layer: top-level + cookies/ + websocket/ subdirs (the cookie jar,
# cookie monster, curl HTTP manager, websocket stack, blob/file streams). Only
# the genuinely Windows-only files are excluded in favor of posix/curl siblings.
file(GLOB_RECURSE NET_PORTABLE_SRC "${NETSRC}/*.cpp")
list(FILTER NET_PORTABLE_SRC EXCLUDE REGEX "Test\\.cpp$")
# Windows-only: WinINet HTTP loader + Win32 file enumeration / Shlwapi path helpers.
# WebURLLoaderWinINet is dropped on every platform (curl is used). The FileSystem
# backend is platform-specific: Windows keeps FileSystemWin, macOS FileSystemPosix.
if(MB_OS_WINDOWS)
    list(FILTER NET_PORTABLE_SRC EXCLUDE REGEX "/(WebURLLoaderWinINet|FileSystemPosix)\\.cpp$")
else()
    list(FILTER NET_PORTABLE_SRC EXCLUDE REGEX "/(WebURLLoaderWinINet|FileSystemWin)\\.cpp$")
endif()
# Missing upstream dependencies (not Win32-specific): SSLHandle needs the legacy
# WebKit ResourceHandle/ResourceHandleInternal classes (absent from miniblink,
# which uses WebURLLoaderInternal), and SharedMemoryReceivedDataFactory needs
# Chromium's content/child/shared_memory_received_data_factory.h + resource_messages.h
# (not vendored here). Neither file is referenced by any other net source.
list(FILTER NET_PORTABLE_SRC EXCLUDE REGEX "/(SSLHandle|SharedMemoryReceivedDataFactory)\\.cpp$")

add_library(net_portable STATIC ${NET_PORTABLE_SRC})
target_link_libraries(net_portable PUBLIC blink_web)
target_include_directories(net_portable PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/wke"
    "${CMAKE_SOURCE_DIR}/third_party/libcurl_7.69/include" "${CMAKE_SOURCE_DIR}/third_party/npapi"
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/core" "${CMAKE_SOURCE_DIR}/third_party/skia/include/config")
if(NOT MB_OS_WINDOWS)
    target_include_directories(net_portable PUBLIC "${CMAKE_SOURCE_DIR}/win_compat")
endif()
target_compile_definitions(net_portable PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH V8_REVERSE_JSARGS)
if(MB_OS_WINDOWS)
    # The bundled libcurl is a static lib; consumers must define CURL_STATICLIB so
    # curl.h declares the functions normally (not __declspec(dllimport)/__imp_).
    # PUBLIC so wke (which also includes curl.h) inherits it.
    target_compile_definitions(net_portable PUBLIC CURL_STATICLIB)
endif()
# net uses Win32 types but relies on Windows PCH inclusion; force-include the shim.
# SHELL: keeps the "-include <path>" pair together — CMake otherwise de-duplicates
# the bare "-include" token against the inherited "-include config.h", orphaning
# config.h as a stray input ("cannot specify -o when generating multiple files").
if(MSVC)
    # Real SDK windows.h + shellapi.h (ShellExecute, trimmed out by WIN32_LEAN_AND_MEAN).
    target_compile_options(net_portable PRIVATE "/FIwindows.h" "/FIshellapi.h")
else()
    target_compile_options(net_portable PRIVATE
        "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
endif()
set_target_properties(net_portable PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(net_portable PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
