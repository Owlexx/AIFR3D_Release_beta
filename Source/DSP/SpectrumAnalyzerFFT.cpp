#include "SpectrumAnalyzerFFT.h"

#include <algorithm>
#include <cmath>

namespace audiosynth::dsp
{

namespace
{
constexpr float kMagnitudeFloor = 1.0e-9f;
constexpr float kHannWindowCorrection = 1.5f;

float linearToDb(float value)
{
    return 20.0f * std::log10(std::max(value, kMagnitudeFloor));
}

std::pair<std::size_t, std::size_t> binRangeForHz(double sampleRate, int fftSize, double lowHz,
                                                  double highHz, std::size_t binCount)
{
    const double binWidth = sampleRate / static_cast<double>(fftSize);
    const auto begin = static_cast<std::size_t>(
        std::clamp(std::floor(lowHz / std::max(1.0, binWidth)), 0.0,
                   static_cast<double>(binCount - 1)));
    const auto end = static_cast<std::size_t>(
        std::clamp(std::ceil(highHz / std::max(1.0, binWidth)), static_cast<double>(begin + 1),
                   static_cast<double>(binCount)));
    return {begin, end};
}

float averageMagnitudeDb(const std::vector<float>& magnitudes, double sampleRate, int fftSize,
                         double lowHz, double highHz)
{
    if (magnitudes.empty() || sampleRate <= 0.0 || highHz <= lowHz)
    {
        return -90.0f;
    }

    const auto [begin, end] =
        binRangeForHz(sampleRate, fftSize, lowHz, highHz, magnitudes.size());
    float sum = 0.0f;
    std::size_t count = 0;
    for (std::size_t index = begin; index < end; ++index)
    {
        sum += magnitudes[index];
        ++count;
    }

    return count > 0 ? linearToDb(sum / static_cast<float>(count)) : -90.0f;
}

float regressionSlopeDbPerOct(const std::vector<float>& magnitudes, double sampleRate, int fftSize)
{
    if (magnitudes.empty() || sampleRate <= 0.0)
    {
        return 0.0f;
    }

    const double binWidth = sampleRate / static_cast<double>(fftSize);
    double sumX = 0.0;
    double sumY = 0.0;
    double sumXX = 0.0;
    double sumXY = 0.0;
    int count = 0;

    for (std::size_t index = 1; index < magnitudes.size(); ++index)
    {
        const double frequency = static_cast<double>(index) * binWidth;
        if (frequency < 20.0 || frequency > std::min(20000.0, sampleRate * 0.5))
        {
            continue;
        }

        const double x = std::log2(frequency / 20.0);
        const double y = static_cast<double>(linearToDb(magnitudes[index]));
        sumX += x;
        sumY += y;
        sumXX += x * x;
        sumXY += x * y;
        ++count;
    }

    if (count < 2)
    {
        return 0.0f;
    }

    const double denominator = (static_cast<double>(count) * sumXX) - (sumX * sumX);
    if (std::abs(denominator) <= 1.0e-9)
    {
        return 0.0f;
    }

    return static_cast<float>(((static_cast<double>(count) * sumXY) - (sumX * sumY)) /
                              denominator);
}
} // namespace

void SpectrumAnalyzerFFT::prepare(double sampleRate)
{
    m_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    m_fifo.assign(static_cast<std::size_t>(kFftSize), 0.0f);
    m_window.resize(static_cast<std::size_t>(kFftSize));
    m_timeDomain.assign(static_cast<std::size_t>(kFftSize), 0.0f);
    m_fftData.assign(static_cast<std::size_t>(kFftSize * 2), 0.0f);
    m_fifoWrite = 0;
    m_fifoCount = 0;

    for (int index = 0; index < kFftSize; ++index)
    {
        const float phase =
            juce::MathConstants<float>::twoPi * static_cast<float>(index) /
            static_cast<float>(kFftSize - 1);
        m_window[static_cast<std::size_t>(index)] = 0.5f - 0.5f * std::cos(phase);
    }
}

SpectralFrame SpectrumAnalyzerFFT::analyze(const juce::AudioBuffer<float>& buffer)
{
    SpectralFrame out;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
    {
        return out;
    }

    if (samples >= kFftSize)
    {
        const int startSample = samples - kFftSize;
        for (int index = 0; index < kFftSize; ++index)
        {
            float mono = 0.0f;
            for (int channel = 0; channel < channels; ++channel)
            {
                mono += buffer.getSample(channel, startSample + index);
            }
            mono /= static_cast<float>(channels);
            m_timeDomain[static_cast<std::size_t>(index)] = mono;
            m_fifo[static_cast<std::size_t>(index)] = mono;
        }
        m_fifoWrite = 0;
        m_fifoCount = kFftSize;
    }
    else
    {
        for (int sample = 0; sample < samples; ++sample)
        {
            float mono = 0.0f;
            for (int channel = 0; channel < channels; ++channel)
            {
                mono += buffer.getSample(channel, sample);
            }
            mono /= static_cast<float>(channels);

            m_fifo[static_cast<std::size_t>(m_fifoWrite)] = mono;
            m_fifoWrite = (m_fifoWrite + 1) % kFftSize;
            m_fifoCount = juce::jmin(kFftSize, m_fifoCount + 1);
        }

        std::fill(m_timeDomain.begin(), m_timeDomain.end(), 0.0f);
        const int available = juce::jmin(kFftSize, m_fifoCount);
        const int zeroPad = kFftSize - available;
        for (int index = 0; index < available; ++index)
        {
            const int sourceIndex =
                (m_fifoCount < kFftSize) ? index : ((m_fifoWrite + index) % kFftSize);
            m_timeDomain[static_cast<std::size_t>(zeroPad + index)] =
                m_fifo[static_cast<std::size_t>(sourceIndex)];
        }
    }

    std::fill(m_fftData.begin(), m_fftData.end(), 0.0f);
    for (int index = 0; index < kFftSize; ++index)
    {
        m_fftData[static_cast<std::size_t>(index)] =
            m_timeDomain[static_cast<std::size_t>(index)] *
            m_window[static_cast<std::size_t>(index)];
    }
    m_fft.performRealOnlyForwardTransform(m_fftData.data());

    std::vector<float> correctedMagnitudes(static_cast<std::size_t>(kFftSize / 2),
                                           kMagnitudeFloor);
    for (int bin = 0; bin < kFftSize / 2; ++bin)
    {
        const float real = m_fftData[static_cast<std::size_t>(bin * 2)];
        const float imag = m_fftData[static_cast<std::size_t>(bin * 2 + 1)];
        const float magnitude = std::sqrt(real * real + imag * imag);
        const float magnitudeNormalised =
            magnitude / (static_cast<float>(kFftSize) * 0.5f);
        correctedMagnitudes[static_cast<std::size_t>(bin)] =
            std::max(kMagnitudeFloor, magnitudeNormalised * kHannWindowCorrection);
    }

    out.averagedBinsDb.assign(m_bandCount, -120.0f);
    out.binWidthHz = static_cast<float>(m_sampleRate / static_cast<double>(kFftSize));
    const double maxFrequency = std::min(20000.0, m_sampleRate * 0.5);

    for (std::size_t band = 0; band < m_bandCount; ++band)
    {
        const double t0 = static_cast<double>(band) / static_cast<double>(m_bandCount);
        const double t1 = static_cast<double>(band + 1) / static_cast<double>(m_bandCount);
        const double lowHz = 20.0 * std::pow(maxFrequency / 20.0, t0);
        const double highHz = 20.0 * std::pow(maxFrequency / 20.0, t1);
        out.averagedBinsDb[band] =
            averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, lowHz, highHz);
    }

    out.subBandDb = averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 20.0, 60.0);
    out.lowBandDb = averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 60.0, 120.0);
    out.lowMidBandDb =
        averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 120.0, 400.0);
    out.midBandDb =
        averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 400.0, 2000.0);
    out.highMidBandDb =
        averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 2000.0, 6000.0);
    out.highBandDb =
        averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 6000.0, 12000.0);
    out.presenceBandDb =
        averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 4000.0, 8000.0);
    out.airBandDb =
        averageMagnitudeDb(correctedMagnitudes, m_sampleRate, kFftSize, 12000.0, maxFrequency);
    out.spectralTiltDb = regressionSlopeDbPerOct(correctedMagnitudes, m_sampleRate, kFftSize);
    return out;
}

float SpectrumAnalyzerFFT::spectralTiltDbPerOct(const juce::AudioBuffer<float>& buffer)
{
    return analyze(buffer).spectralTiltDb;
}

} // namespace audiosynth::dsp
