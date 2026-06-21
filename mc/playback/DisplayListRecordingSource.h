#ifndef DisplayListRecordingSource_h
#define DisplayListRecordingSource_h

// Self-contained header so the mc target can build it without relying on the
// repo-root cc/ copy (which has no includes). Behaviour mirrors the Windows
// build; nothing here is platform specific.
#include "third_party/WebKit/Source/platform/geometry/IntRect.h"
#include "third_party/WebKit/Source/platform/geometry/IntSize.h"
#include "third_party/WebKit/Source/wtf/Vector.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "mc/blink/WebLayerImplClient.h"

namespace cc {

using blink::IntRect;
using blink::IntSize;

class RecordingSourceContainer {
public:
    SkPictureRecorder* recorder;
    IntRect area;
    bool dirty;

    RecordingSourceContainer(const IntRect& area)
        : recorder(0)
        , area(area)
        , dirty(true)
    {}

    ~RecordingSourceContainer();
};

// Loosely models chromium's PicturePile.
class DisplayListRecordingSource {

public:
    DisplayListRecordingSource();

    // Used by WebViewCore
    void invalidate(const IntRect& dirtyRect);
    void applyDirtyRects();
    void setSize(const IntSize& size);

    void updateRecordingSourcesIfNeeded(mc_blink::WebLayerImplClient* client);
    void updateRecordingSources(mc_blink::WebLayerImplClient* client);

private:
    void updateRecordingSource(mc_blink::WebLayerImplClient* client, RecordingSourceContainer& recordingSourceContainer);

    Vector<IntRect> m_dirtyRects;
    blink::IntSize m_size; // Logical size of the layer this recording source covers.
    Vector<RecordingSourceContainer> m_recordingSourcePile;
};

}

#endif // DisplayListRecordingSource_h
