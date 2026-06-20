# Smoke test: skia -> CoreGraphics paint bridge round-trips pixels (headless).
add_executable(cg_bridge_smoketest cg_bridge_smoketest.mm)
target_link_libraries(cg_bridge_smoketest PRIVATE miniblink_mac_port skia)
set_target_properties(cg_bridge_smoketest PROPERTIES CXX_STANDARD 14)
add_test(NAME cg_bridge_smoketest COMMAND cg_bridge_smoketest)

# macOS web-view paint primitive (orientation + draw).
add_executable(web_view_mac_smoketest web_view_mac_smoketest.mm)
target_link_libraries(web_view_mac_smoketest PRIVATE miniblink_mac_port skia)
set_target_properties(web_view_mac_smoketest PROPERTIES CXX_STANDARD 14)
add_test(NAME web_view_mac_smoketest COMMAND web_view_mac_smoketest)
