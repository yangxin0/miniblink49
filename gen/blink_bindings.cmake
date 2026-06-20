# blink generated V8 bindings (macOS port) — the per-DOM-interface V8 wrapper
# classes (V8Element, V8Document, V8HTML*, V8*Event, ...) that bridge JS <-> blink.
# The trial link showed core/web/modules all depend on these (~866 generated files
# under gen/blink/bindings). Prebuilt in-tree — no codegen needed.
#
# Same flag set as blink_modules: blink-internal + the V8 monolith pointer-
# compression ABI (these include v8.h but don't link v8_monolith).

set(GENB "${CMAKE_SOURCE_DIR}/gen/blink/bindings")

file(GLOB_RECURSE BLINK_BINDINGS_SRC "${GENB}/*.cpp")
list(FILTER BLINK_BINDINGS_SRC EXCLUDE REGEX "Test\\.cpp$")
# 150 of 863 generated wrappers fail (specific interfaces with V8 7.5->8.7 binding
# patterns the generator emitted — collection indexers, certain dictionaries,
# track/media types). Listed in blink_bindings_skip.txt; deferred (a generator-
# template-level migration). 713 wrappers compile.
file(STRINGS "${CMAKE_SOURCE_DIR}/gen/blink_bindings_skip.txt" BLINK_BINDINGS_SKIP)
foreach(_skip ${BLINK_BINDINGS_SKIP})
    list(FILTER BLINK_BINDINGS_SRC EXCLUDE REGEX "/${_skip}\\.cpp$")
endforeach()

add_library(blink_bindings STATIC ${BLINK_BINDINGS_SRC})
target_link_libraries(blink_bindings PUBLIC blink_core)
target_include_directories(blink_bindings PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos")
target_compile_definitions(blink_bindings PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(blink_bindings PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_bindings PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
