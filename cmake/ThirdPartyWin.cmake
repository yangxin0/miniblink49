# ThirdPartyWin.cmake — build the bundled third-party dependency libraries from
# the in-tree sources on Windows. macOS links the system versions
# (-lz -lxml2 -lxslt -lcurl); Windows has none, so build them here and link them
# into the deliverables (wkexe / minibrowser / the DLL).

# --- zlib (inflate/deflate; pulled by libxml, ots, skia's SkFlate) ----------
file(GLOB _mb_zlib_src "${CMAKE_SOURCE_DIR}/third_party/zlib/*.c")
add_library(mb_zlib STATIC ${_mb_zlib_src})
target_include_directories(mb_zlib PUBLIC "${CMAKE_SOURCE_DIR}/third_party/zlib")
target_compile_definitions(mb_zlib PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)

# --- libxml2 (XMLDocumentParser, TransformSource) ---------------------------
file(GLOB _mb_xml_src "${CMAKE_SOURCE_DIR}/third_party/libxml/src/*.c")
list(FILTER _mb_xml_src EXCLUDE REGEX "(testdso|runtest|runsuite|testapi|trionan|xmllint|xmlcatalog|test[A-Za-z]*)\\.c$")
add_library(mb_libxml STATIC ${_mb_xml_src})
target_include_directories(mb_libxml PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/libxml/win32/include"
    "${CMAKE_SOURCE_DIR}/third_party/libxml/src/include")
# libxml.h includes "config.h" (the autotools build config); the Windows one lives
# in win32/. PRIVATE so it doesn't leak to libxml's consumers.
target_include_directories(mb_libxml PRIVATE "${CMAKE_SOURCE_DIR}/third_party/libxml/win32")
target_compile_definitions(mb_libxml PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
target_link_libraries(mb_libxml PUBLIC mb_zlib)

# --- libxslt (XSLTProcessor) ------------------------------------------------
file(GLOB _mb_xslt_src "${CMAKE_SOURCE_DIR}/third_party/libxslt/libxslt/*.c")
add_library(mb_libxslt STATIC ${_mb_xslt_src})
target_include_directories(mb_libxslt PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/libxslt"
    "${CMAKE_SOURCE_DIR}/third_party/libxml/win32/include"
    "${CMAKE_SOURCE_DIR}/third_party/libxml/src/include")
target_compile_definitions(mb_libxslt PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
target_link_libraries(mb_libxslt PUBLIC mb_libxml)

# --- libcurl (net loader/cookies/websocket) ---------------------------------
file(GLOB _mb_curl_src "${CMAKE_SOURCE_DIR}/third_party/libcurl_7.69/src/*.c")
add_library(mb_libcurl STATIC ${_mb_curl_src})
target_include_directories(mb_libcurl PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/libcurl_7.69/include"
    "${CMAKE_SOURCE_DIR}/third_party/libcurl_7.69/src")
target_compile_definitions(mb_libcurl PRIVATE
    CURL_STATICLIB BUILDING_LIBCURL _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
target_link_libraries(mb_libcurl PUBLIC mb_zlib)

set(MB_THIRDPARTY_WIN_LIBS mb_libxslt mb_libxml mb_libcurl mb_zlib CACHE INTERNAL "")
