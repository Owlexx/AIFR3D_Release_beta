#include "dawai/dsp_math.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace dawai
{

namespace
{

constexpr double kEps = 1.0e-9;

double median(std::vector<double> values)
{
    if (values.empty())
    {
        return 0.0;
    }

    const std::size_t mid = values.size() / 2;
    std::nth_element(values.begin(), values.begin() + static_cast<long>(mid), values.end());
    const double m = values[mid];

    if (values.size() % 2 == 0)
    {
        std::nth_element(values.begin(), values.begin() + static_cast<long>(mid - 1), values.end());
        return 0.5 * (m + values[mid - 1]);
    }

    return m;
}

} // namespace

double clamp01(double value)
{
    return std::max(0.0, std::min(1.0, value));
}

double logistic(double value)
{
    return 1.0 / (1.0 + std::exp(-value));
}

double robustZ(double x, const std::vector<double>& pool)
{
    if (pool.empty())
    {
        return 0.0;
    }

    const double m = median(pool);
    std::vector<double> absDev;
    absDev.reserve(pool.size());

    for (double v : pool)
    {
        absDev.push_back(std::abs(v - m));
    }

    const double mad = median(absDev);
    return (x - m) / (1.4826 * mad + kEps);
}

double harshnessIndex(const DspFeatureSet& f)
{
    const double r = f.harshBand - f.controlBand;
    const double airExempt = std::max(0.0, f.airBand - f.controlBand);
    const double value = (0.65 * r) - (0.35 * airExempt);
    return clamp01(logistic(value));
}

double mudIndex(const DspFeatureSet& f)
{
    const double r1 = f.mudBand - f.bodyBand;
    const double r2 = f.mudBand - f.presenceBand;
    const double steadiness = std::max(0.0, 1.0 - std::abs(r1 - r2));
    const double value = (0.45 * r1) + (0.45 * r2) + (0.10 * steadiness);
    return clamp01(logistic(value));
}

double transientMatch(const DspFeatureSet& user, const DspFeatureSet& reference)
{
    const double dCrest = user.crestDb - reference.crestDb;
    const double dDensity = user.transientDensity - reference.transientDensity;
    const double distance = std::sqrt((dCrest * dCrest) + (dDensity * dDensity));
    return clamp01(std::exp(-0.45 * distance));
}

double stereoSignatureMatch(const DspFeatureSet& user, const DspFeatureSet& reference)
{
    const double userRatio = user.sideEnergy / (user.sideEnergy + user.midEnergy + kEps);
    const double refRatio = reference.sideEnergy / (reference.sideEnergy + reference.midEnergy + kEps);
    const double distance = std::abs(userRatio - refRatio);
    return clamp01(1.0 - distance);
}

} // namespace dawai
