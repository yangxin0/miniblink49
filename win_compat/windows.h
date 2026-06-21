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

// Pre-parse Apple's MacTypes (via CoreFoundation) BEFORE any translation-unit body.
// This shim is force-included right after config.h, so MacTypes' Fixed/Rect/RGBColor
// typedefs are defined while only the global namespace is in scope. Without this, a
// content TU that does `using namespace blink;` and later pulls CoreFoundation (e.g.
// through wtf/RetainPtr.h) hits an ambiguity between ::Fixed and blink::Fixed (the
// LengthType enumerator) inside MacTypes.h itself. Parsing it first avoids that.
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

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
static inline LONG InterlockedExchange(volatile LONG* v, LONG val) { return __sync_lock_test_and_set(v, val); }

// --- Worker threads (_beginthreadex + WaitForSingleObject/CloseHandle) --------
// net/websocket spins a worker thread via the CRT _beginthreadex and joins it by
// casting the returned id to HANDLE for WaitForSingleObject + CloseHandle. Back
// it with a heap-allocated pthread_t; the HANDLE is a pointer to that wrapper.
//
// Gated: content/web_impl_win/WebThreadImpl.cpp ships its own self-consistent
// event+thread shim (CreateEvent/SetEvent/WaitForSingleObject/CloseHandle share a
// tagged HandleBase* encoding) which would collide with these. That TU's target
// defines WIN_COMPAT_NO_THREAD_PRIMS to opt out; the websocket TU uses these.
#ifndef WIN_COMPAT_NO_THREAD_PRIMS
#include <stdlib.h>
#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#endif

typedef struct mb_thread_handle_ { pthread_t tid; } mb_thread_handle_t;

// Trampoline: Win32 thread procs are 'unsigned __stdcall(void*)'; pthread wants
// 'void*(void*)'. We stash both in a small heap record.
typedef struct mb_thread_start_ { unsigned (*proc)(void*); void* arg; } mb_thread_start_t;
static inline void* mb_thread_trampoline_(void* p) {
    mb_thread_start_t s = *(mb_thread_start_t*)p;
    free(p);
    s.proc(s.arg);
    return 0;
}
static inline uintptr_t _beginthreadex(void* /*security*/, unsigned /*stacksize*/,
    unsigned (*start)(void*), void* arglist, unsigned /*initflag*/, unsigned* thrdaddr) {
    mb_thread_handle_t* h = (mb_thread_handle_t*)malloc(sizeof(mb_thread_handle_t));
    if (!h) return 0;
    mb_thread_start_t* s = (mb_thread_start_t*)malloc(sizeof(mb_thread_start_t));
    if (!s) { free(h); return 0; }
    s->proc = start; s->arg = arglist;
    if (pthread_create(&h->tid, 0, mb_thread_trampoline_, s) != 0) {
        free(s); free(h); return 0;
    }
    if (thrdaddr) *thrdaddr = 0;
    return (uintptr_t)h;
}
static inline DWORD WaitForSingleObject(HANDLE handle, DWORD /*ms*/) {
    mb_thread_handle_t* h = (mb_thread_handle_t*)handle;
    if (!h) return (DWORD)-1;
    pthread_join(h->tid, 0);
    return WAIT_OBJECT_0;
}
static inline BOOL CloseHandle(HANDLE handle) {
    if (handle) free(handle);
    return TRUE;
}
#endif // WIN_COMPAT_NO_THREAD_PRIMS

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
#ifndef MOVEFILE_REPLACE_EXISTING
#define MOVEFILE_REPLACE_EXISTING 0x00000001
#endif

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

typedef struct tagPOINTL { LONG x, y; } POINTL, *PPOINTL;

// =====================================================================================
// Win32 USER/GDI surface for the content/ page-host & input translator.
//
// On macOS the Cocoa backend drives windowing, paint and input (see port/mac/* and
// content/web_impl_mac/*), so the Win32 message-pump path in content/browser/* is never
// executed here. These declarations exist only so that shared, Win32-shaped code parses
// and links; the function stubs are deliberately inert no-ops. Each symbol is #ifndef-
// guarded so a file that defines its own local shim (e.g. WebThreadImpl.cpp) still wins.
// =====================================================================================

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#endif

// --- Color (GDI) -------------------------------------------------------------
#ifndef RGB
#define RGB(r,g,b)   ((COLORREF)(((BYTE)(r))|((WORD)((BYTE)(g))<<8)|(((DWORD)(BYTE)(b))<<16)))
#define GetRValue(c) ((BYTE)(c))
#define GetGValue(c) ((BYTE)(((WORD)(c))>>8))
#define GetBValue(c) ((BYTE)((c)>>16))
#endif

// --- Word/param packing macros ----------------------------------------------
#ifndef LOWORD
#define LOWORD(l)  ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)  ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffff))
#define LOBYTE(w)  ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)  ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))
#define MAKELONG(a,b)   ((LONG)(((WORD)(a))|(((DWORD)((WORD)(b)))<<16)))
#define MAKELPARAM(l,h) ((LPARAM)MAKELONG(l,h))
#define MAKEWPARAM(l,h) ((WPARAM)MAKELONG(l,h))
#define GET_WHEEL_DELTA_WPARAM(w) ((short)HIWORD(w))
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#ifndef DWORD_PTR
typedef ULONG_PTR DWORD_PTR;
#endif

// --- Virtual-key codes (Win32 standard values) -------------------------------
#ifndef VK_RETURN
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#endif

// --- Window messages & mouse-key flags --------------------------------------
#ifndef WM_MOUSEMOVE
#define WM_SETCURSOR 0x0020
#define WM_SYSCOMMAND 0x0112
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MBUTTONDBLCLK 0x0209
#define WM_MOUSEWHEEL 0x020A
#define WM_MOUSELEAVE 0x02A3
#define WM_TIMER 0x0113
#define WM_IME_CHAR 0x0286
#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT 0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010
#define HTCAPTION 2
#define HTCLIENT 1
#define SC_MOVE 0xF010
#define KF_EXTENDED 0x0100
#define WHEEL_DELTA 120
#define WHEEL_PAGESCROLL 0xFFFFFFFF
#define SPI_GETWHEELSCROLLLINES 0x0068
#define SPI_GETWHEELSCROLLCHARS 0x006C
#endif

// --- Window styles -----------------------------------------------------------
#ifndef WS_VISIBLE
#define WS_VISIBLE 0x10000000L
#define WS_OVERLAPPEDWINDOW 0x00CF0000L
#define WS_CHILD 0x40000000L
#define WS_EX_LAYERED 0x00080000L
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWLP_USERDATA (-21)
#define CW_USEDEFAULT ((int)0x80000000)
#endif

// --- ShellExecute show commands ----------------------------------------------
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif

// --- GDI region combine modes ------------------------------------------------
#ifndef RGN_OR
#define RGN_AND 1
#define RGN_OR 2
#define RGN_XOR 3
#define RGN_DIFF 4
#define RGN_COPY 5
#endif

// --- Standard cursor IDs -----------------------------------------------------
#ifndef IDC_ARROW
#define IDC_ARROW       ((const WCHAR*)32512)
#define IDC_IBEAM       ((const WCHAR*)32513)
#define IDC_WAIT        ((const WCHAR*)32514)
#define IDC_CROSS       ((const WCHAR*)32515)
#define IDC_SIZEALL     ((const WCHAR*)32646)
#define IDC_SIZENWSE    ((const WCHAR*)32642)
#define IDC_SIZENESW    ((const WCHAR*)32643)
#define IDC_SIZEWE      ((const WCHAR*)32644)
#define IDC_SIZENS      ((const WCHAR*)32645)
#define IDC_HAND        ((const WCHAR*)32649)
#define IDC_HELP        ((const WCHAR*)32651)
#define IDC_NO          ((const WCHAR*)32648)
#define IDC_APPSTARTING ((const WCHAR*)32650)
#endif

// --- USER32 input/window functions (inert no-ops; Cocoa drives input on mac) -
#ifndef MINIBLINK_WIN_COMPAT_USER_STUBS
#define MINIBLINK_WIN_COMPAT_USER_STUBS
static inline short  GetKeyState(int) { return 0; }
static inline BOOL   GetCursorPos(LPPOINT p) { if (p) { p->x = 0; p->y = 0; } return TRUE; }
static inline BOOL   ClientToScreen(HWND, LPPOINT) { return TRUE; }
static inline BOOL   ScreenToClient(HWND, LPPOINT) { return TRUE; }
static inline HWND   SetCapture(HWND) { return NULL; }
static inline BOOL   ReleaseCapture(void) { return TRUE; }
static inline BOOL   IsWindow(HWND) { return FALSE; }
static inline BOOL   IsWindowVisible(HWND) { return FALSE; }
static inline HWND   GetFocus(void) { return NULL; }
static inline HWND   SetFocus(HWND) { return NULL; }
static inline HWND   GetParent(HWND) { return NULL; }
static inline HWND   GetActiveWindow(void) { return NULL; }
static inline BOOL   GetClientRect(HWND, LPRECT r) { if (r) { r->left=r->top=r->right=r->bottom=0; } return TRUE; }
static inline BOOL   GetWindowRect(HWND, LPRECT r) { if (r) { r->left=r->top=r->right=r->bottom=0; } return TRUE; }
static inline BOOL   PtInRect(const RECT*, POINT) { return FALSE; }
static inline UINT   GetDoubleClickTime(void) { return 500; }
static inline BOOL   SystemParametersInfoW(UINT, UINT, void*, UINT) { return FALSE; }
// Templated on the char type: callers pass either L"..." (wchar_t, 32-bit on macOS)
// or a WCHAR* (16-bit); both must bind to these inert stubs.
template<typename T> static inline HMODULE GetModuleHandleW(const T*) { return NULL; }
static inline HMODULE GetModuleHandleA(const char*) { return NULL; }
static inline DWORD  GetTickCount(void) { return (DWORD)(GetCurrentThreadId()); }
template<typename T> static inline HCURSOR LoadCursorW(HINSTANCE, const T*) { return NULL; }
static inline HCURSOR SetCursor(HCURSOR) { return NULL; }
static inline BOOL   DestroyIcon(HICON) { return TRUE; }
static inline BOOL   ShowWindow(HWND, int) { return FALSE; }
static inline BOOL   UpdateWindow(HWND) { return FALSE; }
static inline BOOL   DestroyWindow(HWND) { return FALSE; }
static inline BOOL   EnableWindow(HWND, BOOL) { return FALSE; }
static inline BOOL   SetForegroundWindow(HWND) { return FALSE; }
static inline BOOL   PostMessageW(HWND, UINT, WPARAM, LPARAM) { return FALSE; }
static inline BOOL   KillTimer(HWND, UINT_PTR) { return FALSE; }
static inline LONG_PTR GetWindowLongPtrW(HWND, int) { return 0; }
static inline LONG_PTR SetWindowLongPtrW(HWND, int, LONG_PTR) { return 0; }
#define GetWindowLongPtr GetWindowLongPtrW
#define SetWindowLongPtr SetWindowLongPtrW
// GDI region (used by the Win draggable-region path; inert on macOS)
static inline HRGN   CreateRectRgn(int, int, int, int) { return NULL; }
static inline int    SetRectRgn(HRGN, int, int, int, int) { return 0; }
static inline int    CombineRgn(HRGN, HRGN, HRGN, int) { return 0; }
template<typename T> static inline BOOL DeleteObject(T) { return TRUE; }
#define GetModuleHandle GetModuleHandleW
#define SystemParametersInfo SystemParametersInfoW
#define LoadCursor LoadCursorW
#define PostMessage PostMessageW
#endif

// =====================================================================================
// Additional Win32 surface used by the wke public API (wke/wke2.cpp, wkeWebView.cpp,
// wkeWebWindow.cpp, wkeSimpleDownload.h). These are standard Win32 constants and inert
// function stubs so the shared, Win32-shaped wke source parses and the libwke archive
// links on macOS; the real windowing / file-dialog / paint behaviour is wired through
// the Cocoa layer (port/mac/*) at the app layer. Additive and #ifndef-guarded so the
// Windows build (real <windows.h>) is unaffected.
// =====================================================================================

// --- More window styles ------------------------------------------------------
#ifndef WS_POPUP
#define WS_POPUP        0x80000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#endif

// --- Window class styles -----------------------------------------------------
#ifndef CS_HREDRAW
#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#endif

// --- Standard icon IDs -------------------------------------------------------
#ifndef IDI_APPLICATION
#define IDI_APPLICATION ((const WCHAR*)32512)
#endif

// --- More window messages ----------------------------------------------------
#ifndef WM_CREATE
#define WM_CREATE       0x0001
#define WM_DESTROY      0x0002
#define WM_QUIT         0x0012
#define PM_REMOVE       0x0001
#define WM_SIZE         0x0005
#define WM_PAINT        0x000F
#define WM_CLOSE        0x0010
#define WM_ERASEBKGND   0x0014
#define WM_CANCELMODE   0x001F
#define WM_GETDLGCODE   0x0087
#define WM_NCPAINT      0x0085
#define WM_NCDESTROY    0x0082
#define WM_KEYDOWN      0x0100
#define WM_KEYUP        0x0101
#define WM_CHAR         0x0102
#define WM_SYSKEYDOWN   0x0104
#define WM_SYSKEYUP     0x0105
#define WM_SYSCHAR      0x0106
#define WM_SETFOCUS     0x0007
#define WM_KILLFOCUS    0x0008
#define WM_CONTEXTMENU  0x007B
#define WM_DROPFILES    0x0233
// NOTE: WM_TOUCH (0x0240) is intentionally NOT defined here. content/browser/TouchStruct.h
// defines it under #ifndef WM_TOUCH together with the touch input types/flags; defining it
// here would suppress that block and break the content_browser build.
#define WM_IME_STARTCOMPOSITION 0x010D
#define WM_IME_ENDCOMPOSITION   0x010E
#define WM_IME_COMPOSITION      0x010F
#endif

// --- Key flags / short bounds ------------------------------------------------
#ifndef KF_REPEAT
#define KF_REPEAT 0x4000
#endif
#ifndef MINSHORT
#define MINSHORT (-32768)
#define MAXSHORT 0x7fff
#endif

// --- DLG codes ---------------------------------------------------------------
#ifndef DLGC_WANTARROWS
#define DLGC_WANTARROWS  0x0001
#define DLGC_WANTCHARS   0x0080
#define DLGC_WANTALLKEYS 0x0004
#endif

// --- File access constants ---------------------------------------------------
#ifndef GENERIC_READ
#define GENERIC_READ          0x80000000L
#define GENERIC_WRITE         0x40000000L
#define FILE_SHARE_READ       0x00000001
#define FILE_SHARE_WRITE      0x00000002
#define CREATE_ALWAYS         2
#define OPEN_EXISTING         3
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#endif

// --- GDI raster-op / device-caps constants -----------------------------------
#ifndef SRCCOPY
#define SRCCOPY (DWORD)0x00CC0020
#endif
#ifndef LOGPIXELSX
#define LOGPIXELSX 88
#define LOGPIXELSY 90
#endif

// --- Open/Save dialog flags --------------------------------------------------
#ifndef OFN_OVERWRITEPROMPT
#define OFN_OVERWRITEPROMPT 0x00000002
#define OFN_SHOWHELP        0x00000010
#endif

// --- SYSTEMTIME + GetLocalTime (real, backed by libc localtime) --------------
#ifndef MINIBLINK_WIN_COMPAT_SYSTEMTIME
#define MINIBLINK_WIN_COMPAT_SYSTEMTIME
#include <time.h>
typedef struct _SYSTEMTIME {
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;
static inline void GetLocalTime(LPSYSTEMTIME st) {
    if (!st) return;
    time_t t = time(nullptr);
    struct tm lt;
    localtime_r(&t, &lt);
    st->wYear = (WORD)(lt.tm_year + 1900);
    st->wMonth = (WORD)(lt.tm_mon + 1);
    st->wDayOfWeek = (WORD)lt.tm_wday;
    st->wDay = (WORD)lt.tm_mday;
    st->wHour = (WORD)lt.tm_hour;
    st->wMinute = (WORD)lt.tm_min;
    st->wSecond = (WORD)lt.tm_sec;
    st->wMilliseconds = 0;
}
#endif

// --- NPAPI NPEvent (macOS) ---------------------------------------------------
// content/web_impl_win/npapi/WebPluginImpl.h declares dispatchNPEvent(NPEvent&).
// In third_party/npapi/bindings/npapi.h, NPEvent is only typedef'd off the Carbon
// EventRecord on macOS, which is disabled under 64-bit (NP_NO_CARBON), leaving NPEvent
// undefined depending on include order. Define it as void* (npapi's own non-Carbon
// fallback) so wke.cpp resolves the type. An identical `typedef void* NPEvent;`
// redefinition is legal C++, so this is safe even where npapi.h also defines it.
#if !defined(_WIN32)
typedef void* NPEvent;
#endif

// --- HRESULT + common codes --------------------------------------------------
#ifndef MINIBLINK_WIN_COMPAT_HRESULT
#define MINIBLINK_WIN_COMPAT_HRESULT
typedef LONG HRESULT;
#ifndef S_OK
#define S_OK            ((HRESULT)0L)
#define S_FALSE         ((HRESULT)1L)
#define E_INVALIDARG    ((HRESULT)0x80070057L)
#define E_FAIL          ((HRESULT)0x80004005L)
#endif
#endif

// --- BMP (DIB) headers -------------------------------------------------------
#ifndef BI_RGB
#define BI_RGB 0L
#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;
#pragma pack(pop)
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
#endif

// --- More window styles / show commands / syscommands ------------------------
#ifndef WS_CAPTION
#define WS_CAPTION      0x00C00000L
#define WS_SYSMENU      0x00080000L
#define WS_SIZEBOX      0x00040000L
#define WS_THICKFRAME   0x00040000L
#define WS_BORDER       0x00800000L
#define WS_EX_TOOLWINDOW 0x00000080L
#endif
#ifndef SW_HIDE
#define SW_HIDE 0
#define SW_SHOW 5
#endif
#ifndef SC_RESTORE
#define SC_RESTORE  0xF120
#define SC_MAXIMIZE 0xF030
#define SC_SIZE     0xF000
#endif
#ifndef SWP_NOSIZE
#define SWP_NOSIZE   0x0001
#define SWP_NOMOVE   0x0002
#define SWP_NOZORDER 0x0004
#endif
#ifndef HWND_TOPMOST
#define HWND_TOPMOST ((HWND)-1)
#endif
#ifndef MB_OK
#define MB_OK        0x00000000L
#define MB_OKCANCEL  0x00000001L
#define IDOK         1
#define IDCANCEL     2
#endif
#ifndef CFS_POINT
#define CFS_POINT          0x0002
#define CFS_FORCE_POSITION 0x0020
#define GCS_COMPSTR        0x0008
#endif
#ifndef SM_CXSCREEN
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#endif

// --- WNDCLASSEX + CREATESTRUCTW + IME types ----------------------------------
#ifndef MINIBLINK_WIN_COMPAT_WNDCLASSEX
#define MINIBLINK_WIN_COMPAT_WNDCLASSEX
typedef struct tagWNDCLASSEXW {
    UINT      cbSize;
    UINT      style;
    WNDPROC   lpfnWndProc;
    int       cbClsExtra, cbWndExtra;
    HINSTANCE hInstance;
    HICON     hIcon;
    HCURSOR   hCursor;
    HBRUSH    hbrBackground;
    LPCWSTR   lpszMenuName, lpszClassName;
    HICON     hIconSm;
} WNDCLASSEXW, *LPWNDCLASSEXW;
typedef WNDCLASSEXW WNDCLASSEX;
typedef struct tagCREATESTRUCTW {
    LPVOID    lpCreateParams;
    HINSTANCE hInstance;
    HMENU     hMenu;
    HWND      hwndParent;
    int       cy, cx, y, x;
    LONG      style;
    LPCWSTR   lpszName, lpszClass;
    DWORD     dwExStyle;
} CREATESTRUCTW, *LPCREATESTRUCTW;
DECLARE_HANDLE(HIMC);
typedef struct tagCOMPOSITIONFORM {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} COMPOSITIONFORM, *LPCOMPOSITIONFORM;
DECLARE_HANDLE(HDROP);
#endif

// --- Additional inert USER/GDI/file/window function stubs --------------------
#ifndef MINIBLINK_WIN_COMPAT_WKE_STUBS
#define MINIBLINK_WIN_COMPAT_WKE_STUBS
// Window class / creation (inert: Cocoa hosts the real window on macOS).
template<typename T> static inline BOOL GetClassInfoW(HINSTANCE, const T*, void*) { return FALSE; }
static inline short RegisterClassW(const void*) { return 0; }
static inline short RegisterClassExW(const void*) { return 0; }
static inline short RegisterClassEx(const void*) { return 0; }
template<typename T1, typename T2> static inline HWND CreateWindowExW(DWORD, const T1*, const T2*, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void*) { return NULL; }
static inline LRESULT DefWindowProcW(HWND, UINT, WPARAM, LPARAM) { return 0; }
template<typename T> static inline HICON LoadIconW(HINSTANCE, const T*) { return NULL; }
#define LoadIcon LoadIconW
// Window properties (inert).
template<typename T> static inline HANDLE GetPropW(HWND, const T*) { return NULL; }
template<typename T> static inline BOOL SetPropW(HWND, const T*, HANDLE) { return FALSE; }
template<typename T> static inline HANDLE RemovePropW(HWND, const T*) { return NULL; }
// Window geometry / text / timers (inert).
static inline BOOL MoveWindow(HWND, int, int, int, int, BOOL) { return FALSE; }
static inline LONG GetWindowLongW(HWND, int) { return 0; }
static inline LONG SetWindowLongW(HWND, int, LONG) { return 0; }
#define GetWindowLong GetWindowLongW
#define SetWindowLong SetWindowLongW
template<typename T> static inline BOOL SetWindowTextW(HWND, const T*) { return FALSE; }
static inline BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return FALSE; }
static inline int  GetSystemMetrics(int) { return 0; }
static inline UINT_PTR SetTimer(HWND, UINT_PTR id, UINT, void*) { return id; }
static inline BOOL IsZoomed(HWND) { return FALSE; }
template<typename T1, typename T2> static inline int MessageBoxW(HWND, const T1*, const T2*, UINT) { return 0; }
// Paint / GDI (inert: paint goes through Cocoa/Skia on macOS).
typedef void* HGDIOBJ;
static inline HDC  CreateCompatibleDC(HDC) { return NULL; }
static inline HBITMAP CreateCompatibleBitmap(HDC, int, int) { return NULL; }
static inline HGDIOBJ SelectObject(HDC, HGDIOBJ) { return NULL; }
static inline BOOL BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return FALSE; }
static inline BOOL InvalidateRect(HWND, const RECT*, BOOL) { return FALSE; }
static inline HDC  BeginPaint(HWND, PAINTSTRUCT*) { return NULL; }
static inline BOOL EndPaint(HWND, const PAINTSTRUCT*) { return FALSE; }
static inline BOOL IntersectRect(RECT* dst, const RECT*, const RECT*) { if (dst) { dst->left=dst->top=dst->right=dst->bottom=0; } return FALSE; }
// Drag & drop (inert).
static inline void DragAcceptFiles(HWND, BOOL) {}
static inline void DragFinish(HDROP) {}
template<typename T> static inline UINT DragQueryFileW(HDROP, UINT, T*, UINT) { return 0; }
static inline HRESULT RevokeDragDrop(HWND) { return 0; }
// IME (inert).
static inline HIMC ImmGetContext(HWND) { return NULL; }
static inline BOOL ImmReleaseContext(HWND, HIMC) { return FALSE; }
static inline BOOL ImmSetCompositionWindow(HIMC, COMPOSITIONFORM*) { return FALSE; }
static inline LONG ImmGetCompositionStringW(HIMC, DWORD, void*, DWORD) { return 0; }
// File delete (inert: real persistence handled by the Cocoa/net layer on macOS).
template<typename T> static inline BOOL DeleteFileW(const T*) { return FALSE; }
// COM apartment init (no-op on macOS).
static inline HRESULT CoInitialize(void*) { return 0; }
static inline void CoUninitialize(void) {}
// Process termination (used by wke's fatal misuse path; abort mirrors that intent).
static inline BOOL TerminateProcess(HANDLE, UINT code) { abort(); return TRUE; }
// Win32 message-pump primitives (inert: the Cocoa run loop drives events on macOS).
static inline BOOL PeekMessageW(MSG*, HWND, UINT, UINT, UINT) { return FALSE; }
static inline BOOL TranslateMessage(const MSG*) { return FALSE; }
static inline LRESULT DispatchMessageW(const MSG*) { return 0; }
static inline BOOL GetMessageW(MSG*, HWND, UINT, UINT) { return FALSE; }
#define PeekMessage PeekMessageW
#define DispatchMessage DispatchMessageW
#define GetMessage GetMessageW

// --- Thread-local storage (real, pthread-key backed) -------------------------
// A small fixed slot table maps a DWORD index (Win32 TLS index) to a pthread_key_t,
// so the index stays a real 32-bit value (a heap pointer would truncate into DWORD).
#ifndef MINIBLINK_WIN_COMPAT_TLS
#define MINIBLINK_WIN_COMPAT_TLS
#include <pthread.h>
#define MB_TLS_MAX_SLOTS 256
#define MB_TLS_INVALID   ((DWORD)0xFFFFFFFF)
static inline pthread_key_t* mb_tls_slots(void) {
    static pthread_key_t s_slots[MB_TLS_MAX_SLOTS];
    return s_slots;
}
static inline bool* mb_tls_used(void) {
    static bool s_used[MB_TLS_MAX_SLOTS];
    return s_used;
}
static inline DWORD TlsAlloc(void) {
    pthread_key_t* slots = mb_tls_slots();
    bool* used = mb_tls_used();
    for (DWORD i = 0; i < MB_TLS_MAX_SLOTS; ++i) {
        if (!used[i]) {
            if (pthread_key_create(&slots[i], nullptr) != 0)
                return MB_TLS_INVALID;
            used[i] = true;
            return i;
        }
    }
    return MB_TLS_INVALID;
}
static inline BOOL TlsSetValue(DWORD index, LPVOID value) {
    if (index >= MB_TLS_MAX_SLOTS || !mb_tls_used()[index]) return FALSE;
    return pthread_setspecific(mb_tls_slots()[index], value) == 0;
}
static inline LPVOID TlsGetValue(DWORD index) {
    if (index >= MB_TLS_MAX_SLOTS || !mb_tls_used()[index]) return nullptr;
    return pthread_getspecific(mb_tls_slots()[index]);
}
static inline BOOL TlsFree(DWORD index) {
    if (index >= MB_TLS_MAX_SLOTS || !mb_tls_used()[index]) return FALSE;
    BOOL ok = pthread_key_delete(mb_tls_slots()[index]) == 0;
    mb_tls_used()[index] = false;
    return ok;
}
#endif
// File I/O (inert: real file persistence is handled by the Cocoa layer on macOS).
template<typename T> static inline HANDLE CreateFileW(const T*, DWORD, DWORD, void*, DWORD, DWORD, HANDLE) { return INVALID_HANDLE_VALUE; }
static inline BOOL  WriteFile(HANDLE, const void*, DWORD, DWORD* written, void*) { if (written) *written = 0; return FALSE; }
static inline BOOL  ReadFile(HANDLE, void*, DWORD, DWORD* read, void*) { if (read) *read = 0; return FALSE; }
static inline DWORD GetFileSize(HANDLE, DWORD* high) { if (high) *high = 0; return 0; }
// GDI device context (inert: paint goes through Cocoa/Skia on macOS).
static inline HDC  GetDC(HWND) { return NULL; }
static inline int  ReleaseDC(HWND, HDC) { return 0; }
static inline int  GetDeviceCaps(HDC, int) { return 96; }  // default 96 DPI
// Path / module helpers (inert).
template<typename T> static inline DWORD GetModuleFileNameW(HMODULE, T*, DWORD) { return 0; }
template<typename T> static inline BOOL  PathRemoveFileSpecW(T*) { return FALSE; }
template<typename T> static inline DWORD GetCurrentDirectoryW(DWORD, T*) { return 0; }
template<typename T> static inline DWORD GetCurrentDirectory(DWORD n, T* b) { return GetCurrentDirectoryW(n, b); }
#endif

#endif // MINIBLINK_WIN_COMPAT_WINDOWS_H_
