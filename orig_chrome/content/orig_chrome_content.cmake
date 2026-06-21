# orig_chrome/content — the Chromium content wrapper layer miniblink's wke uses:
# OrigChromeMgr (V8/compositor bootstrap) + LayerTreeWrap (cc layer tree bridge)
# that wke.cpp and content::WebPageImpl reference.

set(OCC "${CMAKE_SOURCE_DIR}/orig_chrome/content")
file(GLOB OCC_SRC "${OCC}/*.cpp")

add_library(orig_chrome_content STATIC ${OCC_SRC})
target_link_libraries(orig_chrome_content PUBLIC mc)
# orig_chrome first (cc/ headers resolve to Chromium copies); cc_shim fixes the
# case-insensitive cc/tiles/tile.h collision (see mc/mc.cmake).
target_include_directories(orig_chrome_content BEFORE PRIVATE "${CMAKE_SOURCE_DIR}/mc/cc_shim")
target_include_directories(orig_chrome_content PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/orig_chrome" "${CMAKE_SOURCE_DIR}/mc"
    "${CMAKE_SOURCE_DIR}/wke" "${CMAKE_SOURCE_DIR}/electron" "${CMAKE_SOURCE_DIR}/win_compat"
    "${CMAKE_SOURCE_DIR}/third_party/npapi" "${CMAKE_SOURCE_DIR}/third_party/v8shim"
    "${CMAKE_SOURCE_DIR}/third_party/khronos"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/core"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/config"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/utils")
target_compile_definitions(orig_chrome_content PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(orig_chrome_content PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(orig_chrome_content PRIVATE
        -fdeclspec -fno-exceptions -fno-rtti -w
        "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
endif()
