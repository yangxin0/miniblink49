# Smoke test: link against blink_platform geometry and run its algorithms.
# Verifies blink's foundational geometry layer builds, links (WTF + skia), and
# computes correctly on macOS.
add_executable(blink_geometry_smoketest blink_geometry_smoketest.cc)
target_link_libraries(blink_geometry_smoketest PRIVATE blink_platform)
set_target_properties(blink_geometry_smoketest PROPERTIES CXX_STANDARD 14)
add_test(NAME blink_geometry_smoketest COMMAND blink_geometry_smoketest)

# Oilpan GC: prove the garbage collector initializes, allocates, and reclaims.
add_executable(blink_heap_smoketest blink_heap_smoketest.cc)
target_link_libraries(blink_heap_smoketest PRIVATE blink_platform)
set_target_properties(blink_heap_smoketest PROPERTIES CXX_STANDARD 14)
add_test(NAME blink_heap_smoketest COMMAND blink_heap_smoketest)
