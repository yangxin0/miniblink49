# wke — miniblink's public C API (the libwke surface). wke.cpp/wke2.cpp are the API
# entry points; wkeJsBind (JS bindings), wkeNetHook (net hooks), wkeWebView/
# wkeWebWindow (the view/window objects). wkeGlobalVar + wkeString already live in
# the wke_globals archive (blink links those), so they're excluded here.

set(WKED "${CMAKE_SOURCE_DIR}/wke")
file(GLOB WKE_SRC "${WKED}/*.cpp")
list(FILTER WKE_SRC EXCLUDE REGEX "/(wkeGlobalVar|wkeString|CurlVsetoptForwardMac)\\.cpp$")  # in wke_globals

if(MB_OS_WINDOWS)
    # Link-only stubs for the PDF print path (skia src/pdf + mbvip printing plugin
    # aren't built); printing is non-core. (macOS has sk_document_pdf_stub_mac.)
    list(APPEND WKE_SRC "${CMAKE_SOURCE_DIR}/skia/ext/sk_pdf_printing_stub_win.cpp")
endif()
add_library(wke STATIC ${WKE_SRC})
target_link_libraries(wke PUBLIC content_browser mc net_portable)
target_include_directories(wke PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/wke" "${CMAKE_SOURCE_DIR}/orig_chrome"
    "${CMAKE_SOURCE_DIR}/mbvip"   # features/printing/PdfViewerPluginFunc.h (Win-only include)
    "${CMAKE_SOURCE_DIR}/mc" "${CMAKE_SOURCE_DIR}/electron"
    "${CMAKE_SOURCE_DIR}/third_party/npapi" "${CMAKE_SOURCE_DIR}/third_party/v8shim"
    "${CMAKE_SOURCE_DIR}/third_party/khronos" "${CMAKE_SOURCE_DIR}/third_party/libcurl_7.69/include"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/core"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/config"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/utils")
if(NOT MB_OS_WINDOWS)
    target_include_directories(wke PUBLIC "${CMAKE_SOURCE_DIR}/win_compat")  # <windows.h> shim
endif()
target_compile_definitions(wke PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH V8_REVERSE_JSARGS)
set_target_properties(wke PROPERTIES CXX_STANDARD 14)
if(MSVC)
    # Real SDK windows.h + the headers WIN32_LEAN_AND_MEAN trims that wke uses.
    # ole2.h/objbase.h are force-included EARLY (before any `using namespace` in the
    # wke sources) so oaidl.h's DOUBLE binds to ::DOUBLE unambiguously, not a
    # namespace DOUBLE pulled in later (C2872); ole2.h also provides RevokeDragDrop.
    target_compile_options(wke PRIVATE
        "/GR-" "/FIwindows.h" "/FImmsystem.h" "/FIshellapi.h" "/FIcommdlg.h"
        "/FIobjbase.h" "/FIole2.h" "/FIwinspool.h")  # winspool: PRINTER_INFO_2 (mbvip printing)
else()
    target_compile_options(wke PRIVATE
        -fdeclspec -fno-exceptions -fno-rtti -w
        "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
endif()
