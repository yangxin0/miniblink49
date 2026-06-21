# content/ embedding-host page-host archive (macOS port).
#
# This is the layer that drives blink: content::WebPageImpl (the page host),
# BlinkPlatformImpl (the blink::Platform implementation), threading/timer/file
# backends, and the Cocoa siblings under content/web_impl_mac/. It resolves the
# content:: link symbols (WebPageImpl::getCookieJar, traceEventSamplingState) that
# blink::initialize / wke reference.
#
# Windows-only variants are excluded in favor of their curl/posix/Cocoa siblings
# (mirrors the miniblink Windows build's file selection on the macOS code path).

set(C "${CMAKE_SOURCE_DIR}/content")

file(GLOB CB_BROWSER   "${C}/browser/*.cpp")
file(GLOB CB_IMPL_WIN  "${C}/web_impl_win/*.cpp")
file(GLOB CB_IMPL_MAC  "${C}/web_impl_mac/*.mm" "${C}/web_impl_mac/*.cpp")

# Windows-only files: WinINet HTTP/cookies (curl siblings used instead), Win32
# clipboard (NSPasteboard sibling instead), and dead WinINet client.
list(FILTER CB_IMPL_WIN EXCLUDE REGEX "/(WebURLLoaderImpl|WebCookieJarINetImpl|WebClipboardImpl|w3client)\\.cpp$")

# net cookie jar + curl URL-loader + websocket that the page host calls into.
set(CB_NET
    "${CMAKE_SOURCE_DIR}/net/cookies/WebCookieJarCurlImpl.cpp"
    "${CMAKE_SOURCE_DIR}/net/websocket/WebSocketChannelImpl.cpp"
    "${CMAKE_SOURCE_DIR}/net/WebURLLoaderManager.cpp"
    "${CMAKE_SOURCE_DIR}/net/WebURLLoaderManagerUtil.cpp")

add_library(content_browser STATIC ${CB_BROWSER} ${CB_IMPL_WIN} ${CB_IMPL_MAC} ${CB_NET})
target_link_libraries(content_browser PUBLIC blink_web net_portable wke_globals)
target_include_directories(content_browser PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/content" "${CMAKE_SOURCE_DIR}/wke"
    "${CMAKE_SOURCE_DIR}/win_compat" "${CMAKE_SOURCE_DIR}/third_party/npapi"
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/core"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/config"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/utils")
target_compile_definitions(content_browser PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(content_browser PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(content_browser PRIVATE
        -fdeclspec -fno-exceptions -fno-rtti -w
        "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
endif()
