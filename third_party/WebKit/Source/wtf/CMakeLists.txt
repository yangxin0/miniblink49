# WTF — WebKit's foundation library (Phase 9 macOS port).
# WTF requires config.h to be force-included (it sets up OS()/COMPILER()/USE()
# and the WTF_OS_MACOSX platform macros).

set(WSRC "${CMAKE_SOURCE_DIR}/third_party/WebKit/Source")

# Portable + posix/mac sources. Excludes the Windows variants (ThreadingWin,
# ThreadSpecificWin) and the ICU text codec (ICU is disabled in this tree).
set(WTF_SOURCES
    AddressSpaceRandomization.cpp
    ArrayBuffer.cpp
    ArrayBufferBuilder.cpp
    ArrayBufferContents.cpp
    ArrayBufferView.cpp
    ArrayPiece.cpp
    Assertions.cpp
    BitVector.cpp
    CryptographicallyRandomNumber.cpp
    CurrentTime.cpp
    DataLog.cpp
    DateMath.cpp
    DefaultAllocator.cpp
    dtoa.cpp
    DynamicAnnotations.cpp
    FastMalloc.cpp
    FilePrintStream.cpp
    HashTable.cpp
    InstanceCounter.cpp
    MainThread.cpp
    PageAllocator.cpp
    PartitionAlloc.cpp
    PartitionAllocator.cpp
    Partitions.cpp
    PrintStream.cpp
    RefCountedLeakCounter.cpp
    SizeLimits.cpp
    TypeTraits.cpp
    WTF.cpp
    WTFThreadData.cpp
    text/AtomicString.cpp
    text/Base64.cpp
    text/CString.cpp
    text/StringBuilder.cpp
    text/StringConcatenate.cpp
    text/StringImpl.cpp
    text/StringStatics.cpp
    text/TextCodec.cpp
    text/TextCodecICU.cpp
    text/TextCodecLatin1.cpp
    text/TextCodecReplacement.cpp
    text/TextCodecUserDefined.cpp
    text/TextCodecUTF16.cpp
    text/TextCodecUTF8.cpp
    text/TextEncoding.cpp
    text/TextEncodingRegistry.cpp
    text/TextPosition.cpp
    text/UTF8.cpp
    text/WTFString.cpp
    text/WTFStringUtil.cpp
    # double-conversion (dtoa.cpp above calls into these) and the qt4 Unicode
    # backend (QChar case folding, Unicode::toLower/toUpper). Needed once a real
    # consumer (blink) links WTF, not just compiles it.
    dtoa/bignum-dtoa.cc
    dtoa/bignum.cc
    dtoa/cached-powers.cc
    dtoa/diy-fp.cc
    dtoa/double-conversion.cc
    dtoa/fast-dtoa.cc
    dtoa/fixed-dtoa.cc
    dtoa/strtod.cc
    text/qt4/UnicodeQt4.cpp
)

if(APPLE)
    list(APPEND WTF_SOURCES
        ThreadingPthreads.cpp
        text/AtomicStringCF.cpp
        text/StringCF.cpp
        text/StringImplCF.cpp
        text/StringImplMac.mm   # WTF::StringImpl::operator NSString* (ObjC bridge)
        text/StringMac.mm       # WTF::String::String(NSString*)
    )
elseif(MB_OS_WINDOWS)
    # Win32 threading primitives (the posix/mac build uses ThreadingPthreads).
    list(APPEND WTF_SOURCES
        ThreadingWin.cpp
        ThreadSpecificWin.cpp
    )
endif()

list(TRANSFORM WTF_SOURCES PREPEND "${WSRC}/wtf/")

add_library(wtf STATIC ${WTF_SOURCES})
target_include_directories(wtf PUBLIC "${WSRC}" "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/orig_chrome")
if(MB_OS_WINDOWS)
    # Bundled ICU public headers (unicode/*.h) for TextCodecICU.cpp; macOS resolves
    # these via the system ICU it links (-licucore).
    target_include_directories(wtf PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/icu/source/common"
        "${CMAKE_SOURCE_DIR}/third_party/icu/source/i18n")
    # The minimal WTF-integrated ICU (u_charType, uscript_getScript/hasScript that
    # blink's shaping/break iteration need). macOS gets these from system libicucore;
    # on Windows compile it here so it inherits wtf's config.h force-include + Unicode
    # types (it pulls wtf/text/qt4/UnicodeQt4.h).
    target_sources(wtf PRIVATE
        "${CMAKE_SOURCE_DIR}/third_party/icu/source/uchar.cpp"
        # No-op stubs for the full-ICU normalization/property funcs harfbuzz's
        # hb-icu.cc needs (unorm2_*, u_getCombiningClass, ...) — not in the minimal
        # in-tree ICU; macOS links system libicucore for them.
        "${CMAKE_SOURCE_DIR}/third_party/icu/source/icu_hbicu_stubs_win.cpp")
endif()
# Force-include the WTF/blink config bootstrap (MSVC spells it /FI, clang -include).
if(MSVC)
    target_compile_options(wtf PRIVATE "/FI${WSRC}/config.h")
else()
    target_compile_options(wtf PRIVATE -include "${WSRC}/config.h")
endif()
# OFFICIAL_BUILD makes WTF_HEAP_PROFILER_TYPE_NAME a no-op (the non-official path
# needs getStringWithTypeName, which isn't available here).
target_compile_definitions(wtf PRIVATE OFFICIAL_BUILD)
if(NOT MSVC)
    target_compile_options(wtf PRIVATE
        -Wno-unused -Wno-deprecated-declarations -fdeclspec
        -Wno-error=incompatible-function-pointer-types -Wno-error=int-conversion)
endif()
if(APPLE)
    target_link_libraries(wtf PUBLIC "-framework CoreFoundation")
endif()
