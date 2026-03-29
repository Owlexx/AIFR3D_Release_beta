#include "SpectrumAnalyzerFFT.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr double kSpectrumFloor = 1.0e-12;

int fftOrderForSampleCount(int samples)
{
    const int limitedSamples = juce::jlimit(256, 2048, samples);
    int order = 8;
    while ((1 << (order + 1)) <= limitedSamples && order < 11)
    {
        ++order;
    }
    return order;
}

std::vector<double> computePowerSpectrum(const juce::AudioBuffer<float>& buffer, int fftSize)
{
    std::vector<double> powerSpectrum(static_cast<std::size_t>(fftSize / 2), kSpectrumFloor);
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples < fftSize)
    {
        return powerSpectrum;
    }

    juce::dsp::FFT fft(fftOrderForSampleCount(fftSize));
    std::vector<float> fftData(static_cast<std::size_t>(fftSize * 2), 0.0f);
    const int startSample = samples - fftSize;
    const float norm = 1.0f / static_cast<float>(juce::jmax(1, channels));

    for (int i = 0; i < fftSize; ++i)
    {
        float sample = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            sample += buffer.getSample(channel, startSample + i);
        }
        sample *= norm;

        const float phase =
            juce::MathConstants<float>::twoPi * static_cast<float>(i) /
            static_cast<float>(juce::jmax(1, fftSize - 1));
        const float window = 0.5f - 0.5f * std::cos(phase);
        fftData[static_cast<std::size_t>(i)] = sample * window;
    }

    fft.performRealOnlyForwardTransform(fftData.data());

    for (int bin = 0; bin < fftSize / 2; ++bin)
    {
        const float real = fftData[static_cast<std::size_t>(bin * 2)];
        const float imag = fftData[static_cast<std::size_t>(bin * 2 + 1)];
        const double magnitudeSq = static_cast<double>(real * real + imag * imag);
        powerSpectrum[static_cast<std::size_t>(bin)] =
            std::max(kSpectrumFloor, magnitudeSq / static_cast<double>(fftSize * fftSize));
    }

    return powerSpectrum;
}

float averageBandDb(const std::vector<double>& powerSpectrum, double sampleRate, int fftSize,
                    double hzLow, double hzHigh)
{
    if (powerSpectrum.empty() || sampleRate <= 0.0 || hzHigh <= hzLow)
    {
        return -90.0f;
    }

    const double binWidth = sampleRate / static_cast<double>(fftSize);
    const auto binIndex = [&](double hz)
    {
        return static_cast<std::size_t>(std::floor(hz / std::max(1.0, binWidth)));
    };

    const std::size_t begin =
        std::min<std::size_t>(powerSpectrum.size() - 1, binIndex(hzLow));
    const std::size_t end =
        std::min<std::size_t>(powerSpectrum.size(),
                              std::max<std::size_t>(begin + 1, binIndex(hzHigh) + 1));

    double sum = 0.0;
    std::size_t count = 0;
    for (std::size_t index = begin; index < end; ++index)
    {
        sum += powerSpectrum[index];
        ++count;
    }

    if (count == 0)
    {
        return -90.0f;
    }

    return static_cast<float>(10.0 * std::log10(std::max(kSpectrumFloor, sum / count)));
}

std::vector<float> averageSpectrumBands(const std::vector<double>& powerSpectrum,
                                        std::size_t bandCount)
{
    std::vector<float> averaged(std::max<std::size_t>(bandCount, 1U), -120.0f);
    if (powerSpectrum.empty())
    {
        return averaged;
    }

    const std::size_t binsPerBand = std::max<std::size_t>(1, powerSpectrum.size() / averaged.size());
    for (std::size_t band = 0; band < averaged.size(); ++band)
    {
        const std::size_t begin = band * binsPerBand;
        const std::size_t end = std::min<std::size_t>(powerSpectrum.size(), begin + binsPerBand);
        if (begin >= end)
        {
            continue;
        }

        double sum = 0.0;
        for (std::size_t index = begin; index < end; ++index)
        {
            sum += powerSpectrum[index];
        }
        averaged[band] = static_cast<float>(
            10.0 * std::log10(std::max(kSpectrumFloor, sum / static_cast<double>(end - begin))));
    }

    return averaged;
}
} // namespace

namespace aifred::dsp
{

void SpectrumAnalyzerFFT::prepare(double sampleRate)
{
    m_sampleRate = sampleRate;
}

SpectralFrame SpectrumAnalyzerFFT::analyze(const juce::AudioBuffer<float>& buffer) const
{
    SpectralFrame out;

    const int samples = buffer.getNumSamples();
    if (samples < 256)
    {
        return out;
    }

    const int fftOrder = fftOrderForSampleCount(samples);
    const int fftSize = 1 << fftOrder;
    const auto powerSpectrum = computePowerSpectrum(buffer, fftSize);

    out.averagedBinsDb = averageSpectrumBands(powerSpectrum, m_bandCount);
    out.binWidthHz = static_cast<float>(m_sampleRate / static_cast<double>(fftSize));
    out.subBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 20.0, 60.0);
    out.lowBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 60.0, 200.0);
    out.lowMidBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 200.0, 800.0);
    out.midBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 800.0, 2000.0);
    out.highMidBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 2000.0, 6000.0);
    out.highBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 6000.0, 16000.0);
    out.presenceBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 4000.0, 8000.0);
    out.airBandDb = averageBandDb(powerSpectrum, m_sampleRate, fftSize, 8000.0, 16000.0);

    const double lowRef =
        (static_cast<double>(out.subBandDb) + static_cast<double>(out.lowBandDb) +
         static_cast<double>(out.lowMidBandDb)) /
        3.0;
    const double highRef =
        (static_cast<double>(out.highMidBandDb) + static_cast<double>(out.highBandDb)) / 2.0;
    out.spectralTiltDb = static_cast<float>(highRef - lowRef);
    return out;
}

float SpectrumAnalyzerFFT::spectralTiltDbPerOct(const juce::AudioBuffer<float>& buffer) const
{
    return analyze(buffer).spectralTiltDb;
}

} // namespace aifred::dsp
