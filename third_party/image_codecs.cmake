# In-tree image codec libraries (macOS port) — libpng / libjpeg / libwebp, the
# decoders/encoders blink's platform/image-{decoders,encoders} link against. These
# are Chromium's bundled copies (no external deps beyond zlib, which we take from
# the system at link time). Built as plain C static libs.

# --- libpng (needs zlib headers; links system zlib) ---
file(GLOB LIBPNG_SRC "${CMAKE_SOURCE_DIR}/third_party/libpng/*.c")
add_library(miniblink_png STATIC ${LIBPNG_SRC})
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

# --- libwebp (dec + dsp + enc + utils + demux) ---
file(GLOB_RECURSE LIBWEBP_SRC "${CMAKE_SOURCE_DIR}/third_party/libwebp/*.c")
add_library(miniblink_webp STATIC ${LIBWEBP_SRC})
target_include_directories(miniblink_webp PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/libwebp" "${CMAKE_SOURCE_DIR}/third_party/libwebp/src")
if(NOT MSVC)
    target_compile_options(miniblink_webp PRIVATE -w)
endif()
