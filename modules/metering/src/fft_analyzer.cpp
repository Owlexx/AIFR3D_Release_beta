#include "dawai/metering/fft_analyzer.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

namespace dawai::metering
{

namespace
{
constexpr double kEpsilon = 1.0e-12;

double averageBandDb(const std::vector<double>& spectrum, double sampleRate, std::size_t fftSize,
                     double hzLow, double hzHigh)
{
    if (spectrum.empty() || sampleRate <= 0.0 || hzHigh <= hzLow)
    {
        return -90.0;
    }

    const double binWidth = sampleRate / static_cast<double>(fftSize);
    const std::size_t begin = std::min<std::size_t>(
        spectrum.size() - 1,
        static_cast<std::size_t>(std::max(0.0, std::floor(hzLow / std::max(binWidth, 1.0)))));
    const std::size_t end = std::min<std::size_t>(
        spectrum.size(),
        static_cast<std::size_t>(std::max<double>(begin + 1, std::ceil(hzHigh / std::max(binWidth, 1.0)))));

    double acc = 0.0;
    std::size_t count = 0;
    for (std::size_t i = begin; i < end; ++i)
    {
        acc += spectrum[i];
        ++count;
    }

    return (count == 0) ? -90.0 : (acc / static_cast<double>(count));
}

std::vector<double> buildMonoWindow(const AudioBlock& block, std::size_t fftSize)
{
    std::vector<double> mono(fftSize, 0.0);
    if (block.numChannels() == 0 || block.numSamples() == 0)
    {
        return mono;
    }

    const std::size_t samples = std::min(block.numSamples(), fftSize);
    for (std::size_t i = 0; i < samples; ++i)
    {
        double mix = 0.0;
        for (std::size_t c = 0; c < block.numChannels(); ++c)
        {
            mix += static_cast<double>(block.channels[c][i]);
        }
        mix /= static_cast<double>(block.numChannels());

        const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) /
                                                   static_cast<double>(fftSize - 1));
        mono[i] = mix * window;
    }
    return mono;
}

std::vector<double> computeMagnitudeSpectrum(const std::vector<double>& mono)
{
    const std::size_t n = mono.size();
    const std::size_t half = n / 2;
    std::vector<double> magnitudes(half, -120.0);

    for (std::size_t k = 0; k < half; ++k)
    {
        std::complex<double> sum(0.0, 0.0);
        for (std::size_t i = 0; i < n; ++i)
        {
            const double phase =
                -2.0 * std::numbers::pi * static_cast<double>(k * i) / static_cast<double>(n);
            sum += mono[i] * std::complex<double>(std::cos(phase), std::sin(phase));
        }
        const double magnitude = std::abs(sum) / static_cast<double>(n);
        magnitudes[k] = 20.0 * std::log10(std::max(magnitude, kEpsilon));
    }

    return magnitudes;
}

} // namespace

FFTAnalyzer::FFTAnalyzer(std::size_t fftSize, std::size_t bandCount)
    : m_fftSize(std::max<std::size_t>(fftSize, 64)),
      m_bandCount(std::max<std::size_t>(bandCount, 8))
{
}

SpectrumMetrics FFTAnalyzer::analyze(const AudioBlock& block, double sampleRate) const
{
    const auto mono = buildMonoWindow(block, m_fftSize);
    const auto spectrum = computeMagnitudeSpectrum(mono);

    SpectrumMetrics metrics;
    metrics.averagedBinsDb.assign(m_bandCount, -120.0);
    metrics.binWidthHz = sampleRate / static_cast<double>(m_fftSize);

    const std::size_t binsPerBand = std::max<std::size_t>(1, spectrum.size() / m_bandCount);
    for (std::size_t band = 0; band < m_bandCount; ++band)
    {
        const std::size_t begin = band * binsPerBand;
        const std::size_t end = std::min<std::size_t>(spectrum.size(), begin + binsPerBand);
        if (begin >= end)
        {
            continue;
        }

        double acc = 0.0;
        for (std::size_t i = begin; i < end; ++i)
        {
            acc += spectrum[i];
        }
        metrics.averagedBinsDb[band] = acc / static_cast<double>(end - begin);
    }

    metrics.lowBandDb = averageBandDb(spectrum, sampleRate, m_fftSize, 20.0, 120.0);
    metrics.lowMidBandDb = averageBandDb(spectrum, sampleRate, m_fftSize, 120.0, 500.0);
    metrics.presenceBandDb = averageBandDb(spectrum, sampleRate, m_fftSize, 2000.0, 5000.0);
    metrics.airBandDb = averageBandDb(spectrum, sampleRate, m_fftSize, 8000.0, 16000.0);

    const double lowRef = std::max(-90.0, metrics.lowBandDb + 0.5 * metrics.lowMidBandDb);
    const double highRef = std::max(-90.0, metrics.presenceBandDb + 0.5 * metrics.airBandDb);
    metrics.spectralTiltDb = highRef - lowRef;

    return metrics;
}

} // namespace dawai::metering
