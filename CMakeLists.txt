# miniblink49 — cross-platform (CMake) build entry point
#
# Historically miniblink49 builds ONLY with Visual Studio (build/miniblink.sln,
# 182 .vcxproj files). This CMake tree is the cross-platform build method,
# introduced to let the engine build on both Windows and macOS.
#
# JavaScript engine: a SINGLE pinned V8 (8.7.220.3) is shared by both platforms,
# replacing the old in-tree vendored V8 copies (v8_4_5 .. v8_7_5, now removed).
# V8 is built from source with its own toolchain via tools/build-v8-8.7-macos.sh
# (and the equivalent on Windows); see BUILD_CROSSPLATFORM.md.
#
# Status (see BUILD_CROSSPLATFORM.md for the full roadmap):
#   * V8 8.7 monolith ... BUILDS + RUNS native arm64 on macOS (verified)
#   * engine (wke/content/net/cc/...) ... NOT yet ported (still Win32)
#
# A target is only listed here once it actually compiles.

cmake_minimum_required(VERSION 3.16)

project(miniblink
    LANGUAGES C CXX
    DESCRIPTION "miniblink — single-file Chromium-based browser widget")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Platform)

# ---- V8 location ------------------------------------------------------------
# Point this at the V8 checkout built by tools/build-v8-8.7-macos.sh.
set(MINIBLINK_V8_ROOT "$ENV{HOME}/build/v8-8.7/v8" CACHE PATH
    "Path to the pinned V8 8.7 checkout (contains include/ and out/<cpu>.release)")
set(MINIBLINK_V8_OUT "out/${CMAKE_SYSTEM_PROCESSOR}.release" CACHE STRING
    "V8 build output subdir under MINIBLINK_V8_ROOT")

# ---- Output layout ----------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

enable_testing()

# ---- Engine base library (port in progress) ---------------------------------
option(MINIBLINK_BUILD_BASE "Build the engine base/ library (macOS port WIP)" OFF)
option(MINIBLINK_BUILD_GIN  "Build the gin V8 integration layer (macOS port WIP)" OFF)
if(MINIBLINK_BUILD_BASE OR MINIBLINK_BUILD_GIN)
    add_subdirectory(base)   # gin depends on base
endif()
if(MINIBLINK_BUILD_GIN)
    add_subdirectory(gin)
endif()

option(MINIBLINK_BUILD_WTF "Build WebKit/WTF foundation lib (macOS port WIP)" OFF)
if(MINIBLINK_BUILD_WTF)
    add_subdirectory(third_party/WebKit/Source/wtf)
endif()

option(MINIBLINK_BUILD_SKIA "Build skia 2D graphics lib (macOS raster port WIP)" OFF)
if(MINIBLINK_BUILD_SKIA)
    add_subdirectory(third_party/skia)
    add_subdirectory(test/skia)   # smoke test: prove raster rendering works
endif()

# ---- Imported V8 monolith target -------------------------------------------
set(_v8_lib "${MINIBLINK_V8_ROOT}/${MINIBLINK_V8_OUT}/obj/libv8_monolith.a")
set(_v8_inc "${MINIBLINK_V8_ROOT}/include")
if(EXISTS "${_v8_lib}")
    add_library(v8_monolith STATIC IMPORTED GLOBAL)
    set_target_properties(v8_monolith PROPERTIES
        IMPORTED_LOCATION "${_v8_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${_v8_inc}"
        # Must match the ABI the monolith was built with (pointer compression).
        INTERFACE_COMPILE_DEFINITIONS "V8_COMPRESS_POINTERS;V8_31BIT_SMIS_ON_64BIT_ARCH")
    set(MINIBLINK_HAVE_V8 TRUE)

    # Smoke test: prove the engine links + runs JavaScript through CMake.
    add_subdirectory(test/v8)
else()
    set(MINIBLINK_HAVE_V8 FALSE)
    message(WARNING
        "V8 monolith not found at:\n  ${_v8_lib}\n"
        "Build it first with tools/build-v8-8.7-macos.sh "
        "(or set -DMINIBLINK_V8_ROOT=...).")
endif()

# ---- Configuration summary --------------------------------------------------
message(STATUS "")
message(STATUS "==== miniblink cross-platform build ====")
message(STATUS "  Platform   : ${MB_PLATFORM} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "  Compiler   : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  V8 8.7     : ${MINIBLINK_HAVE_V8}  (${_v8_lib})")
message(STATUS "========================================")
message(STATUS "")
