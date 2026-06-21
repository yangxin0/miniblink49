# skia/ext mac bridge (macOS port) — the chromium skia extensions blink's mac
# native-theme path needs: SkiaBitLocker (skia_utils_mac), BitmapPlatformDevice
# (mac), and skia::getMetaData (platform_device). Migrated to the in-tree skia API.
set(SKE "${CMAKE_SOURCE_DIR}/skia/ext")
add_library(skia_ext STATIC
    "${SKE}/platform_device.cc"
    "${SKE}/platform_device_mac.cc"
    "${SKE}/platform_canvas.cc"
    "${SKE}/skia_utils_mac.mm"
    "${SKE}/bitmap_platform_device_mac.cc"
    "${SKE}/analysis_canvas.cc"            # skia::AnalysisCanvas (solid-color analysis)
    "${SKE}/sk_document_pdf_stub_mac.cpp"  # SkDocument PDF link stub (printing deferred)
    "${CMAKE_SOURCE_DIR}/base/mac/scoped_nsobject.mm")   # ScopedNSProtocolTraitsRelease
target_link_libraries(skia_ext PUBLIC skia)
target_include_directories(skia_ext PUBLIC "${CMAKE_SOURCE_DIR}" "${SKE}")
target_compile_definitions(skia_ext PRIVATE SK_RELEASE OFFICIAL_BUILD)
set_target_properties(skia_ext PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(skia_ext PRIVATE -fno-exceptions -Wno-c++11-narrowing -w)
endif()
