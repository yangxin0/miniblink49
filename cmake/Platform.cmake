# Platform.cmake — single place for OS/compiler detection and per-platform flags.
#
# Defines MB_PLATFORM (Windows | macOS | Linux) and sets sensible defaults so
# every target in the tree gets consistent language level and platform macros.

if(WIN32)
    set(MB_PLATFORM "Windows")
    set(MB_OS_WINDOWS TRUE)
elseif(APPLE)
    set(MB_PLATFORM "macOS")
    set(MB_OS_MACOSX TRUE)
elseif(UNIX)
    set(MB_PLATFORM "Linux")
    set(MB_OS_LINUX TRUE)
else()
    set(MB_PLATFORM "Unknown")
endif()

# Language level — V8 5.7 headers and the engine use C++11/14 idioms.
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 11)
endif()

# Default to a Release build if the user didn't pick one (single-config gens).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# The pinned V8 8.7 monolith is built with the STATIC C runtime (/MT, is_debug=false).
# Every target that links it must use the same CRT or the linker reports duplicate /
# missing CRT symbols, so force the static runtime for all MSVC targets. (CMP0091 is
# NEW because cmake_minimum_required >= 3.16.)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

# Per-platform definitions consumed by ported code via #ifdef.
add_compile_definitions(
    $<$<BOOL:${MB_OS_WINDOWS}>:MB_OS_WINDOWS=1>
    $<$<BOOL:${MB_OS_MACOSX}>:MB_OS_MACOSX=1>
    $<$<BOOL:${MB_OS_LINUX}>:MB_OS_LINUX=1>
)

# miniblink is a Unicode Win32 application: define UNICODE/_UNICODE so the Win32
# API macros resolve to the wide (*W) entry points that the engine's wchar_t code
# paths expect. Without this, CreateFile/VerQueryValue/GetFileVersionInfo/... pick
# the ANSI (*A) variants and reject wchar_t* arguments (this matches what the
# historical miniblink.vcxproj defined).
if(MB_OS_WINDOWS)
    add_compile_definitions(UNICODE _UNICODE)
endif()
