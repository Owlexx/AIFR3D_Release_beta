#include "dawai/engine.hpp"

#include <algorithm>
#include <cmath>

namespace dawai
{

Engine::Engine()
{
    m_catalog.push_back({"demo-track-1", "Demo Track", "media/catalog/demo-track.wav"});
}

const std::vector<TrackRef>& Engine::catalog() const noexcept
{
    return m_catalog;
}

MeterState Engine::analyze(const std::vector<float>& samples) const
{
    if (samples.empty())
    {
        return {};
    }

    double peak = 0.0;
    double sumSquares = 0.0;

    for (float sample : samples)
    {
        const double value = static_cast<double>(sample);
        peak = std::max(peak, std::abs(value));
        sumSquares += value * value;
    }

    MeterState out;
    out.rms = std::sqrt(sumSquares / static_cast<double>(samples.size()));
    out.peak = peak;
    out.crestFactor = out.rms > 0.0 ? (out.peak / out.rms) : 0.0;

    DspFeatureSet userFeatures;
    userFeatures.harshBand = out.peak * 0.71;
    userFeatures.controlBand = out.rms * 0.64;
    userFeatures.airBand = out.peak * 0.43;
    userFeatures.mudBand = out.rms * 0.57;
    userFeatures.bodyBand = out.rms * 0.46;
    userFeatures.presenceBand = out.peak * 0.51;
    userFeatures.crestDb = 20.0 * std::log10(std::max(out.crestFactor, 1.0e-9));
    userFeatures.transientDensity = clamp01(out.peak - out.rms);
    userFeatures.sideEnergy = out.rms * 0.35;
    userFeatures.midEnergy = out.rms * 0.65;

    DspFeatureSet reference = userFeatures;
    reference.harshBand *= 0.96;
    reference.mudBand *= 0.94;
    reference.transientDensity = clamp01(userFeatures.transientDensity * 1.04);

    out.harshness = harshnessIndex(userFeatures);
    out.mud = mudIndex(userFeatures);
    out.transientAlignment = transientMatch(userFeatures, reference);
    out.stereoAlignment = stereoSignatureMatch(userFeatures, reference);
    return out;
}

} // namespace dawai
