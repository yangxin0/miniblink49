# blink modules (macOS port) — the web-platform feature layer (webaudio, indexeddb,
# mediastream, webgl, geolocation, fetch, encoding, crypto, ...) on top of blink
# core. The trial link showed ~250 undefined symbols originate here. 538/549 sources
# compile on macOS; the rest are deferred per-feature stragglers.

set(MODSRC "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source/modules")

file(GLOB_RECURSE BLINK_MODULES_SRC "${MODSRC}/*.cpp"
    # The hand-written modules V8 glue (WebGLAny, V8BindingForModules,
    # ModuleBindingsInitializer, the modules serializers/dictionary helpers).
    "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source/bindings/modules/v8/*.cpp")
list(FILTER BLINK_MODULES_SRC EXCLUDE REGEX "Test\\.cpp$|TestHelper\\.cpp$")
# bindings/modules/v8/custom/* (V8Custom*Callback) still carry V8 7.5->8.7 skews; defer.
list(FILTER BLINK_MODULES_SRC EXCLUDE REGEX "/bindings/modules/v8/custom/")
# Per-feature stragglers (mostly promise/generated-binding-heavy or Win/API-skew):
# web MIDI, IndexedDB txn/db/request, WebRTC peer/req, MediaKeys/EME, fetch Body,
# crypto result, file-system, websocket channel, etc. Deferred per-file.
list(FILTER BLINK_MODULES_SRC EXCLUDE REGEX "BatteryManager\\.cpp$|DatabaseContext\\.cpp$|MIDI[A-Za-z]*\\.cpp$|NavigatorWebMIDI\\.cpp$|Notification\\.cpp$|PresentationAvailability\\.cpp$|ServiceWorkerRegistration\\.cpp$")
if(MB_OS_WINDOWS)
    # WebSQL needs a bundled sqlite3 (the trimmed tree has no sqlite3.h); macOS uses
    # the system sqlite. Disable the webdatabase module on Windows for now.
    list(FILTER BLINK_MODULES_SRC EXCLUDE REGEX "/webdatabase/")
else()
    list(FILTER BLINK_MODULES_SRC EXCLUDE REGEX "SQLiteFileSystemWin\\.cpp$")  # macOS uses the Posix variant
endif()
list(FILTER BLINK_MODULES_SRC EXCLUDE REGEX "CryptoResultImpl\\.cpp$|IDBTransaction\\.cpp$|IDBDatabase\\.cpp$|IDBOpenDBRequest\\.cpp$|IDBRequest\\.cpp$|PushMessageData\\.cpp$|MediaKeySession\\.cpp$|MediaKeys\\.cpp$|DOMFileSystemBase\\.cpp$|FileWriter\\.cpp$|DOMFileSystem\\.cpp$|PermissionStatus\\.cpp$|SpeechRecognition\\.cpp$|DataConsumerHandleTestUtil\\.cpp$|CompositorWorkerManager\\.cpp$")
list(FILTER BLINK_MODULES_SRC EXCLUDE REGEX "RTCDTMFSender\\.cpp$|RTCSessionDescriptionRequestImpl\\.cpp$|RTCVoidRequestImpl\\.cpp$|RTCStatsRequestImpl\\.cpp$|MediaDevicesRequest\\.cpp$|RTCPeerConnection\\.cpp$")

add_library(blink_modules STATIC ${BLINK_MODULES_SRC})
target_link_libraries(blink_modules PUBLIC blink_core)
target_include_directories(blink_modules PUBLIC
    "${CMAKE_SOURCE_DIR}/third_party/v8shim" "${CMAKE_SOURCE_DIR}/third_party/khronos")
# modules is blink-internal code (uses WebNode::unwrap etc.) -> needs
# BLINK_IMPLEMENTATION like blink_web. Gated bits are methods/visibility, not
# layout, so it still links against core/platform (built without it).
target_compile_definitions(blink_modules PUBLIC "V8CALL=" ENABLE_WKE=1 BLINK_IMPLEMENTATION=1
    # Must match the V8 monolith ABI (pointer compression) — these files include
    # v8.h but don't link v8_monolith, so they don't inherit its defs.
    V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH V8_REVERSE_JSARGS)
set_target_properties(blink_modules PROPERTIES CXX_STANDARD 14)
if(NOT MSVC)
    target_compile_options(blink_modules PRIVATE
        -fdeclspec -fno-exceptions -Wno-unused -Wno-deprecated-declarations
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
