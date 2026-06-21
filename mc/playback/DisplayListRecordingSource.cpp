// Use the self-contained mc copy of the header (the repo-root cc/ copy has no
// includes). Behaviour mirrors the Windows build.
#include "mc/playback/DisplayListRecordingSource.h"

namespace cc {

RecordingSourceContainer::~RecordingSourceContainer()
{
    delete recorder;
    recorder = 0;
}

DisplayListRecordingSource::DisplayListRecordingSource()
{
}

void DisplayListRecordingSource::invalidate(const IntRect& dirtyRect)
{
    IntRect inval = dirtyRect;
    inval.intersect(IntRect(0, 0, m_size.width(), m_size.height()));
    if (inval.isEmpty())
        return;

    // TODO: Support multiple non-intersecting webkit invals
    if (m_dirtyRects.size())
        m_dirtyRects[0].unite(inval);
    else
        m_dirtyRects.append(inval);
}

void DisplayListRecordingSource::setSize(const IntSize& size)
{
    m_size = size;
}

void DisplayListRecordingSource::applyDirtyRects()
{
    // TODO: merge dirty rects into the recording source pile.
    for (size_t i = 0; i < m_recordingSourcePile.size(); ++i) {
        RecordingSourceContainer& recordingSourceContainer = m_recordingSourcePile[i];
        for (size_t j = 0; j < m_dirtyRects.size(); ++j) {
            if (recordingSourceContainer.area.intersects(m_dirtyRects[j]))
                recordingSourceContainer.dirty = true;
        }
    }
}

void DisplayListRecordingSource::updateRecordingSource(mc_blink::WebLayerImplClient* client, RecordingSourceContainer& recordingSourceContainer)
{
}

void DisplayListRecordingSource::updateRecordingSources(mc_blink::WebLayerImplClient* client)
{
    for (size_t i = 0; i < m_recordingSourcePile.size(); i++) {
        RecordingSourceContainer& recordingSourceContainer = m_recordingSourcePile[i];
        if (recordingSourceContainer.dirty)
            updateRecordingSource(client, recordingSourceContainer);
    }
}

void DisplayListRecordingSource::updateRecordingSourcesIfNeeded(mc_blink::WebLayerImplClient* client)
{
    applyDirtyRects();
    updateRecordingSources(client);
}

} // cc
