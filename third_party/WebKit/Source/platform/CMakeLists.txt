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
# GLOB_RECURSE to pick up graphics/filters (SVG FE* effects, 28), graphics/paint
# (DisplayItem* recorders, 19) and graphics/skia — all pure-CPU and compile clean.
# graphics/gpu is excluded below (Ganesh/GLES2 not brought up).
file(GLOB_RECURSE BLINK_GRAPHICS "${BSRC}/graphics/*.cpp")
file(GLOB BLINK_TEXT     "${BSRC}/text/*.cpp")
file(GLOB_RECURSE BLINK_FONTS "${BSRC}/fonts/*.cpp")  # incl. fonts/shaping (harfbuzz)
# More platform sublayers the link needs (all compile clean on macOS): scroll
# (ScrollableArea/Scrollbar), network (ResourceRequest/Response), mhtml,
# mediastream.
file(GLOB BLINK_SUBLAYERS
    "${BSRC}/scroll/*.cpp" "${BSRC}/network/*.cpp"
    "${BSRC}/mhtml/*.cpp" "${BSRC}/mediastream/*.cpp" "${BSRC}/blob/*.cpp"
    "${BSRC}/plugins/*.cpp" "${BSRC}/clipboard/*.cpp" "${BSRC}/scheduler/*.cpp"
    "${BSRC}/image-decoders/*.cpp"   # base ImageDecoder/ImageFrame/ImagePlanes (codec subdirs deferred)
    "${BSRC}/weborigin/*.cpp"    # KURL recovered; KURL_new/KURL_w variants excluded below
    "${BSRC}/*.cpp")             # platform-root files (JSONValues/Widget/SharedBuffer/DateComponents/...)
# platform-root: 48/50 compile. The 4 value types already in BLINK_CORE_SOURCES
# (Length/LengthBox/LengthFunctions/Decimal) are excluded here to avoid duplicate
# objects; NotImplemented (Win logging) deferred. FileMetadata is kept (getFileSize
# ported to posix stat).
list(FILTER BLINK_SUBLAYERS EXCLUDE REGEX "/(Length|LengthBox|LengthFunctions|Decimal|NotImplemented)\\.cpp$")
# Unit tests (gtest) and the value types already in the core list.
list(FILTER BLINK_HEAP      EXCLUDE REGEX "Test\\.cpp$|RunAllTests\\.cpp$")
list(FILTER BLINK_GRAPHICS  EXCLUDE REGEX "Test\\.cpp$|/Color\\.cpp$|/GraphicsTypes\\.cpp$")
list(FILTER BLINK_TEXT      EXCLUDE REGEX "Test\\.cpp$")
list(FILTER BLINK_FONTS     EXCLUDE REGEX "Test\\.cpp$")
list(FILTER BLINK_SUBLAYERS EXCLUDE REGEX "Test\\.cpp$")
# weborigin: keep KURL.cpp (recovered); the alternate KURL_new (chromium url/) and
# KURL_w (copyASCII skew) variants are not used on macOS.
list(FILTER BLINK_SUBLAYERS EXCLUDE REGEX "KURL_new\\.cpp$|KURL_w\\.cpp$")
# clipboard: keep the Posix variant; the Win one needs shlwapi.h.
list(FILTER BLINK_SUBLAYERS EXCLUDE REGEX "ClipboardUtilitiesWin\\.cpp$")
# UnifiedHeapController now compiles: migrated to the V8 8.7 EmbedderHeapTracer API
# (TracePrologue(TraceFlags)/TraceEpilogue(TraceSummary*)/TracedReferenceBase).
# Most of graphics/ is skia-2D and compiles clean (GraphicsContext, the
# *ImageBufferSurface recorders, PictureSnapshot, GradientGeneratedImage, ...).
# Truly deferred: graphics/gpu (Ganesh/GLES2 not brought up), Canvas2DLayerBridge
# (WebExternalTextureMailbox::syncPoint API skew), ImageBuffer.cpp (pulls wke ->
# windows.h), GDIPlusInit (Win-only).
# graphics/gpu compiles (uses the virtual gpu::gles2::GLES2Interface, not direct GL).
# Still deferred: Canvas2DLayerBridge (WebExternalTextureMailbox::syncPoint skew),
# GDIPlusInit (Win-only).
list(FILTER BLINK_GRAPHICS EXCLUDE REGEX "Canvas2DLayerBridge|GDIPlusInit")
# Calendar/date ICU (ucal/udat) + Windows locale: deferred.
list(FILTER BLINK_TEXT EXCLUDE REGEX "LocaleICU|LocaleWin")
# Deferred font backends: FontCustomPlatformData/FontCacheSkia (CoreText/fontconfig
# platform-data), OpenTypeSanitizer (ots library my_ntohl), and the non-mac
# platform subdirs (win/android/linux). fonts/shaping (harfbuzz) is kept.
list(FILTER BLINK_FONTS EXCLUDE REGEX "FontCustomPlatformData|FontCacheSkia|/opentype/OpenTypeSanitizer|/win/|/android/|/linux/")

# --- Platform infrastructure: the Platform interface, exported Web* wrappers,
#     and support objects the heap (and the rest of blink) need at link time. ---
set(BLINK_INFRA_SOURCES
    ScriptForbiddenScope.cpp
    PartitionAllocMemoryDumpProvider.cpp
    WebScheduler.cpp
    EventTracer.cpp
)
list(TRANSFORM BLINK_INFRA_SOURCES PREPEND "${BSRC}/")
# The exported/ Web* wrappers (WebURLRequest/Response, WebData, WebURL, ...) are the
# public-API surface blink links against: all 88 compile clean on macOS. Glob them.
file(GLOB BLINK_EXPORTED "${BSRC}/exported/*.cpp")
list(FILTER BLINK_EXPORTED EXCLUDE REGEX "Test\\.cpp$")

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
    ${BLINK_FONTS} ${BLINK_SUBLAYERS} ${BLINK_INFRA_SOURCES} ${BLINK_EXPORTED}
    ${BLINK_BASE_SHIM})
# alias.cc is plain Chromium base, not WTF: don't force-include WTF's config.h.
set_source_files_properties("${BLINK_BASE_SHIM}" PROPERTIES COMPILE_OPTIONS "")

target_include_directories(blink_platform PUBLIC
    "${CMAKE_SOURCE_DIR}"   # third_party/skia/include/core/SkRect.h style paths
    "${WSRC}"               # platform/..., wtf/...
    "${WK}"                 # public/platform/WebCommon.h
    "${CMAKE_SOURCE_DIR}/gen/blink"             # generated *Names.h headers
    "${CMAKE_SOURCE_DIR}/third_party/icu/source/common"   # unicode/*.h
    "${MINIBLINK_V8_ROOT}/include"              # v8.h (bindings-adjacent headers)
    "${CMAKE_SOURCE_DIR}/third_party/khronos"   # GLES2/* (DragImage and other root files)
    "${CMAKE_SOURCE_DIR}/third_party/harfbuzz-ng/src"  # hb.h (fonts/shaping)
    "${CMAKE_SOURCE_DIR}/third_party/v8shim"    # v8-debug.h shim (plugins/heap pull ScriptState)
    "${CMAKE_SOURCE_DIR}/win_compat"            # <windows.h> shim (ImageBuffer pulls wke)
    "${CMAKE_SOURCE_DIR}/orig_chrome")

target_link_libraries(blink_platform PUBLIC wtf skia harfbuzz)
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
    OFFICIAL_BUILD U_DISABLE_RENAMING=1 INSIDE_BLINK=1 ENABLE_OILPAN=1
    # plugins/heap now pull bindings headers (ScriptWrappable.h) which use the
    # empty calling-convention macro V8CALL and the wke embedding hooks.
    "V8CALL=" ENABLE_WKE=1
    # Match the V8 monolith ABI (pointer compression). PUBLIC so every blink
    # archive that links blink_platform (core/generated/bindings/modules/web)
    # inherits it — v8::Local/Persistent sizes depend on it, so all blink code
    # and the monolith must agree or the final link is ABI-corrupt.
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(blink_platform PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_platform PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
