// PluginAndMiscStubsMac.cpp — link-only stubs for deferred/Windows-only paths
// on the macOS port. Each stub mirrors a Windows implementation that is either
// (a) NPAPI plugin support (deferred on macOS — content::createPlugin already
// returns null), or (b) a Win32-only helper. All are version-guarded so the
// Windows build is byte-for-byte unaffected.

#if !defined(_WIN32)

#include "content/web_impl_win/npapi/PluginDatabase.h"
#include "content/web_impl_win/npapi/PluginPackage.h"
#include "third_party/npapi/bindings/npapi.h"
#include "third_party/npapi/bindings/npfunctions.h"

#include <new>
#include <type_traits>

// ---------------------------------------------------------------------------
// NPAPI plugin database / package — plugins are not supported on macOS yet.
// createPlugin() returns null elsewhere, so these never feed a live plugin.
// ---------------------------------------------------------------------------
namespace content {

// installedPlugins() is dereferenced by wkeAddPluginDirectory (which then calls
// addExtraPluginDirectory). Return a pointer to a never-touched static storage
// blob cast to PluginDatabase so the deref is valid; addExtraPluginDirectory is
// a no-op that ignores `this`, so no member is ever read or written.
PluginDatabase* PluginDatabase::installedPlugins(bool /*populate*/)
{
    alignas(PluginDatabase) static unsigned char s_storage[sizeof(PluginDatabase)] = { 0 };
    return reinterpret_cast<PluginDatabase*>(s_storage);
}

void PluginDatabase::addExtraPluginDirectory(const WTF::String&)
{
    // No plugin directories are scanned on macOS.
}

PluginPackage::~PluginPackage()
{
    // Never constructed on macOS (createVirtualPackage returns null); defined
    // only to satisfy the vtable/link.
}

PassRefPtr<PluginPackage> PluginPackage::createVirtualPackage(
    NP_InitializeFuncPtr /*NP_Initialize*/,
    NP_GetEntryPointsFuncPtr /*NP_GetEntryPoints*/,
    NPP_ShutdownProcPtr /*NPP_Shutdown*/)
{
    // No NPAPI plugins on macOS.
    return nullptr;
}

} // namespace content

// ---------------------------------------------------------------------------
// s_wkeBrowserFuncs — the NPAPI browser-side function table, defined in the
// (un-built) npapi/npapi.cpp on Windows. Provide the same zero-initialized
// global so WebMediaPlayerImpl's extern reference links.
// ---------------------------------------------------------------------------
#if ENABLE_WKE
NPNetscapeFuncs s_wkeBrowserFuncs = { 0 };
#endif

// ---------------------------------------------------------------------------
// gfx::win::InitDeviceScaleFactor — Windows DPI initialization. On macOS the
// scale factor comes from the backing NSScreen later; no-op for now.
// ---------------------------------------------------------------------------
namespace gfx {
namespace win {
void InitDeviceScaleFactor()
{
}
} // namespace win
} // namespace gfx

// ---------------------------------------------------------------------------
// InternetSetCookieA — WinINet cookie setter, called from blink's NPV8Object
// cookie path (and the excluded w3client.cpp). No WinINet on macOS; cookies are
// handled by the curl cookie jar, so this is a no-op returning TRUE. Signature
// matches the extern "C" declaration in NPV8Object.cpp (win_compat types come
// from the force-included win_compat/windows.h).
// ---------------------------------------------------------------------------
extern "C" BOOL WINAPI InternetSetCookieA(LPCSTR /*lpszUrl*/, LPCSTR /*lpszCookieName*/, LPCSTR /*lpszCookieData*/)
{
    return TRUE;
}

#endif // !defined(_WIN32)
