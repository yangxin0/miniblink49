# harfbuzz (macOS port) — the OpenType text shaper blink's HarfBuzzShaper uses.
# Mirrors the Windows harfbuzz.gyp configuration: the in-tree harfbuzz 1.1.1 built
# with HAVE_OT + HAVE_ICU (its own OpenType shaper, Unicode properties from ICU),
# HB_NO_MT (single-thread). No platform backend (no CoreText/Uniscribe): the font
# tables are fed from a Skia typeface (see HarfBuzzFace's portable path), so this
# builds identically on Windows and macOS.
#
# ICU: uses the in-tree third_party/icu headers (unversioned, U_DISABLE_RENAMING)
# + system libicucore, exactly like blink_platform. A few headers needed only by
# hb-icu.cc (unorm/unorm2/ustring/uversion/utf16 and some uchar.h enums) were added
# to the in-tree ICU subset.

set(HBSRC "${CMAKE_SOURCE_DIR}/third_party/harfbuzz-ng/src")
file(GLOB HARFBUZZ_SRC "${HBSRC}/*.cc")

add_library(harfbuzz STATIC ${HARFBUZZ_SRC})

target_include_directories(harfbuzz PUBLIC "${HBSRC}")
target_include_directories(harfbuzz PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/third_party/WebKit"
    "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source"   # in-tree ICU umachine.h -> WTF UChar
    "${CMAKE_SOURCE_DIR}/third_party/icu/source/common"
    "${CMAKE_SOURCE_DIR}/orig_chrome")

target_compile_definitions(harfbuzz PRIVATE
    HAVE_OT HAVE_ICU HAVE_ICU_BUILTIN HB_NO_MT U_DISABLE_RENAMING=1
    # Unlocks the in-tree ICU header additions hb-icu.cc needs (uchar.h enums/funcs,
    # utf16.h macros) without affecting blink, which keeps its own ICU shims.
    MINIBLINK_HARFBUZZ_ICU
    # miniblink patched harfbuzz to call the MSVC _strdup; map it to posix strdup
    # off-Windows (Windows keeps the real _strdup).
    $<$<NOT:$<BOOL:${WIN32}>>:_strdup=strdup>)

set_target_properties(harfbuzz PROPERTIES CXX_STANDARD 11)
if(APPLE)
    target_link_libraries(harfbuzz PUBLIC icucore)
endif()
if(NOT MSVC)
    target_compile_options(harfbuzz PRIVATE
        -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types)
endif()
