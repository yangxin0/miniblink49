// Minimal <windows.h> compatibility shim for the macOS port.
//
// miniblink's public wke API (wke/wke.h) is intentionally Win32-shaped: the host
// window is an HWND, messages are (UINT, WPARAM, LPARAM), etc. To build that API
// on macOS we provide the Win32 *types* it names. The window HANDLE types are
// opaque pointers here; the Cocoa windowing backend (separate) maps HWND<->NSView.
//
// This is types-only: the Win32 *functions* (CreateWindow, GetDC, ...) used by
// the wke windowing files are not provided — those files need the real Cocoa
// backend. The portable wke files (JS binding, string, net hook, globals) only
// need these typedefs.
//
// Widths match Win32 (LLP64): DWORD/LONG are 32-bit, WCHAR is 16-bit. All guarded
// so this coexists with the smaller per-module shims elsewhere in the tree.

#ifndef MINIBLINK_WIN_COMPAT_WINDOWS_H_
#define MINIBLINK_WIN_COMPAT_WINDOWS_H_

#if defined(_WIN32)
#error "win_compat/windows.h is the non-Windows shim; do not use it on Windows"
#endif

#include <stdint.h>
#include <stddef.h>

// MSVC sized-integer keywords used in the wke sources.
#ifndef _MSC_VER
#define __int8   char
#define __int16  short
#define __int32  int
#define __int64  long long
// Calling-convention annotations: no-ops off Windows.
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
// Win32 API import/export decorations -> no-ops in the shim.
#ifndef WINBASEAPI
#define WINBASEAPI
#endif
#ifndef WINUSERAPI
#define WINUSERAPI
#endif
#ifndef WINADVAPI
#define WINADVAPI
#endif
#ifndef WINGDIAPI
#define WINGDIAPI
#endif
#endif

// --- Scalar types (Win32 widths) --------------------------------------------
#ifndef _WINDOWS_SCALARS_DEFINED
#define _WINDOWS_SCALARS_DEFINED
// BOOL collides with Apple's <objc/objc.h>, which unconditionally typedefs it
// (`bool` on arm64, `signed char` on legacy Intel) with no suppression guard.
// Redefining it as `int` here clashes regardless of include order, so on Apple
// platforms we pull objc.h's canonical BOOL and don't define our own; its
// width never matters to our (non-Win32-ABI) shim code. Elsewhere use Win32 int.
#if defined(__APPLE__)
#include <objc/objc.h>
#else
typedef int                 BOOL;
#endif
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef uint32_t            DWORD;
typedef int32_t             LONG;
typedef uint32_t            ULONG;
typedef int64_t             LONGLONG;
typedef uint64_t            ULONGLONG;
typedef unsigned int        UINT;
typedef int                 INT;
typedef float               FLOAT;
typedef unsigned short      WCHAR;   // UTF-16 code unit (Win32 wchar_t is 16-bit)
typedef char                CHAR;
typedef DWORD               COLORREF;
#endif

// Pointer-sized integers + message params.
typedef intptr_t            INT_PTR;
typedef uintptr_t           UINT_PTR;
typedef intptr_t            LONG_PTR;
typedef uintptr_t           ULONG_PTR;
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;

// --- Handles (opaque on macOS) ----------------------------------------------
#define DECLARE_HANDLE(name) typedef struct name##__ { int unused; } *name
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HCURSOR);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HMODULE);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HFONT);
typedef void*               HANDLE;
typedef void*               HGLOBAL;
typedef void*               LPVOID;

// --- Strings -----------------------------------------------------------------
typedef CHAR*               LPSTR;
typedef const CHAR*         LPCSTR;
typedef WCHAR*              LPWSTR;
typedef const WCHAR*        LPCWSTR;

// --- Structs -----------------------------------------------------------------
typedef struct tagPOINT  { LONG x, y; } POINT, *LPPOINT;
typedef struct tagSIZE   { LONG cx, cy; } SIZE, *LPSIZE;
typedef struct tagRECT   { LONG left, top, right, bottom; } RECT, *LPRECT;

// Process-creation structs named in the wke API (node CreateProcess callback).
typedef struct _STARTUPINFOW {
    DWORD  cb;
    LPWSTR lpReserved, lpDesktop, lpTitle;
    DWORD  dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars;
    DWORD  dwFillAttribute, dwFlags;
    WORD   wShowWindow, cbReserved2;
    BYTE*  lpReserved2;
    HANDLE hStdInput, hStdOutput, hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;
typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess, hThread;
    DWORD  dwProcessId, dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

// --- Threading / timing primitives (posix-backed) ----------------------------
// The wke/net sources use Win32 critical sections, Sleep and a few helpers. Map
// them to pthreads/unistd so the portable (non-windowing) parts build and run.
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

typedef pthread_mutex_t CRITICAL_SECTION, *LPCRITICAL_SECTION;

static inline void InitializeCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutexattr_t a; pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);  // Win32 CS is recursive
    pthread_mutex_init(cs, &a); pthread_mutexattr_destroy(&a);
}
static inline void EnterCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_lock(cs); }
static inline void LeaveCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_unlock(cs); }
static inline void DeleteCriticalSection(CRITICAL_SECTION* cs) { pthread_mutex_destroy(cs); }
static inline BOOL TryEnterCriticalSection(CRITICAL_SECTION* cs) { return pthread_mutex_trylock(cs) == 0; }
static inline void Sleep(DWORD ms) { usleep((useconds_t)ms * 1000); }
static inline DWORD GetCurrentThreadId(void) { return (DWORD)(uintptr_t)pthread_self(); }

// Interlocked atomics -> gcc/clang __sync builtins.
static inline LONG InterlockedIncrement(volatile LONG* v) { return __sync_add_and_fetch(v, 1); }
static inline LONG InterlockedDecrement(volatile LONG* v) { return __sync_sub_and_fetch(v, 1); }
static inline LONG InterlockedExchangeAdd(volatile LONG* v, LONG a) { return __sync_fetch_and_add(v, a); }
static inline LONG InterlockedCompareExchange(volatile LONG* v, LONG ex, LONG cmp) { return __sync_val_compare_and_swap(v, cmp, ex); }

// --- Dynamic loading + misc (posix-backed) -----------------------------------
// wke's public header (wkeInitializeEx) loads the wke library via LoadLibrary/
// GetProcAddress. Map to dlopen/dlsym (HMODULE is the dl handle).
#include <dlfcn.h>
#include <stdio.h>

static inline HMODULE LoadLibraryA(const char* path) {
    return path ? (HMODULE)dlopen(path, RTLD_NOW) : (HMODULE)0;
}
static inline HMODULE LoadLibraryW(const wchar_t* path) {
    if (!path) return (HMODULE)0;
    char buf[1024]; size_t i = 0;
    for (; path[i] && i < sizeof(buf) - 1; ++i) buf[i] = (char)path[i];  // ASCII path
    buf[i] = 0;
    return (HMODULE)dlopen(buf, RTLD_NOW);
}
static inline void* GetProcAddress(HMODULE module, const char* name) {
    return module ? dlsym((void*)module, name) : (void*)0;
}
static inline BOOL FreeLibrary(HMODULE module) {
    return module ? (dlclose((void*)module) == 0) : 0;
}
static inline int MessageBoxA(HWND, const char* text, const char* caption, unsigned) {
    fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "", text ? text : "");
    return 0;
}
static inline void OutputDebugStringW(const wchar_t* s) {
    if (!s) return;
    for (const wchar_t* p = s; *p; ++p) fputc((int)(*p & 0x7F), stderr);
}
#ifndef OutputDebugStringA
static inline void OutputDebugStringA(const char* s) {
    if (s) fputs(s, stderr);
}
#endif

// Wide fopen / file move used by net/ — convert the wide (ASCII) paths and defer
// to the posix calls.
static inline void mb_wide_to_narrow_(const wchar_t* w, char* out, size_t cap) {
    size_t i = 0; for (; w && w[i] && i < cap - 1; ++i) out[i] = (char)w[i]; out[i] = 0;
}
static inline FILE* _wfopen(const wchar_t* path, const wchar_t* mode) {
    char p[1024], m[16]; mb_wide_to_narrow_(path, p, sizeof(p)); mb_wide_to_narrow_(mode, m, sizeof(m));
    return fopen(p, m);
}
static inline BOOL MoveFileExW(const wchar_t* from, const wchar_t* to, DWORD) {
    char a[1024], b[1024]; mb_wide_to_narrow_(from, a, sizeof(a)); mb_wide_to_narrow_(to, b, sizeof(b));
    return rename(a, b) == 0;
}

// --- Common constants --------------------------------------------------------
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL  0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// --- GUI message / window-class types (content/* page-host backend) ----------
// These are referenced by the Win32 windowing path that the macOS Cocoa backend
// replaces; declared here so the shared headers parse on macOS. The actual
// window procedure / paint structs are never invoked on macOS (Cocoa drives it).
typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *LPMSG, *PMSG;

typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef struct tagPAINTSTRUCT {
    HDC  hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore, fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagWNDCLASSW {
    UINT      style;
    WNDPROC   lpfnWndProc;
    int       cbClsExtra, cbWndExtra;
    HINSTANCE hInstance;
    HICON     hIcon;
    HCURSOR   hCursor;
    HBRUSH    hbrBackground;
    LPCWSTR   lpszMenuName, lpszClassName;
} WNDCLASSW, *LPWNDCLASSW;
typedef WNDCLASSW WNDCLASS;  // narrow/wide unified on the macOS shim

typedef struct tagTRACKMOUSEEVENT {
    DWORD cbSize, dwFlags;
    HWND  hwndTrack;
    DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

#endif // MINIBLINK_WIN_COMPAT_WINDOWS_H_
