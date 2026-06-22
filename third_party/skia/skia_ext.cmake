# skia/ext mac bridge (macOS port) — the chromium skia extensions blink's mac
# native-theme path needs: SkiaBitLocker (skia_utils_mac), BitmapPlatformDevice
# (mac), and skia::getMetaData (platform_device). Migrated to the in-tree skia API.
set(SKE "${CMAKE_SOURCE_DIR}/skia/ext")
# Portable extensions shared by both platforms.
set(SKIA_EXT_SRC
    "${SKE}/platform_device.cc"
    "${SKE}/platform_canvas.cc"
    "${SKE}/analysis_canvas.cc")           # skia::AnalysisCanvas (solid-color analysis)
if(MB_OS_WINDOWS)
    list(APPEND SKIA_EXT_SRC
        "${SKE}/platform_device_win.cc"
        "${SKE}/bitmap_platform_device_win.cc"
        "${SKE}/skia_utils_win.cc"
        "${SKE}/fontmgr_default_win.cc")
else()
    list(APPEND SKIA_EXT_SRC
        "${SKE}/platform_device_mac.cc"
        "${SKE}/skia_utils_mac.mm"
        "${SKE}/bitmap_platform_device_mac.cc"
        "${SKE}/sk_document_pdf_stub_mac.cpp"  # SkDocument PDF link stub (printing deferred)
        "${CMAKE_SOURCE_DIR}/base/mac/scoped_nsobject.mm")   # ScopedNSProtocolTraitsRelease
endif()
add_library(skia_ext STATIC ${SKIA_EXT_SRC})
target_link_libraries(skia_ext PUBLIC skia)
target_include_directories(skia_ext PUBLIC "${CMAKE_SOURCE_DIR}" "${SKE}")
# SK_DEBUG/SK_RELEASE is provided (config-based) by the skia target we link; don't
# hardcode SK_RELEASE here or it conflicts with skia's SK_DEBUG in Debug configs.
target_compile_definitions(skia_ext PRIVATE OFFICIAL_BUILD)
set_target_properties(skia_ext PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(skia_ext PRIVATE -fno-exceptions -Wno-c++11-narrowing -w)
endif()
