// MbWinOptionalStubs.cpp — Windows link-only stubs for optional subsystems the
// content layer references but that this CMake build does not compile:
//   * OrigChromeMgr / LayerTreeWrap — the heavyweight "OrigChrome mode" compositor
//     (orig_chrome/content/). The default browser uses the lightweight mc/
//     compositor; getInst() is null so LayerTreeWrap is never constructed.
//   * gfx::win::InitDeviceScaleFactor — DPI init (no-op here).
// (macOS provides the equivalents in OrigChromeStubs.cpp + PluginAndMiscStubsMac.)
//
// NOTE: g_uiThreadHeartbeatCallback is intentionally NOT defined here — on Windows
// content/web_impl_win/WebThreadImpl.cpp already provides it.
#if defined(_WIN32)

#include "orig_chrome/content/OrigChromeMgr.h"
#include "orig_chrome/content/LayerTreeWrap.h"

namespace content {

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
void OrigChromeMgr::initUiThread() {}
void OrigChromeMgr::initBlinkThread() {}
void OrigChromeMgr::setGLImplType(GLImplType) {}

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

namespace gfx { namespace win {
void InitDeviceScaleFactor() {}
} }  // namespace gfx::win

#endif  // defined(_WIN32)
