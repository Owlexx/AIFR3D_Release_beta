#pragma once

#include "dawai/dsp_math.hpp"

#include <string>
#include <vector>

namespace dawai
{

struct MeterState
{
    double rms = 0.0;
    double peak = 0.0;
    double crestFactor = 0.0;
    double harshness = 0.0;
    double mud = 0.0;
    double transientAlignment = 0.0;
    double stereoAlignment = 0.0;
};

struct TrackRef
{
    std::string id;
    std::string title;
    std::string sourcePath;
};

class Engine
{
public:
    Engine();

    const std::vector<TrackRef>& catalog() const noexcept;
    MeterState analyze(const std::vector<float>& samples) const;

private:
    std::vector<TrackRef> m_catalog;
};

} // namespace dawai
