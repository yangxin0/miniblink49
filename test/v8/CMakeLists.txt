# Smoke test: link a tiny program against the pinned V8 8.7 monolith and run JS.
# Verifies the cross-platform build can actually execute JavaScript.
add_executable(v8_smoketest v8_smoketest.cc)
target_link_libraries(v8_smoketest PRIVATE v8_monolith)
set_target_properties(v8_smoketest PROPERTIES CXX_STANDARD 17)
if(APPLE)
    target_link_libraries(v8_smoketest PRIVATE "-framework CoreFoundation")
endif()
add_test(NAME v8_smoketest COMMAND v8_smoketest)
