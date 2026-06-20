# blink platform (macOS port) — the first blink layer above WTF + skia.
#
# Starts with platform/geometry: IntRect/FloatRect/FloatPoint/LayoutRect/Region/
# etc. These are the foundational geometry primitives the rest of blink builds on,
# and depend only on WTF (already built) and skia (already built). All 15 non-test
# geometry sources compile unmodified on macOS clang. More platform subdirs
# (graphics, fonts, text, ...) get appended here as they are brought up.

set(BSRC "${CMAKE_CURRENT_SOURCE_DIR}")              # .../WebKit/Source/platform
set(WK   "${CMAKE_SOURCE_DIR}/third_party/WebKit")
set(WSRC "${WK}/Source")
set(SK   "${CMAKE_SOURCE_DIR}/third_party/skia")

set(BLINK_GEOMETRY_SOURCES
    geometry/DoublePoint.cpp
    geometry/DoubleRect.cpp
    geometry/DoubleSize.cpp
    geometry/FloatPoint.cpp
    geometry/FloatPoint3D.cpp
    geometry/FloatPolygon.cpp
    geometry/FloatQuad.cpp
    geometry/FloatRect.cpp
    geometry/FloatRoundedRect.cpp
    geometry/FloatSize.cpp
    geometry/IntRect.cpp
    geometry/LayoutRect.cpp
    geometry/LayoutRectOutsets.cpp
    geometry/Region.cpp
    geometry/TransformState.cpp
)
list(TRANSFORM BLINK_GEOMETRY_SOURCES PREPEND "${BSRC}/")

# base::debug::Alias — a tiny no-op blink references to defeat optimization. Pull
# the single base file in directly rather than linking the whole base lib for it.
set(BLINK_BASE_SHIM "${CMAKE_SOURCE_DIR}/orig_chrome/base/debug/alias.cc")

add_library(blink_platform STATIC ${BLINK_GEOMETRY_SOURCES} ${BLINK_BASE_SHIM})
# alias.cc is plain Chromium base, not WTF: don't force-include WTF's config.h.
set_source_files_properties("${BLINK_BASE_SHIM}" PROPERTIES COMPILE_OPTIONS "")

target_include_directories(blink_platform PUBLIC
    "${CMAKE_SOURCE_DIR}"   # third_party/skia/include/core/SkRect.h style paths
    "${WSRC}"               # platform/..., wtf/...
    "${WK}"                 # public/platform/WebCommon.h
    "${CMAKE_SOURCE_DIR}/orig_chrome")

target_link_libraries(blink_platform PUBLIC wtf skia)

# blink/WTF need the config bootstrap force-included; skia needs SK_RELEASE and
# blink uses __declspec annotations (-fdeclspec) and is built without exceptions.
target_compile_options(blink_platform PUBLIC -include "${WSRC}/config.h")
target_compile_definitions(blink_platform PUBLIC OFFICIAL_BUILD)
set_target_properties(blink_platform PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_platform PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
