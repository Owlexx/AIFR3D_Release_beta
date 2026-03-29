#include "PluginProcessor.h"

#include "../DSP/AnalysisEngine.h"
#include "PluginEditor.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

DawAiAudioProcessor::DawAiAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("In", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Out", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AIFRED_PARAMS", aifred::parameters::createLayout())
{
    analysis = std::make_unique<aifred::dsp::AnalysisEngine>();
    analysisWorker = std::thread([this] { analysisLoop(); });
}

DawAiAudioProcessor::~DawAiAudioProcessor()
{
    analysisStopping.store(true, std::memory_order_relaxed);
    if (analysisWorker.joinable())
    {
        analysisWorker.join();
    }
}

void DawAiAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    runtimeSampleRate.store(sampleRate > 0.0 ? sampleRate : 48000.0, std::memory_order_relaxed);
    runtimeBlockSize.store(samplesPerBlock > 0 ? samplesPerBlock : 512, std::memory_order_relaxed);
    analysisNeedsPrepare.store(true, std::memory_order_relaxed);
}

void DawAiAudioProcessor::releaseResources() {}

bool DawAiAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet() &&
           (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono() ||
            layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void DawAiAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto sr = getSampleRate();
    const auto sampleRate = (sr > 0.0 ? sr : runtimeSampleRate.load(std::memory_order_relaxed));
    const auto frameIndex = frameCounter.fetch_add(1, std::memory_order_relaxed);
    const auto tSec = static_cast<double>(frameIndex) * static_cast<double>(buffer.getNumSamples()) /
                      sampleRate;

    AnalysisTapBuffer tap;
    tap.numChannels = juce::jlimit(0, AnalysisTapBuffer::kMaxChannels, buffer.getNumChannels());
    tap.numSamples = juce::jlimit(0, AnalysisTapBuffer::kMaxSamples, buffer.getNumSamples());
    tap.sampleRate = sampleRate;
    tap.tSec = tSec;

    for (int channel = 0; channel < tap.numChannels; ++channel)
    {
        const float* src = buffer.getReadPointer(channel);
        for (int i = 0; i < tap.numSamples; ++i)
        {
            tap.data[static_cast<std::size_t>(channel)][static_cast<std::size_t>(i)] = src[i];
        }
    }

    (void)analysisTapFifo.push(tap);
}

juce::AudioProcessorEditor* DawAiAudioProcessor::createEditor()
{
    return new DawAiAudioProcessorEditor(*this);
}

void DawAiAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void DawAiAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, size));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DawAiAudioProcessor();
}

void DawAiAudioProcessor::analysisLoop()
{
    constexpr int kBurstBufferSamples = 8192;
    constexpr auto kIdleSleep = std::chrono::milliseconds(3);
    juce::AudioBuffer<float> burstScratch(AnalysisTapBuffer::kMaxChannels, kBurstBufferSamples);
    int burstSamples = 0;
    int burstChannels = 0;
    double burstSampleRate = runtimeSampleRate.load(std::memory_order_relaxed);

    while (!analysisStopping.load(std::memory_order_relaxed))
    {
        if (analysisNeedsPrepare.exchange(false, std::memory_order_relaxed))
        {
            analysis->prepare(runtimeSampleRate.load(std::memory_order_relaxed),
                              runtimeBlockSize.load(std::memory_order_relaxed));
        }

        AnalysisTapBuffer tap;
        if (!analysisTapFifo.pop(tap))
        {
            std::this_thread::sleep_for(kIdleSleep);
            continue;
        }

        burstChannels = juce::jmax(1, tap.numChannels);
        burstSampleRate = tap.sampleRate > 0.0 ? tap.sampleRate : burstSampleRate;

        if (tap.numSamples >= kBurstBufferSamples)
        {
            const int srcStart = tap.numSamples - kBurstBufferSamples;
            for (int channel = 0; channel < burstChannels; ++channel)
            {
                std::memcpy(burstScratch.getWritePointer(channel),
                            tap.data[static_cast<std::size_t>(channel)].data() + srcStart,
                            static_cast<std::size_t>(kBurstBufferSamples) * sizeof(float));
            }
            burstSamples = kBurstBufferSamples;
        }
        else
        {
            const int overflow = juce::jmax(0, (burstSamples + tap.numSamples) - kBurstBufferSamples);
            if (overflow > 0)
            {
                const int keepSamples = burstSamples - overflow;
                for (int channel = 0; channel < burstChannels; ++channel)
                {
                    auto* dst = burstScratch.getWritePointer(channel);
                    std::memmove(dst, dst + overflow,
                                 static_cast<std::size_t>(keepSamples) * sizeof(float));
                }
                burstSamples = keepSamples;
            }

            for (int channel = 0; channel < burstChannels; ++channel)
            {
                std::memcpy(burstScratch.getWritePointer(channel, burstSamples),
                            tap.data[static_cast<std::size_t>(channel)].data(),
                            static_cast<std::size_t>(tap.numSamples) * sizeof(float));
            }
            burstSamples += tap.numSamples;
        }

        const int burstWindowSamples =
            juce::jlimit(2048, kBurstBufferSamples,
                         static_cast<int>(std::round(burstSampleRate * 0.08)));
        const int burstHopSamples =
            juce::jlimit(512, burstWindowSamples,
                         static_cast<int>(std::round(burstSampleRate * 0.020)));

        if (burstSamples < burstWindowSamples)
        {
            continue;
        }

        if (burstSamples > burstWindowSamples)
        {
            const int trim = burstSamples - burstWindowSamples;
            for (int channel = 0; channel < burstChannels; ++channel)
            {
                auto* dst = burstScratch.getWritePointer(channel);
                std::memmove(dst, dst + trim,
                             static_cast<std::size_t>(burstWindowSamples) * sizeof(float));
            }
            burstSamples = burstWindowSamples;
        }

        float* viewChannels[AnalysisTapBuffer::kMaxChannels]{};
        for (int channel = 0; channel < burstChannels; ++channel)
        {
            viewChannels[channel] = burstScratch.getWritePointer(channel);
        }

        juce::AudioBuffer<float> scratch(viewChannels, burstChannels, burstWindowSamples);
        AnalysisFrame frame;
        frame.tSec = tap.tSec;
        analysis->setPinnedGenre(static_cast<aifred::dsp::GenreId>(
            pinnedReferenceGenreIndex.load(std::memory_order_relaxed)));
        if (analysis->process(scratch, tap.tSec, frame))
        {
            (void)uiFifo.push(frame);
        }

        const int retainedSamples = juce::jmax(1024, burstWindowSamples - burstHopSamples);
        if (retainedSamples < burstSamples)
        {
            const int retainOffset = burstSamples - retainedSamples;
            for (int channel = 0; channel < burstChannels; ++channel)
            {
                auto* dst = burstScratch.getWritePointer(channel);
                std::memmove(dst, dst + retainOffset,
                             static_cast<std::size_t>(retainedSamples) * sizeof(float));
            }
            burstSamples = retainedSamples;
        }
    }
}
