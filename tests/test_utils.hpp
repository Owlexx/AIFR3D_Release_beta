#pragma once

#include "dawai/metering/metrics.hpp"

#include <cmath>
#include <numbers>
#include <random>

namespace test_utils
{

inline dawai::metering::AudioBlock makeSine(double frequency, double sampleRate,
                                            std::size_t samples, float amplitude = 0.5F)
{
    dawai::metering::AudioBlock block;
    block.channels.resize(2);
    block.channels[0].resize(samples, 0.0F);
    block.channels[1].resize(samples, 0.0F);

    for (std::size_t i = 0; i < samples; ++i)
    {
        const auto value =
            static_cast<float>(amplitude * std::sin(2.0 * std::numbers::pi * frequency *
                                                    static_cast<double>(i) / sampleRate));
        block.channels[0][i] = value;
        block.channels[1][i] = value;
    }

    return block;
}

inline dawai::metering::AudioBlock makeNoise(std::size_t samples, float amplitude = 0.3F)
{
    dawai::metering::AudioBlock block;
    block.channels.resize(2);
    block.channels[0].resize(samples, 0.0F);
    block.channels[1].resize(samples, 0.0F);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-amplitude, amplitude);

    for (std::size_t i = 0; i < samples; ++i)
    {
        block.channels[0][i] = dist(rng);
        block.channels[1][i] = dist(rng);
    }

    return block;
}

} // namespace test_utils
