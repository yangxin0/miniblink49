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
    text/StringStatics.cpp
    text/TextCodec.cpp
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
)

# --- 4 files fenced (deep WTF platform/version tail, TODO): the allocator
#     glue (PartitionAllocator/Partitions: Partitions::fastMalloc API skew),
#     StringImpl (Win VirtualAlloc -> needs mmap), WTFStringUtil (Win types).
set(WTF_SOURCES_TODO
    PartitionAllocator.cpp
    Partitions.cpp
    text/StringImpl.cpp
    text/WTFStringUtil.cpp
)

if(APPLE)
    list(APPEND WTF_SOURCES
        ThreadingPthreads.cpp
        text/AtomicStringCF.cpp
        text/StringCF.cpp
        text/StringImplCF.cpp
    )
endif()

list(TRANSFORM WTF_SOURCES PREPEND "${WSRC}/wtf/")

add_library(wtf STATIC ${WTF_SOURCES})
target_include_directories(wtf PUBLIC "${WSRC}" "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/orig_chrome")
# Force-include the WTF/blink config bootstrap.
target_compile_options(wtf PRIVATE -include "${WSRC}/config.h")
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
