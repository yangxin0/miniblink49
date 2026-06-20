# gin — the V8 integration layer (Chromium's gin), built against the pinned
# V8 8.7 and the engine's base/. Compiling this verifies the V8 7.5->8.7 API
# migration end-to-end. Built as a static lib (archive only) so it compile-
# verifies without needing every orig_chrome/base .cc linked.

set(GIN_SOURCES
    arguments.cc
    array_buffer.cc
    context_holder.cc
    converter.cc
    debug_impl.cc
    dictionary.cc
    function_template.cc
    interceptor.cc
    isolate_holder.cc
    microtasks_scope.cc
    object_template_builder.cc
    per_context_data.cc
    per_isolate_data.cc
    promise.cc
    run_microtasks_observer.cc
    runner.cc
    shell_runner.cc
    try_catch.cc
    v8_initializer_empty.cc      # NOTE: the _empty variant (miniblink uses this, not v8_initializer.cc)
    v8_isolate_memory_dump_provider.cc
    v8_platform.cc
    wrappable.cc
    wrapper_info.cc
    modules/console.cc
    modules/file_module_provider.cc
    modules/module_registry.cc
    modules/module_runner_delegate.cc
    modules/timer.cc
)

add_library(gin STATIC ${GIN_SOURCES})

target_include_directories(gin PRIVATE
    "${CMAKE_SOURCE_DIR}"                              # gin/..., base/... (root base wins)
    "${CMAKE_SOURCE_DIR}/orig_chrome"                 # base/ subsystems not in trimmed base/
    "${CMAKE_SOURCE_DIR}/electron"                    # cef/include/...
    "${CMAKE_SOURCE_DIR}/electron/cef"                # CEF-internal "include/..."
    "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source"   # wtf/...
    "${MINIBLINK_V8_ROOT}/include"                     # <v8.h>
    "${MINIBLINK_V8_ROOT}/.."                          # "v8/include/v8.h" style
)

# Must match the V8 monolith ABI (pointer compression).
target_compile_definitions(gin PRIVATE
    V8_COMPRESS_POINTERS
    V8_31BIT_SMIS_ON_64BIT_ARCH
)
set_target_properties(gin PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(gin PRIVATE
        -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types
        -Wno-error=int-conversion)
endif()
add_dependencies(gin base)
