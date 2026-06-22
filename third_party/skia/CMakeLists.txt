# skia — blink's 2D graphics library (macOS raster bring-up).
#
# This builds a CPU/raster skia for macOS arm64: core + effects + image (raster)
# + lazy + pathops + sfnt + utils, the CoreText/CoreGraphics platform ports, and
# the portable ("_none") SIMD opts. The GPU (Ganesh) backend, FreeType/fontconfig
# (Linux), DirectWrite/GDI (Windows) and ETC1/Lua extras are intentionally left
# out of this first pass; NEON opts can replace the _none variants later.
#
# skia is largely cross-platform already: 159/159 of src/core compiles unmodified
# on macOS clang once the include paths are right. The only in-tree fixes needed
# were a portable DebugBreak() (SkPostConfig.h), a clang-strictness bug in
# SkTLazy.h, and gating the x86 SSE2 opts header in SkXfermode.cpp by arch.

set(SK "${CMAKE_CURRENT_SOURCE_DIR}")

# --- Glob the portable subsystems, then drop the per-file exceptions. ---------
file(GLOB SKIA_CORE     "${SK}/src/core/*.cpp")
file(GLOB SKIA_EFFECTS  "${SK}/src/effects/*.cpp" "${SK}/src/effects/gradients/*.cpp")
file(GLOB SKIA_IMAGE    "${SK}/src/image/*.cpp")
file(GLOB SKIA_LAZY     "${SK}/src/lazy/*.cpp")
file(GLOB SKIA_PATHOPS  "${SK}/src/pathops/*.cpp")
file(GLOB SKIA_SFNT     "${SK}/src/sfnt/*.cpp")
file(GLOB SKIA_UTILS    "${SK}/src/utils/*.cpp")

# GPU (Ganesh) backend — not built in this raster pass.
list(FILTER SKIA_IMAGE EXCLUDE REGEX "SkImage_Gpu\\.cpp$|SkSurface_Gpu\\.cpp$")
# In-tree source bugs / debug-only translation units in this skia snapshot.
list(FILTER SKIA_PATHOPS EXCLUDE REGEX "SkConicLineIntersection\\.cpp$|SkPathOpsPostSect\\.cpp$")
# Lua bindings (no lua) and ETC1 texture compression (GPU) are excluded on every
# platform. The thread backend is platform-specific: macOS keeps the pthread
# variants, Windows keeps SkThreadUtils_win.
list(FILTER SKIA_UTILS EXCLUDE REGEX "SkLua.*\\.cpp$|SkTextureCompressor\\.cpp$")
if(MB_OS_WINDOWS)
    list(FILTER SKIA_UTILS EXCLUDE REGEX "SkThreadUtils_pthread.*\\.cpp$")
else()
    list(FILTER SKIA_UTILS EXCLUDE REGEX "SkThreadUtils_pthread_linux\\.cpp$|SkThreadUtils_pthread_other\\.cpp$|SkThreadUtils_win\\.cpp$")
endif()

# --- Platform ports. Portable pieces are shared; the font host, file, TLS, time
#     and debug backends are platform-specific. ---------------------------------
set(SKIA_PORTS
    src/ports/SkOSFile_stdio.cpp
    src/ports/SkMemory_malloc.cpp
    src/ports/SkImageGenerator_none.cpp
    src/ports/SkGlobalInitialization_default.cpp
    src/ports/SkImageDecoder_empty.cpp
    src/ports/SkDiscardableMemory_none.cpp
)
if(MB_OS_WINDOWS)
    # DirectWrite font host — blink's FontCacheSkiaWin calls SkFontMgr_New_DirectWrite
    # — plus the COM/DWrite helpers, and Win32 file/TLS/time/debug.
    list(APPEND SKIA_PORTS
        src/ports/SkFontHost_win.cpp          # SkFontMgr_New_GDI (FontCacheSkiaWin picks GDI or DW)
        src/ports/SkFontMgr_win_dw.cpp
        src/ports/SkFontMgr_win_dw_factory.cpp
        src/ports/SkScalerContext_win_dw.cpp
        src/ports/SkTypeface_win_dw.cpp
        src/ports/SkRemotableFontMgr_win_dw.cpp
        src/fonts/SkFontMgr_indirect.cpp      # SkFontMgr_Indirect (DW mgr base)
        src/fonts/SkRemotableFontMgr.cpp
        src/utils/win/SkDWrite.cpp
        src/utils/win/SkDWriteFontFileStream.cpp
        src/utils/win/SkDWriteGeometrySink.cpp
        src/utils/win/SkHRESULT.cpp
        src/utils/win/SkIStream.cpp
        src/utils/win/SkAutoCoInitialize.cpp
        src/ports/SkOSFile_win.cpp
        src/ports/SkTime_win.cpp
        src/ports/SkTLS_win.cpp
        src/ports/SkDebug_win.cpp
    )
else()
    # CoreText fonts, CoreGraphics images, posix files/TLS.
    list(APPEND SKIA_PORTS
        src/ports/SkFontHost_mac.cpp
        src/ports/SkOSFile_posix.cpp
        src/ports/SkTime_Unix.cpp
        src/ports/SkTLS_pthread.cpp
        src/ports/SkDebug_stdio.cpp
        src/utils/mac/SkCreateCGImageRef.cpp
        src/utils/mac/SkStream_mac.cpp
    )
endif()
list(TRANSFORM SKIA_PORTS PREPEND "${SK}/")

# --- SIMD opts. macOS/arm64 uses the NEON dispatchers + implementations (arm64
#     always has NEON). Windows/x64 uses the portable "_none" variants for this
#     first pass; the SSE2/SSSE3 dispatchers can replace them later.
if(MB_OS_WINDOWS)
    set(SKIA_OPTS
        src/opts/SkBitmapProcState_opts_none.cpp
        src/opts/SkBlitMask_opts_none.cpp
        src/opts/SkBlitRow_opts_none.cpp
        src/opts/SkBlurImage_opts_none.cpp
        src/opts/SkMorphology_opts_none.cpp
        src/opts/SkUtils_opts_none.cpp
        src/opts/SkXfermode_opts_none.cpp
        src/opts/SkTextureCompression_opts_none.cpp
    )
else()
    set(SKIA_OPTS
        src/opts/SkBitmapProcState_opts_arm.cpp
        src/opts/SkBitmapProcState_arm_neon.cpp
        src/opts/SkBitmapProcState_matrixProcs_neon.cpp
        src/opts/SkBlitMask_opts_arm.cpp
        src/opts/SkBlitMask_opts_arm_neon.cpp
        src/opts/SkBlitRow_opts_arm.cpp
        src/opts/SkBlitRow_opts_arm_neon.cpp
        src/opts/SkBlurImage_opts_arm.cpp
        src/opts/SkBlurImage_opts_neon.cpp
        src/opts/SkMorphology_opts_arm.cpp
        src/opts/SkMorphology_opts_neon.cpp
        src/opts/SkUtils_opts_arm.cpp
        src/opts/SkUtils_opts_arm_neon.cpp
        src/opts/SkXfermode_opts_arm.cpp
        src/opts/SkXfermode_opts_arm_neon.cpp
        src/opts/SkTextureCompression_opts_none.cpp
    )
endif()
list(TRANSFORM SKIA_OPTS PREPEND "${SK}/")

add_library(skia STATIC
    ${SKIA_CORE} ${SKIA_EFFECTS} ${SKIA_IMAGE} ${SKIA_LAZY}
    ${SKIA_PATHOPS} ${SKIA_SFNT} ${SKIA_UTILS} ${SKIA_PORTS} ${SKIA_OPTS})

# Public headers blink and the rest of the engine include.
target_include_directories(skia PUBLIC
    "${SK}/include/core"   "${SK}/include/config"  "${SK}/include/private"
    "${SK}/include/utils"  "${SK}/include/utils/mac" "${SK}/include/effects"
    "${SK}/include/ports"  "${SK}/include/gpu"     "${SK}/include/pathops"
    "${SK}/include/images" "${SK}/include/pipe"    "${SK}/include/client/android"
    "${SK}/include/c")
# Internal headers shared across the skia sources.
target_include_directories(skia PRIVATE
    "${SK}/src/core"   "${SK}/src/utils" "${SK}/src/image"  "${SK}/src/gpu"
    "${SK}/src/lazy"   "${SK}/src/sfnt"  "${SK}/src/opts"   "${SK}/src/effects"
    "${SK}/src/pathops" "${SK}/src/ports")

if(MB_OS_WINDOWS)
    # Bundled zlib headers for SkFlate.cpp; macOS uses the system zlib it links (-lz).
    # src/utils/win: SkHRESULT.h and the Win COM/DWrite helpers the GDI font host pulls.
    target_include_directories(skia PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/zlib"
        "${SK}/include/utils/win"
        "${SK}/src/utils/win")
endif()

target_compile_definitions(skia PRIVATE SK_INTERNAL)
# skia requires exactly one of SK_DEBUG / SK_RELEASE.
target_compile_definitions(skia PUBLIC
    $<$<CONFIG:Debug>:SK_DEBUG> $<$<NOT:$<CONFIG:Debug>>:SK_RELEASE>)
set_target_properties(skia PROPERTIES CXX_STANDARD 14)

if(NOT MSVC)
    target_compile_options(skia PRIVATE
        -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()

if(APPLE)
    # CoreText/CoreGraphics font + image ports.
    target_link_libraries(skia PUBLIC
        "-framework CoreFoundation" "-framework CoreGraphics"
        "-framework CoreText" "-framework ImageIO")
endif()
