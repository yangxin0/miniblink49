// OrigChrome-mode link stubs (macOS lightweight-compositor build).
//
// miniblink has an optional "OrigChrome mode" that drives a full Chromium cc/
// media/gpu compositor on dedicated threads (content::OrigChromeMgr +
// content::LayerTreeWrap, in orig_chrome/content/). It is activated only via
// wkeSetDebugConfig("initOrigChromeUiThread"); the default browser uses the
// lightweight mc/ compositor (content::WebPageImpl guards every LayerTreeWrap use
// behind `if (OrigChromeMgr::getInst())`, which is null here).
//
// Building the real orig_chrome/content archive pulls the entire Chromium cc +
// media + gpu stack (hundreds of symbols). Since the default macOS browser never
// enters OrigChrome mode, we provide link-only no-op stubs instead: getInst()
// returns null (so LayerTreeWrap is never constructed) and the factory methods
// return null. To enable real OrigChrome mode, link orig_chrome_content.cmake
// instead of this file. Windows is unaffected (it builds the real archive).
#include "orig_chrome/content/OrigChromeMgr.h"
#include "orig_chrome/content/LayerTreeWrap.h"

namespace content {

// --- OrigChromeMgr (never instantiated in the default lightweight path) -------
OrigChromeMgr* OrigChromeMgr::getInst() { return nullptr; }
void OrigChromeMgr::init() {}
void OrigChromeMgr::shutdown() {}
void OrigChromeMgr::runUntilIdle() {}
void OrigChromeMgr::runUntilIdleWithoutMsgPeek() {}
void OrigChromeMgr::postBlinkTask(OrigTaskType) {}
void OrigChromeMgr::postUiTask(OrigTaskType) {}
void OrigChromeMgr::postWebTask(const blink::WebTraceLocation&, blink::WebThread::Task*) {}
void OrigChromeMgr::postWebDelayedTask(const blink::WebTraceLocation&, blink::WebThread::Task*, long long) {}
void OrigChromeMgr::addTaskObserver(blink::WebThread::TaskObserver*) {}
void OrigChromeMgr::removeTaskObserver(blink::WebThread::TaskObserver*) {}
blink::WebGraphicsContext3D* OrigChromeMgr::createOffscreenGraphicsContext3D(
    const blink::WebGraphicsContext3D::Attributes&, blink::WebGraphicsContext3D*, blink::WebGLInfo*) { return nullptr; }
blink::WebAudioDevice* OrigChromeMgr::createAudioDevice(
    size_t, unsigned, unsigned, double, blink::WebAudioDevice::RenderCallback*, const blink::WebString&) { return nullptr; }
blink::WebCompositorSupport* OrigChromeMgr::createWebCompositorSupport() { return nullptr; }
blink::WebMediaPlayer* OrigChromeMgr::createWebMediaPlayer(
    blink::WebLocalFrame*, const blink::WebURL&, blink::WebMediaPlayerClient*) { return nullptr; }

// OrigChrome thread bring-up: only invoked from wkeSetDebugConfig(
// "initOrigChromeUiThread"/"initOrigChromeBlinkThread"). In the default
// lightweight path these are never reached (getInst() is null), so no-op stubs
// suffice to satisfy the link. Mirrors the rest of this file.
void OrigChromeMgr::initUiThread() {}
void OrigChromeMgr::initBlinkThread() {}
void OrigChromeMgr::setGLImplType(GLImplType) {}

// Heartbeat callback the UI thread fires each spin. Declared (typedef + extern)
// inline in wke/wke.cpp and assigned via wkeSetUiThreadHeartbeatCallback; with
// no OrigChrome UI thread running on macOS it is only ever stored, never fired.
// Define it here so the assignment in wke.cpp links.
typedef void(WINAPI* PfnUiThreadHeartbeatCallback)();
PfnUiThreadHeartbeatCallback g_uiThreadHeartbeatCallback = nullptr;

// --- LayerTreeWrap (constructed only when getInst() != null, i.e. never here) --
LayerTreeWrap::LayerTreeWrap(WebPageOcBridge*, bool) {}
LayerTreeWrap::~LayerTreeWrap() {}
void LayerTreeWrap::setHWND(HWND) {}
void LayerTreeWrap::onHostResized(int, int) {}
void LayerTreeWrap::firePaintEvent(HDC, const RECT&) {}
HDC LayerTreeWrap::getHdcLocked() { return nullptr; }
void LayerTreeWrap::releaseHdc() {}
void LayerTreeWrap::initializeLayerTreeView() {}
blink::WebLayerTreeView* LayerTreeWrap::layerTreeView() { return nullptr; }

}  // namespace content
