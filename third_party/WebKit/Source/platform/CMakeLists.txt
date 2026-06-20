# blink platform (macOS port) — the first blink layer above WTF + skia + ICU.
#
# Layering / verification status:
#   * geometry + math primitives + transforms + graphics value types are
#     LINK-VERIFIED: the blink_geometry smoke test links and runs them.
#   * graphics / text / fonts are COMPILE-VERIFIED: every source listed compiles
#     on macOS clang and lands in the archive, but full-binary linkability of the
#     GPU (Ganesh/GLES2), Oilpan heap/GC, harfbuzz shaping and CoreText font
#     backends is deferred — those subsystems are not brought up yet, so files
#     depending on them are excluded below. As a static archive this is sound:
#     consumers only pull the objects they reference.
#
# ICU: the in-tree third_party/icu is headers-only (no sources, no data). On
# macOS we use the system libicucore, which exports UNVERSIONED symbols, so the
# headers are told not to version-rename (U_DISABLE_RENAMING=1). blink's ICU use
# here is the stable C API (uscript_*, u_charType, unorm_normalize, ...).

set(BSRC "${CMAKE_CURRENT_SOURCE_DIR}")              # .../WebKit/Source/platform
set(WK   "${CMAKE_SOURCE_DIR}/third_party/WebKit")
set(WSRC "${WK}/Source")

# --- Link-verified core: geometry + primitives + transforms + value types -----
set(BLINK_CORE_SOURCES
    geometry/DoublePoint.cpp geometry/DoubleRect.cpp geometry/DoubleSize.cpp
    geometry/FloatPoint.cpp geometry/FloatPoint3D.cpp geometry/FloatPolygon.cpp
    geometry/FloatQuad.cpp geometry/FloatRect.cpp geometry/FloatRoundedRect.cpp
    geometry/FloatSize.cpp geometry/IntRect.cpp geometry/LayoutRect.cpp
    geometry/LayoutRectOutsets.cpp geometry/Region.cpp geometry/TransformState.cpp
    Length.cpp LengthBox.cpp LengthFunctions.cpp Decimal.cpp
    transforms/AffineTransform.cpp transforms/InterpolatedTransformOperation.cpp
    transforms/Matrix3DTransformOperation.cpp transforms/MatrixTransformOperation.cpp
    transforms/PerspectiveTransformOperation.cpp transforms/RotateTransformOperation.cpp
    transforms/ScaleTransformOperation.cpp transforms/SkewTransformOperation.cpp
    transforms/TransformationMatrix.cpp transforms/TransformOperations.cpp
    transforms/TranslateTransformOperation.cpp
    graphics/Color.cpp graphics/GraphicsTypes.cpp
)
list(TRANSFORM BLINK_CORE_SOURCES PREPEND "${BSRC}/")

# --- Compile-verified: heap / graphics / text / fonts (glob, minus deferred) ---
# INSIDE_BLINK=1 (set below) is the standard blink build flag; it gates internal
# APIs like FROM_HERE and the WebString<->String conversions, and unlocks the
# bulk of these dirs (heap 11/12, graphics 39/49, text 21/23, fonts 16/17).
file(GLOB BLINK_HEAP     "${BSRC}/heap/*.cpp")
file(GLOB BLINK_GRAPHICS "${BSRC}/graphics/*.cpp")
file(GLOB BLINK_TEXT     "${BSRC}/text/*.cpp")
file(GLOB BLINK_FONTS    "${BSRC}/fonts/*.cpp")
# More platform sublayers the link needs (all compile clean on macOS): scroll
# (ScrollableArea/Scrollbar), network (ResourceRequest/Response), mhtml,
# mediastream.
file(GLOB BLINK_SUBLAYERS
    "${BSRC}/scroll/*.cpp" "${BSRC}/network/*.cpp"
    "${BSRC}/mhtml/*.cpp" "${BSRC}/mediastream/*.cpp")
# Unit tests (gtest) and the value types already in the core list.
list(FILTER BLINK_HEAP      EXCLUDE REGEX "Test\\.cpp$|RunAllTests\\.cpp$")
list(FILTER BLINK_GRAPHICS  EXCLUDE REGEX "Test\\.cpp$|/Color\\.cpp$|/GraphicsTypes\\.cpp$")
list(FILTER BLINK_TEXT      EXCLUDE REGEX "Test\\.cpp$")
list(FILTER BLINK_FONTS     EXCLUDE REGEX "Test\\.cpp$")
list(FILTER BLINK_SUBLAYERS EXCLUDE REGEX "Test\\.cpp$")
# V8 EmbedderHeapTracer API skew (final/virtual): deferred.
list(FILTER BLINK_HEAP EXCLUDE REGEX "UnifiedHeapController")
# GPU (Ganesh/GLES2), canvas-layer/compositor, GDI/Win: deferred.
list(FILTER BLINK_GRAPHICS EXCLUDE REGEX "Canvas2DLayerBridge|Canvas2DLayerManager|GDIPlusInit|GradientGeneratedImage|GraphicsContext|ImageBuffer|ImageBufferSurface|PictureSnapshot|RecordingImageBufferSurface|UnacceleratedImageBufferSurface")
# Calendar/date ICU (ucal/udat) + Windows locale: deferred.
list(FILTER BLINK_TEXT EXCLUDE REGEX "LocaleICU|LocaleWin")
# Needs the font platform-data backend (CoreText): deferred.
list(FILTER BLINK_FONTS EXCLUDE REGEX "FontCustomPlatformData")

# --- Platform infrastructure: the Platform interface, exported Web* wrappers,
#     and support objects the heap (and the rest of blink) need at link time. ---
set(BLINK_INFRA_SOURCES
    ScriptForbiddenScope.cpp
    PartitionAllocMemoryDumpProvider.cpp
    WebScheduler.cpp
    EventTracer.cpp
    exported/Platform.cpp
    exported/WebCommon.cpp
    exported/WebString.cpp
    exported/WebCString.cpp
    exported/WebTraceLocation.cpp
    exported/WebMemoryDumpProvider.cpp
    exported/WebConvertableToTraceFormat.cpp
)
list(TRANSFORM BLINK_INFRA_SOURCES PREPEND "${BSRC}/")

# Oilpan conservative stack scanning needs an arch-specific register-save asm
# routine (pushAllRegisters). The in-tree SaveRegisters_arm64.S is ELF-only;
# SaveRegisters_arm64_mac.S is the Mach-O port. It must NOT receive blink's
# C++ flags (the target force-includes config.h, which pulls <cstdio>), so it
# lives in its own tiny library linked into blink_platform.
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    add_library(blink_heap_asm STATIC "${BSRC}/heap/asm/SaveRegisters_arm64_mac.S")
    set_source_files_properties("${BSRC}/heap/asm/SaveRegisters_arm64_mac.S"
        PROPERTIES LANGUAGE ASM)
endif()

# base::debug::Alias — a tiny no-op blink references to defeat optimization. Pull
# the single base file in directly rather than linking the whole base lib for it.
set(BLINK_BASE_SHIM "${CMAKE_SOURCE_DIR}/orig_chrome/base/debug/alias.cc")

add_library(blink_platform STATIC
    ${BLINK_CORE_SOURCES} ${BLINK_HEAP} ${BLINK_GRAPHICS} ${BLINK_TEXT}
    ${BLINK_FONTS} ${BLINK_SUBLAYERS} ${BLINK_INFRA_SOURCES} ${BLINK_BASE_SHIM})
# alias.cc is plain Chromium base, not WTF: don't force-include WTF's config.h.
set_source_files_properties("${BLINK_BASE_SHIM}" PROPERTIES COMPILE_OPTIONS "")

target_include_directories(blink_platform PUBLIC
    "${CMAKE_SOURCE_DIR}"   # third_party/skia/include/core/SkRect.h style paths
    "${WSRC}"               # platform/..., wtf/...
    "${WK}"                 # public/platform/WebCommon.h
    "${CMAKE_SOURCE_DIR}/gen/blink"             # generated *Names.h headers
    "${CMAKE_SOURCE_DIR}/third_party/icu/source/common"   # unicode/*.h
    "${MINIBLINK_V8_ROOT}/include"              # v8.h (bindings-adjacent headers)
    "${CMAKE_SOURCE_DIR}/orig_chrome")

target_link_libraries(blink_platform PUBLIC wtf skia)
if(APPLE)
    # System ICU (unversioned symbols) + CoreText/CoreGraphics for fonts.
    target_link_libraries(blink_platform PUBLIC
        icucore "-framework CoreText" "-framework CoreGraphics"
        "-framework CoreFoundation")
    if(TARGET blink_heap_asm)
        target_link_libraries(blink_platform PUBLIC blink_heap_asm)
    endif()
endif()

# blink/WTF need the config bootstrap force-included; skia needs SK_RELEASE and
# blink uses __declspec annotations (-fdeclspec) and is built without exceptions.
target_compile_options(blink_platform PUBLIC -include "${WSRC}/config.h")
# ENABLE_OILPAN=1 makes blink's RefCountedWillBe*/RefPtrWillBeMember transitional
# types resolve to garbage-collected (Member/GarbageCollectedFinalized). It is an
# ABI-affecting flag, so it must be identical across platform and core; PUBLIC so
# blink_core inherits it.
target_compile_definitions(blink_platform PUBLIC
    OFFICIAL_BUILD U_DISABLE_RENAMING=1 INSIDE_BLINK=1 ENABLE_OILPAN=1)
set_target_properties(blink_platform PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_platform PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
