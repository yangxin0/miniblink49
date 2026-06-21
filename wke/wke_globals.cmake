# wke embedding-layer globals (macOS port) — the portable slice of the wke public
# API that blink itself references at link time. miniblink patched blink (ENABLE_WKE=1)
# to read wke globals: navigator.* overrides (g_navigatorAppName/Language/...),
# window outer size (g_outerWidth/Height), the ImageBuffer->dataURL callback hooks,
# the media-player factory, etc. wkeGlobalVar.cpp defines all of these.
#
# The rest of the wke layer (wkeString, wke.cpp/wke2/wkeJsBind, and the windowing
# files wkeWebView/wkeWebWindow that need a Cocoa backend) is the later embedding +
# windowing bring-up; only these definitions are needed to LINK blink::initialize.

add_library(wke_globals STATIC
    "${CMAKE_SOURCE_DIR}/wke/wkeGlobalVar.cpp"
    "${CMAKE_SOURCE_DIR}/wke/wkeString.cpp")   # wke::CString (UTF-32 wchar_t ported)
target_link_libraries(wke_globals PUBLIC blink_platform)
target_include_directories(wke_globals PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/wke"
    "${CMAKE_SOURCE_DIR}/win_compat" "${CMAKE_SOURCE_DIR}/third_party/npapi"
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos")
target_compile_definitions(wke_globals PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(wke_globals PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(wke_globals PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
