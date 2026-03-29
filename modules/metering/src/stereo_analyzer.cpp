#include "dawai/metering/stereo_analyzer.hpp"

#include <algorithm>
#include <cmath>

namespace dawai::metering
{

namespace
{
constexpr double kEpsilon = 1.0e-12;
}

StereoMetrics StereoAnalyzer::analyze(const AudioBlock& block) const
{
    StereoMetrics out;
    if (block.numChannels() < 2 || block.numSamples() == 0)
    {
        return out;
    }

    const auto& left = block.channels[0];
    const auto& right = block.channels[1];
    const std::size_t n = std::min(left.size(), right.size());

    if (n == 0)
    {
        return out;
    }

    double dot = 0.0;
    double ll = 0.0;
    double rr = 0.0;

    double midEnergy = 0.0;
    double sideEnergy = 0.0;

    for (std::size_t i = 0; i < n; ++i)
    {
        const double l = static_cast<double>(left[i]);
        const double r = static_cast<double>(right[i]);

        dot += l * r;
        ll += l * l;
        rr += r * r;

        const double mid = 0.5 * (l + r);
        const double side = 0.5 * (l - r);
        midEnergy += mid * mid;
        sideEnergy += side * side;
    }

    out.correlation = dot / (std::sqrt(ll * rr) + kEpsilon);
    out.correlation = std::clamp(out.correlation, -1.0, 1.0);

    out.midEnergy = midEnergy / static_cast<double>(n);
    out.sideEnergy = sideEnergy / static_cast<double>(n);
    out.width = sideEnergy / (midEnergy + kEpsilon);
    out.width = std::clamp(out.width, 0.0, 2.0);
    out.phaseRisk = std::clamp((0.35 - out.correlation) / 1.35, 0.0, 1.0);
    out.subMonoIntegrity = std::clamp(1.0 - std::min(out.width, 1.0) * out.phaseRisk, 0.0, 1.0);
    return out;
}

} // namespace dawai::metering
