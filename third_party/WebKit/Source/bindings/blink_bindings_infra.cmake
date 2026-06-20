# blink bindings infrastructure (macOS port) — the hand-written V8<->blink glue
# (Source/bindings/core/v8): V8Initializer, V8PerIsolateData, V8GCController,
# V8Binding, V8ScriptRunner, ScriptState, V8DOMWrapper, the V8*Custom handlers,
# etc. Distinct from the generated wrappers (gen/blink/bindings). blink::initialize
# needs this; 91/98 compile after the V8 7.5->8.7 migration.

set(BIV8 "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source/bindings/core/v8")

file(GLOB BLINK_BINDINGS_INFRA "${BIV8}/*.cpp" "${BIV8}/custom/*.cpp")
list(FILTER BLINK_BINDINGS_INFRA EXCLUDE REGEX "Test\\.cpp$")
# The NPAPI plugin bindings (npruntime/NPV8Object/V8NPObject/V8NPUtils/
# V8HTMLPlugInElementCustom), ScriptController and the modern V8GCController_v8_5_7
# variant now all compile on macOS (third_party/npapi + win_compat includes, the
# Carbon TextEncoding guard, and the V8 7.5->8.7 migration of V8NPObject/V8NPUtils/
# V8GCController_v8_5_7). They mirror what the Windows miniblink.vcxproj compiles.

add_library(blink_bindings_infra STATIC ${BLINK_BINDINGS_INFRA})
target_link_libraries(blink_bindings_infra PUBLIC blink_core)
target_include_directories(blink_bindings_infra PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos"
    "${CMAKE_SOURCE_DIR}/third_party/npapi"   # bindings/npruntime.h (NP* plugin glue)
    "${CMAKE_SOURCE_DIR}/win_compat")         # <windows.h> shim (WinINet/NP cookie decls)
target_compile_definitions(blink_bindings_infra PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)
set_target_properties(blink_bindings_infra PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_bindings_infra PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
