#include "dawai/aifr3d_core/feature_ingestion.hpp"

#include "dawai/reference_engine/reference_profiler.hpp"

#include <algorithm>
#include <cmath>

namespace dawai::aifr3d_core
{

FeatureSet FeatureIngestor::ingest(const dawai::metering::MeterSnapshot& mix,
                                   const dawai::reference_engine::ReferenceProfile& reference,
                                   const dawai::reference_engine::ReferenceProfile& targetProfile) const
{
    FeatureSet out;
    out.mixSnapshot = mix;
    out.referenceProfile = reference;
    out.targetProfile = targetProfile;
    out.delta = dawai::reference_engine::computeDelta(mix, reference);
    out.targetDelta = dawai::reference_engine::computeDelta(mix, targetProfile);

    const auto makeDeviation = [&](const std::string& category, double targetSeverity,
                                   double referenceSeverity, double targetDelta,
                                   double referenceDelta)
    {
        out.categoryDeviations.push_back(
            {category, std::abs(targetSeverity), std::abs(referenceSeverity), targetDelta,
             referenceDelta});
    };

    const auto averageBandDelta = [&](const std::vector<double>& values, std::size_t from, std::size_t to)
    {
        if (values.empty())
        {
            return 0.0;
        }
        const auto begin = std::min(from, values.size());
        const auto end = std::min(to, values.size());
        if (begin >= end)
        {
            return 0.0;
        }

        double sum = 0.0;
        for (std::size_t i = begin; i < end; ++i)
        {
            sum += values[i];
        }
        return sum / static_cast<double>(end - begin);
    };

    const auto bands = out.targetDelta.spectrumDeltaDb.size();
    const auto bandRange = [&](std::size_t slot)
    {
        if (bands == 0U)
        {
            return std::pair<std::size_t, std::size_t>{0U, 0U};
        }

        std::size_t start = (bands * slot) / 4U;
        std::size_t end = (bands * (slot + 1U)) / 4U;
        if (end <= start)
        {
            end = std::min<std::size_t>(bands, start + 1U);
        }
        return std::pair<std::size_t, std::size_t>{start, end};
    };

    const auto [lowStart, lowEnd] = bandRange(0);
    const auto [lowMidStart, lowMidEnd] = bandRange(1);
    const auto [midStart, midEnd] = bandRange(2);
    const auto [highStart, highEnd] = bandRange(3);

    const double lowTarget = averageBandDelta(out.targetDelta.spectrumDeltaDb, lowStart, lowEnd);
    const double lowReference = averageBandDelta(out.delta.spectrumDeltaDb, lowStart, lowEnd);
    const double lowMidTarget =
        averageBandDelta(out.targetDelta.spectrumDeltaDb, lowMidStart, lowMidEnd);
    const double lowMidReference =
        averageBandDelta(out.delta.spectrumDeltaDb, lowMidStart, lowMidEnd);
    const double midTarget = averageBandDelta(out.targetDelta.spectrumDeltaDb, midStart, midEnd);
    const double midReference = averageBandDelta(out.delta.spectrumDeltaDb, midStart, midEnd);
    const double highTarget =
        averageBandDelta(out.targetDelta.spectrumDeltaDb, highStart, highEnd);
    const double highReference = averageBandDelta(out.delta.spectrumDeltaDb, highStart, highEnd);

    const double transientTargetDelta =
        mix.dynamics.transientDensity - targetProfile.transientDensity;
    const double transientReferenceDelta =
        mix.dynamics.transientDensity - reference.transientDensity;
    const double widthSeverity =
        std::max({std::abs(out.targetDelta.widthDelta) * 6.0, mix.stereo.phaseRisk * 6.0,
                  (1.0 - mix.stereo.subMonoIntegrity) * 6.0});
    const double referenceWidthSeverity =
        std::max({std::abs(out.delta.widthDelta) * 6.0, mix.stereo.phaseRisk * 6.0,
                  (1.0 - mix.stereo.subMonoIntegrity) * 6.0});
    const double dynamicsSeverity =
        std::max(std::abs(out.targetDelta.crestFactorDelta), std::abs(transientTargetDelta) * 12.0);
    const double referenceDynamicsSeverity = std::max(std::abs(out.delta.crestFactorDelta),
                                                      std::abs(transientReferenceDelta) * 12.0);
    const double loudnessSeverity =
        std::max(std::abs(out.targetDelta.integratedLufsDelta),
                 std::abs(out.targetDelta.truePeakDelta) * 2.0);
    const double referenceLoudnessSeverity =
        std::max(std::abs(out.delta.integratedLufsDelta), std::abs(out.delta.truePeakDelta) * 2.0);

    makeDeviation("Low End", lowTarget, lowReference, lowTarget, lowReference);
    makeDeviation("Low-Mids", lowMidTarget, lowMidReference, lowMidTarget, lowMidReference);
    makeDeviation("Mids/Presence", midTarget, midReference, midTarget, midReference);
    makeDeviation("High End", highTarget, highReference, highTarget, highReference);
    makeDeviation("Loudness/Headroom", loudnessSeverity, referenceLoudnessSeverity,
                  out.targetDelta.integratedLufsDelta, out.delta.integratedLufsDelta);
    makeDeviation("Width/Depth", widthSeverity, referenceWidthSeverity, out.targetDelta.widthDelta,
                  out.delta.widthDelta);
    makeDeviation("Dynamics/Impact", dynamicsSeverity, referenceDynamicsSeverity,
                  out.targetDelta.crestFactorDelta, out.delta.crestFactorDelta);

    return out;
}

} // namespace dawai::aifr3d_core
