#pragma once

#include "../Common/AnalysisTypes.h"

#include <vector>

namespace audiosynth::dsp
{

class SnapshotManager
{
  public:
    void push(const AnalysisFrame& frame);
    const std::vector<AnalysisFrame>& all() const noexcept
    {
        return m_frames;
    }
    void clear();

  private:
    std::vector<AnalysisFrame> m_frames;
    static constexpr std::size_t kMaxSnapshots = 600;
};

} // namespace audiosynth::dsp
