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
    LANGUAGES C CXX ASM
    DESCRIPTION "miniblink — single-file Chromium-based browser widget")

# Objective-C++ for the macOS platform backend (port/mac). Enabled separately so
# non-Apple builds don't require it.
if(APPLE)
    enable_language(OBJCXX)
endif()

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
# ---- macOS port options (all off by default; default build stays green) ------
option(MINIBLINK_BUILD_BASE  "Build the engine base/ library (macOS port WIP)" OFF)
option(MINIBLINK_BUILD_GIN   "Build the gin V8 integration layer (macOS port WIP)" OFF)
option(MINIBLINK_BUILD_WTF   "Build WebKit/WTF foundation lib (macOS port WIP)" OFF)
option(MINIBLINK_BUILD_SKIA  "Build skia 2D graphics lib (macOS raster port WIP)" OFF)
option(MINIBLINK_BUILD_BLINK "Build blink platform (geometry first) (macOS port WIP)" OFF)

# Resolve inter-target dependencies before adding any subdirectory:
#   gin   -> base ;  blink platform -> WTF + skia
if(MINIBLINK_BUILD_BLINK)
    set(MINIBLINK_BUILD_WTF ON)
    set(MINIBLINK_BUILD_SKIA ON)
endif()

if(MINIBLINK_BUILD_BASE OR MINIBLINK_BUILD_GIN)
    add_subdirectory(base)   # gin depends on base
endif()
if(MINIBLINK_BUILD_GIN)
    add_subdirectory(gin)
endif()
if(MINIBLINK_BUILD_WTF)
    add_subdirectory(third_party/WebKit/Source/wtf)
endif()
if(MINIBLINK_BUILD_SKIA)
    add_subdirectory(third_party/skia)
    add_subdirectory(test/skia)   # smoke test: prove raster rendering works
    if(APPLE)
        add_subdirectory(port/mac)    # macOS platform backend (paint bridge first)
        add_subdirectory(test/mac)    # smoke test: skia -> CoreGraphics bridge
    endif()
endif()
if(MINIBLINK_BUILD_BLINK)
    include(third_party/harfbuzz-ng/harfbuzz.cmake)          # OpenType text shaper (blink_platform links it)
    include(third_party/image_codecs.cmake)                  # libpng/libjpeg/libwebp (image-decoders/encoders)
    include(third_party/skia/skia_ext.cmake)                  # skia/ext mac bridge (theme)
    add_subdirectory(third_party/WebKit/Source/platform)
    include(third_party/WebKit/Source/core/blink_core.cmake)  # blink_core archive
    include(third_party/WebKit/Source/web/blink_web.cmake)    # blink_web embedding layer
    include(gen/blink_generated.cmake)                       # generated *Names/Features tables
    include(gen/blink_bindings.cmake)                        # generated V8 DOM-wrapper bindings
    include(third_party/WebKit/Source/bindings/blink_bindings_infra.cmake)  # hand-written V8 glue
    include(third_party/WebKit/Source/modules/blink_modules.cmake)  # web-platform feature modules
    include(net/net_portable.cmake)                          # portable net/ subset
    include(wke/wke_globals.cmake)                            # wke embedding-layer globals blink links against
    include(content/content_browser.cmake)                   # embedding-host page host (content::WebPageImpl + Cocoa web_impl_mac)
    add_subdirectory(test/blink)  # smoke test: prove geometry links + computes

    if(APPLE)
        # macOS backend (part 3): NSEvent -> blink WebInputEvent. Blink-coupled
        # (WebInputEvent uses IntRect under INSIDE_BLINK), so it inherits
        # blink_platform's flags/includes.
        add_library(miniblink_mac_input STATIC
            "${CMAKE_SOURCE_DIR}/port/mac/web_input_event_mac.mm")
        target_link_libraries(miniblink_mac_input PUBLIC blink_platform "-framework Cocoa")
        target_include_directories(miniblink_mac_input PUBLIC "${CMAKE_SOURCE_DIR}")
        set_target_properties(miniblink_mac_input PROPERTIES CXX_STANDARD 14)

        add_executable(web_input_event_mac_smoketest
            "${CMAKE_SOURCE_DIR}/test/mac/web_input_event_mac_smoketest.mm")
        target_link_libraries(web_input_event_mac_smoketest PRIVATE miniblink_mac_input)
        set_target_properties(web_input_event_mac_smoketest PROPERTIES CXX_STANDARD 14)
        add_test(NAME web_input_event_mac_smoketest COMMAND web_input_event_mac_smoketest)
    endif()
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
