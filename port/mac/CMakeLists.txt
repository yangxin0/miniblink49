# macOS platform backend (port/mac) — the from-scratch Cocoa/CoreGraphics layer
# that replaces miniblink's Win32 backend. Starts with the skia->CoreGraphics
# paint bridge; the NSView/NSWindow windowing + event translation land here next.

set(MAC_PORT_SOURCES skia_cg_bridge.mm web_view_mac.mm)
list(TRANSFORM MAC_PORT_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

add_library(miniblink_mac_port STATIC ${MAC_PORT_SOURCES})

target_include_directories(miniblink_mac_port PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(miniblink_mac_port PUBLIC
    skia "-framework CoreGraphics" "-framework CoreFoundation" "-framework Cocoa")
set_target_properties(miniblink_mac_port PROPERTIES CXX_STANDARD 14)
