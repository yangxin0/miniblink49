# wke — miniblink's public C API (the libwke surface). wke.cpp/wke2.cpp are the API
# entry points; wkeJsBind (JS bindings), wkeNetHook (net hooks), wkeWebView/
# wkeWebWindow (the view/window objects). wkeGlobalVar + wkeString already live in
# the wke_globals archive (blink links those), so they're excluded here.

set(WKED "${CMAKE_SOURCE_DIR}/wke")
file(GLOB WKE_SRC "${WKED}/*.cpp")
list(FILTER WKE_SRC EXCLUDE REGEX "/(wkeGlobalVar|wkeString|CurlVsetoptForwardMac)\\.cpp$")  # in wke_globals

add_library(wke STATIC ${WKE_SRC})
target_link_libraries(wke PUBLIC content_browser mc net_portable)
target_include_directories(wke PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/wke" "${CMAKE_SOURCE_DIR}/orig_chrome"
    "${CMAKE_SOURCE_DIR}/mc" "${CMAKE_SOURCE_DIR}/electron" "${CMAKE_SOURCE_DIR}/win_compat"
    "${CMAKE_SOURCE_DIR}/third_party/npapi" "${CMAKE_SOURCE_DIR}/third_party/v8shim"
    "${CMAKE_SOURCE_DIR}/third_party/khronos" "${CMAKE_SOURCE_DIR}/third_party/libcurl_7.69/include"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/core"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/config"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/utils")
target_compile_definitions(wke PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(wke PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(wke PRIVATE
        -fdeclspec -fno-exceptions -fno-rtti -w
        "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
endif()
