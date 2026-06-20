# blink generated code (macOS port) — the codegen output blink links against:
# the *Names tables (HTMLNames/SVGNames/EventTypeNames/...), RuntimeEnabledFeatures,
# CSSPropertyNames, etc. The trial link showed ~1,000+ undefined symbols come from
# here (we'd compiled the generated *headers* but never the generated *sources*).
# These are prebuilt in-tree under gen/blink — no codegen step needed.
#
# Compile-verified static archive. The V8 DOM-binding wrappers (gen/blink/bindings,
# ~866 files) are a separate, later step.

set(GENSRC "${CMAKE_SOURCE_DIR}/gen/blink")

file(GLOB_RECURSE BLINK_GEN_SRC "${GENSRC}/core/*.cpp" "${GENSRC}/platform/*.cpp")
# CSSTokenizerCodepoints needs core CSSTokenizer; InspectorInstrumentationImpl
# pulls the inspector debugger (v8::NativeWeakMap, removed in 8.7) — deferred.
list(FILTER BLINK_GEN_SRC EXCLUDE REGEX "CSSTokenizerCodepoints\\.cpp$|InspectorInstrumentationImpl\\.cpp$")

add_library(blink_generated STATIC ${BLINK_GEN_SRC})
target_link_libraries(blink_generated PUBLIC blink_platform)
target_include_directories(blink_generated PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos")
target_compile_definitions(blink_generated PUBLIC "V8CALL=" ENABLE_WKE=1)
set_target_properties(blink_generated PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_generated PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
