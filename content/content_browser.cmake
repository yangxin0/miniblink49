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
file(GLOB CB_RESOURCES "${C}/resources/*.cpp")     # embedded CSS/JS/PNG resource blobs
file(GLOB CB_DEVTOOLS  "${C}/devtools/*.cpp")       # inspector agent/client glue

# WinINet HTTP/cookies (curl/net_portable siblings used instead) and the dead
# WinINet client are dropped on every platform. The Win32 clipboard is used on
# Windows; macOS uses the NSPasteboard sibling (web_impl_mac) and its Cocoa
# web_impl_mac files, so on Windows CB_IMPL_MAC is dropped entirely.
if(MB_OS_WINDOWS)
    list(FILTER CB_IMPL_WIN EXCLUDE REGEX "/(WebURLLoaderImpl|WebCookieJarINetImpl|w3client)\\.cpp$")
    set(CB_IMPL_MAC "")
    # Windows compiles the real NPAPI plugin support (PluginPackage/Database,
    # WebPluginImpl, s_wkeBrowserFuncs) from web_impl_win/npapi/ — macOS stubs it.
    # The mutil_thread_np/ variant is the alternate multi-thread NP impl; skip it
    # to avoid duplicate symbols.
    # Windows UI: popup menus, drag-drop (content/ui/, not under content/browser/).
    file(GLOB CB_UI "${C}/ui/*.cpp")
    list(APPEND CB_IMPL_WIN ${CB_UI})

    file(GLOB CB_NPAPI "${C}/web_impl_win/npapi/*.cpp")
    list(FILTER CB_NPAPI EXCLUDE REGEX "/mutil_thread_np/")
    # NetscapePlugInStreamLoader couples to blink-53 ResourceLoader internals that
    # have API-skewed (not on the core path); defer it. PluginStream is needed by
    # WebPluginImpl, so it's kept.
    list(FILTER CB_NPAPI EXCLUDE REGEX "NetscapePlugInStreamLoader\\.cpp$")
    list(APPEND CB_IMPL_WIN ${CB_NPAPI}
        # Stubs for the optional VIP subsystems the Windows content references but
        # that we don't build (OrigChromeMgr/LayerTreeWrap heavyweight mode; the
        # mbvip printing/pdfium stack; DPI init). The real WkePrinting.cpp /
        # OrigChromeStubs.cpp pull in pdfium + duplicate g_uiThreadHeartbeatCallback.
        "${CMAKE_SOURCE_DIR}/content/web_impl_win/MbWinOptionalStubs.cpp"
        # common::LiveIdDetect (self-contained mbvip helper referenced by content).
        "${CMAKE_SOURCE_DIR}/mbvip/common/LiveIdDetect.cpp")
else()
    list(FILTER CB_IMPL_WIN EXCLUDE REGEX "/(WebURLLoaderImpl|WebCookieJarINetImpl|WebClipboardImpl|w3client)\\.cpp$")
endif()

# (net cookie/loader/websocket now live in the full net_portable archive.)
add_library(content_browser STATIC ${CB_BROWSER} ${CB_IMPL_WIN} ${CB_IMPL_MAC} ${CB_RESOURCES} ${CB_DEVTOOLS})
target_link_libraries(content_browser PUBLIC blink_web net_portable wke_globals)
target_include_directories(content_browser PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/content" "${CMAKE_SOURCE_DIR}/wke"
    "${CMAKE_SOURCE_DIR}/mbvip"   # features/printing/WkePrinting.h (Win printing)
    "${CMAKE_SOURCE_DIR}/third_party/npapi"
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/core"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/config"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/utils")
# Lowest-priority fallback: the POSIX content::WaitableEvent uses base/
# synchronization (Lock/ConditionVariable), whose headers ship only under
# orig_chrome/base. Appended last so root base/ wins for everything else.
target_include_directories(content_browser PRIVATE "${CMAKE_SOURCE_DIR}/orig_chrome")
target_compile_definitions(content_browser PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH V8_REVERSE_JSARGS)
# WebThreadImpl.cpp has its own event/thread shim; opt out of win_compat's worker-
# thread primitives (WaitForSingleObject/CloseHandle/_beginthreadex) to avoid a clash.
target_compile_definitions(content_browser PRIVATE WIN_COMPAT_NO_THREAD_PRIMS)
# We use the lightweight mc compositor, not the heavyweight OrigChrome mode, so
# disable WebPageImpl.cpp's `#pragma comment(lib, "orig_chrome.lib")` (that lib is
# not built; the OrigChrome paths are stubbed).
target_compile_definitions(content_browser PRIVATE NO_USE_ORIG_CHROME)
set_target_properties(content_browser PROPERTIES CXX_STANDARD 14)
# win_compat is the <windows.h> shim for NON-Windows; on Windows the real SDK
# windows.h is used (force-included to match the macOS shim force-include).
if(NOT MB_OS_WINDOWS)
    target_include_directories(content_browser PUBLIC "${CMAKE_SOURCE_DIR}/win_compat")
endif()
if(MSVC)
    # Force-include the Win32 headers trimmed out by WIN32_LEAN_AND_MEAN that the
    # web_impl_win files use: mmsystem (timeBeginPeriod), objbase/ole2 (CoInitializeEx,
    # OleInitialize).
    target_compile_options(content_browser PRIVATE
        "/GR-" "/FIwindows.h" "/FImmsystem.h" "/FIobjbase.h" "/FIole2.h" "/FIcommdlg.h"
        "/FIwinspool.h"   # WkePrinting: PRINTER_INFO_2, DocumentProperties, EnumForms
        "/FIshellapi.h")  # DragHandle: HDROP, DragQueryFile, ShellExecute
else()
    target_compile_options(content_browser PRIVATE
        -fdeclspec -fno-exceptions -fno-rtti -w
        "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
endif()
