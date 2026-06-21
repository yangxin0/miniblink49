# mc/ — miniblink's compositor (the cc-derived layer tree the page host drives).
# Provides mc::LayerTreeHost / RasterTaskWorkerThreadPool etc. that
# content::WebPageImpl references for painting/compositing.

set(MCD "${CMAKE_SOURCE_DIR}/mc")
file(GLOB_RECURSE MC_SRC "${MCD}/*.cpp")
# LayerSorter ships as a .cc (not .cpp), so the *.cpp glob misses it; add it
# explicitly to supply mc::LayerSorter (ctor/dtor/Sort) at link time.
list(APPEND MC_SRC "${MCD}/trees/LayerSorter.cc")

add_library(mc STATIC ${MC_SRC})
target_link_libraries(mc PUBLIC blink_web)
target_include_directories(mc BEFORE PRIVATE "${CMAKE_SOURCE_DIR}/mc/cc_shim")
target_include_directories(mc PUBLIC
    "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/mc" "${CMAKE_SOURCE_DIR}/wke"
    "${CMAKE_SOURCE_DIR}/electron"  # base/files/scoped_file.h pulls cef/include/base/cef_scoped_ptr.h
    "${CMAKE_SOURCE_DIR}/win_compat" "${CMAKE_SOURCE_DIR}/third_party/npapi"
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/core"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/config"
    "${CMAKE_SOURCE_DIR}/third_party/skia/include/utils")
target_compile_definitions(mc PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(mc PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(mc PRIVATE
        -fdeclspec -fno-exceptions -fno-rtti -w
        "SHELL:-include ${CMAKE_SOURCE_DIR}/win_compat/windows.h")
endif()
