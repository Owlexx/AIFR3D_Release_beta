#pragma once

#include "dawai/metering/metering_engine.hpp"
#include "dawai/reference_engine/reference_profile.hpp"

#include <string>
#include <vector>

namespace dawai::aifr3d_core
{

struct CategoryDeviation
{
    std::string category;
    double deviation = 0.0;
    double referenceDeviation = 0.0;
    double targetDelta = 0.0;
    double referenceDelta = 0.0;
};

struct FeatureSet
{
    dawai::metering::MeterSnapshot mixSnapshot;
    dawai::reference_engine::ReferenceProfile referenceProfile;
    dawai::reference_engine::ReferenceProfile targetProfile;
    dawai::reference_engine::ProfileDelta delta;
    dawai::reference_engine::ProfileDelta targetDelta;
    std::vector<CategoryDeviation> categoryDeviations;
    double targetWindow = 3.0;
};

class FeatureIngestor
{
  public:
    [[nodiscard]] FeatureSet
    ingest(const dawai::metering::MeterSnapshot& mix,
           const dawai::reference_engine::ReferenceProfile& reference,
           const dawai::reference_engine::ReferenceProfile& targetProfile) const;
};

} // namespace dawai::aifr3d_core
