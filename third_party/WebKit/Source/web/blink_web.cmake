# blink web (macOS port) — the public-API embedding layer (WebView, WebLocalFrame,
# WebNode, ...) that sits on top of blink_core and that wke ultimately drives.
# Compile-verified static archive (162/175 of web/ compile on macOS arm64).
#
# Defined in a standalone .cmake (included from the root). Needs -DBLINK_IMPLEMENTATION=1
# (marks this as blink-internal code; gates the WebNode::unwrap/constUnwrap helpers
# and the export macros). That flag is scoped to web only: platform/core do not
# need it and a few platform files don't compile with it; the BLINK_IMPLEMENTATION-
# gated bits in shared headers are methods/visibility, not layout, so the archives
# still link together.

set(WEBSRC "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source/web")

file(GLOB BLINK_WEB_SRC "${WEBSRC}/*.cpp" "${WEBSRC}/painting/*.cpp")  # +painting/ContinuousPainter
list(FILTER BLINK_WEB_SRC EXCLUDE REGEX "Test\\.cpp$")
# Deferred: devtools/inspector glue (legacy v8::Debug API), find-in-page
# (TextFinder, guard quirk), NPAPI WebBindings, the worker glue that pulls the
# inspector debugger (v8::NativeWeakMap, removed in 8.7), and WebMutationEvent
# (missing public header). WebKit.cpp (blink::initialize) and WebLocalFrameImpl
# now compile and are included.
# Use the real DevToolsEmulator + WebDevToolsAgentImpl; exclude their *None stub
# variants (would duplicate symbols) and WebDevToolsFrontendImpl (V8 8.7 Delete skew).
list(FILTER BLINK_WEB_SRC EXCLUDE REGEX "DevToolsEmulatorNone|WebDevToolsAgentImplNone|InspectorOverlayImplNone|TextFinder\\.cpp$|WebBindings\\.cpp$|WebEmbeddedWorkerImpl\\.cpp$|WebSharedWorkerImpl\\.cpp$|WebMutationEvent\\.cpp$|WebTestingSupport\\.cpp$")

add_library(blink_web STATIC ${BLINK_WEB_SRC})

target_link_libraries(blink_web PUBLIC blink_core)

target_compile_definitions(blink_web PUBLIC BLINK_IMPLEMENTATION=1)

set_target_properties(blink_web PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_web PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
