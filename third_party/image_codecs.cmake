# In-tree image codec libraries (macOS port) — libpng / libjpeg / libwebp, the
# decoders/encoders blink's platform/image-{decoders,encoders} link against. These
# are Chromium's bundled copies (no external deps beyond zlib, which we take from
# the system at link time). Built as plain C static libs.

# --- libpng (needs zlib headers; links system zlib) ---
file(GLOB LIBPNG_SRC "${CMAKE_SOURCE_DIR}/third_party/libpng/*.c")
add_library(miniblink_png STATIC ${LIBPNG_SRC})
# This libpng copy has no arm/ NEON sources, only the declaration; disable the
# ARM NEON filter path so it isn't referenced.
target_compile_definitions(miniblink_png PRIVATE PNG_ARM_NEON_OPT=0)
target_include_directories(miniblink_png PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/libpng/mac_compat")  # <fp.h> -> <math.h> shim
target_include_directories(miniblink_png PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/libpng" "${CMAKE_SOURCE_DIR}/third_party/zlib")
if(NOT MSVC)
    target_compile_options(miniblink_png PRIVATE -w)
endif()

# --- libjpeg (Chromium copy: jmemmgr + jmemnobs backing, no tools) ---
file(GLOB LIBJPEG_SRC "${CMAKE_SOURCE_DIR}/third_party/libjpeg/*.c")
add_library(miniblink_jpeg STATIC ${LIBJPEG_SRC})
target_include_directories(miniblink_jpeg PUBLIC "${CMAKE_SOURCE_DIR}/third_party/libjpeg")
if(NOT MSVC)
    target_compile_options(miniblink_jpeg PRIVATE -w)
endif()

# --- ots (OpenType Sanitiser) — web @font-face validation; woff2.cc needs brotli (deferred) ---
file(GLOB OTS_SRC "${CMAKE_SOURCE_DIR}/third_party/ots/src/*.cc")
list(FILTER OTS_SRC EXCLUDE REGEX "woff2\\.cc$")
add_library(miniblink_ots STATIC ${OTS_SRC})
target_include_directories(miniblink_ots PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/ots/include")
target_include_directories(miniblink_ots PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/ots/src" "${CMAKE_SOURCE_DIR}/third_party")
if(NOT MSVC)
    target_compile_options(miniblink_ots PRIVATE -w -fno-exceptions)
endif()

# --- libwebp (dec + dsp + enc + utils + demux) ---
file(GLOB_RECURSE LIBWEBP_SRC "${CMAKE_SOURCE_DIR}/third_party/libwebp/*.c")
add_library(miniblink_webp STATIC ${LIBWEBP_SRC})
# No NEON dsp sources in this copy -> take the scalar/SSE path (see dsp.h).
target_compile_definitions(miniblink_webp PRIVATE WEBP_DISABLE_NEON)
target_include_directories(miniblink_webp PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/libwebp" "${CMAKE_SOURCE_DIR}/third_party/libwebp/src")
if(NOT MSVC)
    target_compile_options(miniblink_webp PRIVATE -w)
endif()
