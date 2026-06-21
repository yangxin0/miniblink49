# gin — the V8 integration layer (Chromium's gin), built against the pinned
# V8 8.7 and the engine's base/. Compiling this verifies the V8 7.5->8.7 API
# migration end-to-end. Built as a static lib (archive only) so it compile-
# verifies without needing every orig_chrome/base .cc linked.

# --- gin against V8 8.7 + orig_chrome base: these compile cleanly on macOS.
#     Proves the V8 7.5->8.7 API migration AND the base/CEF reconciliation.
set(GIN_SOURCES
    arguments.cc
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
    v8_initializer_empty.cc
    wrapper_info.cc
    wrappable.cc
    modules/console.cc
    modules/file_module_provider.cc
    modules/module_registry.cc
    modules/module_runner_delegate.cc
    modules/timer.cc
)

# array_buffer.cc uses WebKit/WTF (ArrayBufferContents); now that WTF builds, it
# compiles — but it needs WTF's config/flags, applied per-file below.
list(APPEND GIN_SOURCES array_buffer.cc)

# v8_platform.cc: gin's v8::Platform impl. Needed for V8::InitializePlatform
# (v8_initializer_empty.cc). Worker-thread scheduling is delegated to V8 8.7's
# NewDefaultPlatform via the third_party/v8shim DefaultPlatformWrap shim; the
# foreground task runner rides on blink's V8PerIsolateData. Needs WTF config +
# blink Platform headers, applied per-file below.
list(APPEND GIN_SOURCES v8_platform.cc)

# --- Remaining 2 files, blocked by SUBSYSTEMS not yet brought up on macOS (NOT
#     V8): base/trace_event and blink's Platform. Re-enable as those land.
set(GIN_SOURCES_SUBSYSTEM_DEP
    v8_isolate_memory_dump_provider.cc   # base/trace_event
    v8_platform.cc                       # blink Platform
)

add_library(gin STATIC ${GIN_SOURCES})

# array_buffer.cc pulls WTF headers: needs the WTF config force-include + WTF's
# build flags (which the other gin files don't).
# -fno-rtti: gin::ArrayBufferAllocator subclasses v8::ArrayBuffer::Allocator. With
# RTTI on, the derived typeinfo references the base typeinfo, which the V8 monolith
# (built -fno-rtti) does not export -> undefined "typeinfo for
# v8::ArrayBuffer::Allocator". Building this TU -fno-rtti drops the typeinfo chain.
set_source_files_properties(array_buffer.cc PROPERTIES COMPILE_OPTIONS
    "-include;${CMAKE_SOURCE_DIR}/third_party/WebKit/Source/config.h;-fdeclspec;-DOFFICIAL_BUILD;-fno-rtti")

# v8_platform.cc pulls blink Platform/WebThread + WTF headers; same config as
# array_buffer.cc (WTF config force-include + flags). FROM_HERE is only defined by
# WebTraceLocation.h under INSIDE_BLINK (a blink-impl-only macro); v8_task_runner.h
# uses it inline, so define INSIDE_BLINK for this TU to get the blink FROM_HERE.
set_source_files_properties(v8_platform.cc PROPERTIES COMPILE_OPTIONS
    "-include;${CMAKE_SOURCE_DIR}/third_party/WebKit/Source/config.h;-fdeclspec;-DOFFICIAL_BUILD;-DINSIDE_BLINK")

target_include_directories(gin PRIVATE
    "${CMAKE_SOURCE_DIR}"                              # gin/..., base/... (root base wins)
    "${CMAKE_SOURCE_DIR}/orig_chrome"                 # base/ subsystems not in trimmed base/
    "${CMAKE_SOURCE_DIR}/electron"                    # cef/include/...
    "${CMAKE_SOURCE_DIR}/electron/cef"                # CEF-internal "include/..."
    "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source"   # wtf/...
    "${CMAKE_SOURCE_DIR}/third_party/WebKit"          # public/platform/... (blink Platform/WebThread)
    "${CMAKE_SOURCE_DIR}/gen/blink"                   # generated blink core/ headers (InspectorTypeBuilder.h)
    "${CMAKE_SOURCE_DIR}/third_party/v8shim"          # v8_7_5/.../default_platform_wrap.h shim
    "${MINIBLINK_V8_ROOT}/include"                     # <v8.h>, libplatform/libplatform.h
    "${MINIBLINK_V8_ROOT}/.."                          # "v8/include/v8.h" style
)

# Must match the V8 monolith ABI (pointer compression).
target_compile_definitions(gin PRIVATE
    V8_COMPRESS_POINTERS V8_REVERSE_JSARGS

    V8_31BIT_SMIS_ON_64BIT_ARCH
    "V8CALL="
)
set_target_properties(gin PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(gin PRIVATE
        -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types
        -Wno-error=int-conversion)
endif()
add_dependencies(gin base)
