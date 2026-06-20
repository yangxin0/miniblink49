# base — the engine's (trimmed Chromium) base library.
#
# Historically Win32-only: it ships *_win.cc implementations and no posix/mac
# ones. This target compiles the portable sources on macOS and adds posix/mac
# implementations under base/port_posix/ for the platform-specific pieces.
# This is the foundation gin/ and wke/ depend on.

# Portable sources (compiled on every platform).
set(BASE_SOURCES
    command_line.cc
    debug/alias.cc
    files/file_path_constants.cc
    files/file_path.cc
    files/file.cc
    files/scoped_file.cc
    json/json_parser.cc
    json/json_reader.cc
    json/json_util.cc
    json/json_writer.cc
    pickle.cc
    process/memory.cc
    rand_util.cc
    strings/string_piece.cc
    strings/string_util.cc
    strings/utf_offset_string_conversions.cc
    strings/utf_string_conversion_utils.cc   # pulled from orig_chrome
    third_party/icu/icu_utf.cc               # pulled from orig_chrome
    third_party/nspr/prtime.cc               # pulled from orig_chrome
    thread.cc
    time/time.cc
    values.cc
)

if(MB_OS_WINDOWS)
    list(APPEND BASE_SOURCES
        debug/gdi_debug_util_win.cc
        file_version_info_win.cc
        files/file_win.cc
        files/scoped_platform_file_closer.cc
        platform_file_win.cc
        platform_file.cc
        process/memory_win.cc
        rand_util_win.cc
        time/time_win.cc
        win/scoped_handle.cc
        win/shell.cc
        win/shortcut.cc
        win/windows_version.cc
    )
else()
    # POSIX/macOS platform implementations are added incrementally below as the
    # build surfaces missing symbols. (none wired yet — discovery pass)
endif()

add_library(base STATIC ${BASE_SOURCES})
target_include_directories(base PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/electron"               # CEF headers: cef/include/...
    "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source"  # WTF headers: wtf/...
)
if(APPLE)
    # macOS filesystem paths are UTF-8: take FilePath's native-UTF8 code paths
    # (avoids SysNativeMBToWide/locale conversions).
    target_compile_definitions(base PRIVATE SYSTEM_NATIVE_UTF8)
endif()
if(NOT MSVC)
    target_compile_options(base PRIVATE -Wno-unused -Wno-deprecated-declarations)
endif()
