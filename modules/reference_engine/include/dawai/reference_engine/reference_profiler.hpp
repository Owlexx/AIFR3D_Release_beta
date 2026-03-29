#pragma once

#include "dawai/metering/metering_engine.hpp"
#include "dawai/reference_engine/reference_profile.hpp"

#include <string>
#include <vector>

namespace dawai::reference_engine
{

class ReferenceProfiler
{
  public:
    ReferenceProfiler(std::size_t fftSize = 1024, std::size_t bands = 32);

    [[nodiscard]] ReferenceProfile buildProfile(const std::string& id, const std::string& title,
                                                const std::string& sourcePath,
                                                const dawai::metering::AudioBlock& block,
                                                double sampleRate);

  private:
    dawai::metering::MeteringEngine m_meteringEngine;
};

ProfileDelta computeDelta(const dawai::metering::MeterSnapshot& mix,
                          const ReferenceProfile& referenceProfile);

} // namespace dawai::reference_engine
