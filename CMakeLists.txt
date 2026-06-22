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
    include(mc/mc.cmake)                                     # compositor (mc::LayerTreeHost) the page host drives
    # NOTE: orig_chrome/content (OrigChromeMgr/LayerTreeWrap) is the OPTIONAL heavyweight
    # "OrigChrome mode" — it pulls the FULL Chromium cc/media/gpu stack. It is only
    # activated via wkeSetDebugConfig("initOrigChromeUiThread"); the default browser uses
    # the lightweight mc/ compositor. So we link no-op stubs (content/web_impl_mac/
    # OrigChromeStubs.cpp) instead of orig_chrome_content. To build the real one:
    #   include(orig_chrome/content/orig_chrome_content.cmake)
    include(content/content_browser.cmake)                   # embedding-host page host (content::WebPageImpl + Cocoa web_impl_mac)
    include(wke/wke.cmake)                                    # wke public C API (libwke surface)

    if(APPLE)
        # The mini-browser: a Cocoa host that drives the wke C API. Links the full
        # engine archive set. force_load wke/wke_globals/bindings_infra so the C-API
        # entry points + blink's ENABLE_WKE hooks are all included.
        # Release V8 monolith. The DOM-wrapper-construction "0x2 heisenbug" that
        # previously forced a dcheck_always_on build turned out to be a symptom of
        # the V8_REVERSE_JSARGS ABI mismatch (blink read callback args backwards,
        # so wrapper constructors saw a corrupt receiver). With V8_REVERSE_JSARGS
        # now defined consistently (see the v8_monolith interface below), the pure
        # release build runs JS-driven pages cleanly (baidu + YouTube app shell),
        # and avoids the dcheck VerifyApiCallResultType aborts entirely.
        set(_v8mono "${MINIBLINK_V8_ROOT}/out/arm64.release/obj/libv8_monolith.a")
        add_executable(minibrowser "${CMAKE_SOURCE_DIR}/port/mac/minibrowser_main.mm")
        set_target_properties(minibrowser PROPERTIES CXX_STANDARD 14)
        target_include_directories(minibrowser PRIVATE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/wke" "${CMAKE_SOURCE_DIR}/win_compat")
        target_compile_options(minibrowser PRIVATE -fdeclspec -fno-exceptions -Wno-deprecated-declarations)
        target_link_libraries(minibrowser PRIVATE
            -Wl,-force_load,$<TARGET_FILE:wke> -Wl,-force_load,$<TARGET_FILE:wke_globals>
            -Wl,-force_load,$<TARGET_FILE:blink_bindings_infra>
            content_browser mc net_portable blink_web blink_modules blink_bindings blink_core
            blink_generated blink_platform blink_heap_asm wtf skia_ext skia harfbuzz
            miniblink_png miniblink_jpeg miniblink_webp miniblink_ots gin base "${_v8mono}"
            -licucore -lxml2 -lxslt -lz -lcurl
            "-framework Cocoa" "-framework CoreText" "-framework CoreGraphics"
            "-framework CoreFoundation" "-framework Foundation" "-framework AppKit" "-framework Carbon")

        # mb_demo: macOS port of weolar/mb-demo's "Estart" launcher demo, driving
        # the same wke C API + engine archive set as minibrowser. Loads the demo's
        # web resources by path at runtime (default baked into the source; override
        # with argv[1]) so no demo assets are vendored into this repo.
        add_executable(mb_demo "${CMAKE_SOURCE_DIR}/port/mac/mb_demo_main.mm")
        set_target_properties(mb_demo PROPERTIES CXX_STANDARD 14)
        target_include_directories(mb_demo PRIVATE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/wke" "${CMAKE_SOURCE_DIR}/win_compat")
        target_compile_options(mb_demo PRIVATE -fdeclspec -fno-exceptions -Wno-deprecated-declarations)
        target_link_libraries(mb_demo PRIVATE
            -Wl,-force_load,$<TARGET_FILE:wke> -Wl,-force_load,$<TARGET_FILE:wke_globals>
            -Wl,-force_load,$<TARGET_FILE:blink_bindings_infra>
            content_browser mc net_portable blink_web blink_modules blink_bindings blink_core
            blink_generated blink_platform blink_heap_asm wtf skia_ext skia harfbuzz
            miniblink_png miniblink_jpeg miniblink_webp miniblink_ots gin base "${_v8mono}"
            -licucore -lxml2 -lxslt -lz -lcurl
            "-framework Cocoa" "-framework CoreText" "-framework CoreGraphics"
            "-framework CoreFoundation" "-framework Foundation" "-framework AppKit" "-framework Carbon")
    endif()

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
        # V8_REVERSE_JSARGS MUST match the monolith (BUILD.gn adds -DV8_REVERSE_JSARGS,
        # but it is a gn build define, NOT in v8config.h). Without it, blink's
        # FunctionCallbackInfo::operator[] reads args in the wrong direction
        # (values_ - i instead of values_ + i): info[0] happens to be right but
        # info[1] returns the receiver/Holder, so every multi-arg DOM constructor
        # (new URL/Event/MouseEvent...) reads garbage and segfaults.
        INTERFACE_COMPILE_DEFINITIONS "V8_COMPRESS_POINTERS;V8_31BIT_SMIS_ON_64BIT_ARCH;V8_REVERSE_JSARGS")
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
