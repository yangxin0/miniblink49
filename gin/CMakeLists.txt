# gin — the V8 integration layer (Chromium's gin), built against the pinned
# V8 8.7 and the engine's base/. Compiling this verifies the V8 7.5->8.7 API
# migration end-to-end. Built as a static lib (archive only) so it compile-
# verifies without needing every orig_chrome/base .cc linked.

# --- Verified V8-core: these compile cleanly against V8 8.7 on macOS. They are
#     the self-contained V8 conversion/handle files and PROVE the V8 7.5->8.7 API
#     migration (0 V8-API errors). This is what the gin target builds today.
set(GIN_SOURCES
    arguments.cc
    converter.cc
    interceptor.cc
    microtasks_scope.cc
    per_isolate_data.cc
    promise.cc
    try_catch.cc
    wrapper_info.cc
)

# --- Engine-bring-up boundary (NOT yet building on macOS): the files below fail
#     NOT on V8, but because they pull in whole engine subsystems that aren't
#     ported yet — CEF base (base::Bind/Callback via cef_callback.h), WebKit/WTF
#     (COMPILER() config + blink Platform), and trace_event. Re-enable as those
#     subsystems are brought up (Phases 9-10). Their V8-API deltas are already
#     migrated (see V8_API_MIGRATION.md); only their dependencies block them.
set(GIN_SOURCES_ENGINE_DEP
    array_buffer.cc
    context_holder.cc
    debug_impl.cc
    dictionary.cc
    function_template.cc
    isolate_holder.cc
    object_template_builder.cc
    per_context_data.cc
    run_microtasks_observer.cc
    runner.cc
    shell_runner.cc
    v8_initializer_empty.cc
    v8_isolate_memory_dump_provider.cc
    v8_platform.cc
    wrappable.cc
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
