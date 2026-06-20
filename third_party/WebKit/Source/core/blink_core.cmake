# blink core (macOS port) — the DOM/HTML/CSS/layout engine on top of
# blink_platform. Compile-verified static archive: each listed source compiles
# on macOS clang against V8 8.7. Full-binary linkage (wke, the deferred wrapper-
# tracing GC, generated bindings .cpp) is a later step; as an archive this is
# sound — consumers pull only referenced objects.
#
# Defined in a standalone .cmake (included from the root) so it doesn't disturb
# the legacy core/CMakeLists.txt. Inherits the Oilpan/ICU/INSIDE_BLINK flags +
# skia/wtf/icu links from blink_platform, and adds the core-specific bits: the
# v8-debug shim, in-tree khronos GL headers, -DV8CALL= (empty calling-convention
# macro) and -DENABLE_WKE=1 (miniblink patched blink's bindings to call its wke
# embedding layer).

set(CSRC "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source/core")

# The DOM/HTML/CSS/layout/events/frame/page subdirs all compile on macOS off the
# same V8 8.7 + Oilpan migration. Glob each and drop the per-dir stragglers:
# gtest helpers, non-mac platform themes, GPU/plugin and a couple of API-skew
# files (handled in a later pass).
# GLOB_RECURSE to also pick up the nested subdirs (css/resolver, layout/svg,
# layout/line, dom/custom, paint/..., svg/graphics, ...) — ~412 files the earlier
# non-recursive globs missed, a large slice of the link gap.
file(GLOB_RECURSE BLINK_CORE_SRC
    "${CSRC}/dom/*.cpp"   "${CSRC}/html/*.cpp"  "${CSRC}/css/*.cpp"
    "${CSRC}/layout/*.cpp" "${CSRC}/events/*.cpp" "${CSRC}/frame/*.cpp"
    "${CSRC}/page/*.cpp"  "${CSRC}/style/*.cpp" "${CSRC}/animation/*.cpp"
    "${CSRC}/loader/*.cpp" "${CSRC}/fetch/*.cpp" "${CSRC}/timing/*.cpp"
    "${CSRC}/fileapi/*.cpp" "${CSRC}/xml/*.cpp" "${CSRC}/clipboard/*.cpp"
    "${CSRC}/svg/*.cpp" "${CSRC}/paint/*.cpp" "${CSRC}/editing/*.cpp"
    "${CSRC}/workers/*.cpp" "${CSRC}/streams/*.cpp" "${CSRC}/xmlhttprequest/*.cpp"
    "${CSRC}/input/*.cpp" "${CSRC}/imagebitmap/*.cpp"
    "${CSRC}/inspector/*.cpp")   # recovered via the deferred-file workflow

# Unit tests + *TestHelper.
list(FILTER BLINK_CORE_SRC EXCLUDE REGEX "Test\\.cpp$|TestHelper\\.cpp$")
# html: plugin element (GPU/plugin host) + a generated-in straggler.
list(FILTER BLINK_CORE_SRC EXCLUDE REGEX "HTMLPlugInElement\\.cpp$|HTMLMetaElement-in\\.cpp$")
# nested-subdir stragglers: API skews / platform variants (HTMLParserScheduler
# recovered by the workflow).
list(FILTER BLINK_CORE_SRC EXCLUDE REGEX "CustomElementNone\\.cpp$|FileInputType\\.cpp$|CanvasRenderingContextFactory\\.cpp$")
# layout: non-mac platform themes (we keep LayoutThemeMac). LayoutText/LayoutReplaced recovered.
list(FILTER BLINK_CORE_SRC EXCLUDE REGEX "LayoutTheme(Android|Default|Linux|Win|FontProviderWin)\\.cpp$")
# paint: non-mac theme painter (we keep the mac path).
list(FILTER BLINK_CORE_SRC EXCLUDE REGEX "ThemePainterDefault\\.cpp$")
# workers: threading-proxy files with API skews (deferred).
list(FILTER BLINK_CORE_SRC EXCLUDE REGEX "WorkerMessagingProxy\\.cpp$|WorkerThread\\.cpp$")

add_library(blink_core STATIC ${BLINK_CORE_SRC})

target_link_libraries(blink_core PUBLIC blink_platform)

target_include_directories(blink_core PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/v8shim"     # v8-debug.h compatibility shim
    "${CMAKE_SOURCE_DIR}/third_party/khronos"    # in-tree GLES2/GLES3/EGL headers
    "${CMAKE_SOURCE_DIR}/win_compat"             # <windows.h> shim (wke API surface)
    "${CMAKE_SOURCE_DIR}/third_party/npapi")     # bindings/npapi.h (HTMLPlugInElement)

target_compile_definitions(blink_core PUBLIC "V8CALL=" ENABLE_WKE=1)

set_target_properties(blink_core PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_core PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
