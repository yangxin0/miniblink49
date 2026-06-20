# Smoke test: link a tiny program against the macOS skia raster build and render.
# Verifies skia (blink's 2D graphics library) works end-to-end on this platform.
add_executable(skia_smoketest skia_smoketest.cc)
target_link_libraries(skia_smoketest PRIVATE skia)
set_target_properties(skia_smoketest PROPERTIES CXX_STANDARD 14)
add_test(NAME skia_smoketest COMMAND skia_smoketest)
