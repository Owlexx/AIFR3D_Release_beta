#pragma once

#include <vector>

namespace dawai
{

struct DspFeatureSet
{
    double harshBand = 0.0;
    double controlBand = 0.0;
    double airBand = 0.0;
    double mudBand = 0.0;
    double bodyBand = 0.0;
    double presenceBand = 0.0;
    double crestDb = 0.0;
    double transientDensity = 0.0;
    double sideEnergy = 0.0;
    double midEnergy = 0.0;
};

double clamp01(double value);
double logistic(double value);
double robustZ(double x, const std::vector<double>& pool);

double harshnessIndex(const DspFeatureSet& f);
double mudIndex(const DspFeatureSet& f);
double transientMatch(const DspFeatureSet& user, const DspFeatureSet& reference);
double stereoSignatureMatch(const DspFeatureSet& user, const DspFeatureSet& reference);

} // namespace dawai
