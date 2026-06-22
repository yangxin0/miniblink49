// mb_dll_main.cpp — translation unit for the miniblink Windows SHARED/STATIC
// targets. The exported surface is the wke C API (wke/wke.h, marked
// __declspec(dllexport) via BUILDING_wke), pulled in whole-archive at link time.
// content/web_impl_win already provides DllMain (BlinkPlatformImpl.cpp), so this
// file deliberately contains no entry point — it only gives the target a TU.
#if defined(_WIN32)
namespace { volatile int mb_dll_anchor = 0; }
#endif
