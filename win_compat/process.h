// macOS shim for the MSVC CRT <process.h>. wkeSimpleDownload.h includes this header
// for _beginthreadex, which is already provided (pthread-backed) by win_compat/windows.h
// (force-included on the macOS build). This shim only needs to make the include resolve.
//
// Windows builds use the real CRT <process.h>; this file is only on the include path
// for the non-MSVC (macOS) build.
#pragma once
