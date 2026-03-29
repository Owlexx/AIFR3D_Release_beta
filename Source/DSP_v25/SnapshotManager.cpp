#include "SnapshotManager.h"

namespace audiosynth::dsp
{

void SnapshotManager::push(const AnalysisFrame& frame)
{
    if (m_frames.size() >= kMaxSnapshots)
    {
        m_frames.erase(m_frames.begin());
    }
    m_frames.push_back(frame);
}

void SnapshotManager::clear()
{
    m_frames.clear();
}

} // namespace audiosynth::dsp
