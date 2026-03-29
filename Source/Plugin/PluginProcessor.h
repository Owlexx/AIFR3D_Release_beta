#pragma once

#include <JuceHeader.h>

#include "../Common/AnalysisTypes.h"
#include "../Common/SimpleLockFreeQueue.h"
#include "Parameters.h"

#include <array>
#include <atomic>
#include <thread>

namespace audiosynth::dsp
{
class AnalysisEngine;
enum class GenreId : uint8_t;
}

class CoreSynthAudioProcessor : public juce::AudioProcessor
{
  public:
    CoreSynthAudioProcessor();
    ~CoreSynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override
    {
        return true;
    }

    const juce::String getName() const override
    {
        return "AudioSuite 2.3";
    }
    double getTailLengthSeconds() const override
    {
        return 0.0;
    }
    bool acceptsMidi() const override
    {
        return false;
    }
    bool producesMidi() const override
    {
        return false;
    }
    bool isMidiEffect() const override
    {
        return false;
    }

    int getNumPrograms() override
    {
        return 1;
    }
    int getCurrentProgram() override
    {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override
    {
        return {};
    }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    using juce::AudioProcessor::processBlock;
    void setPinnedReferenceGenreIndex(int genreIndex) noexcept
    {
        pinnedReferenceGenreIndex.store(juce::jlimit(0, 6, genreIndex), std::memory_order_relaxed);
    }
    int getPinnedReferenceGenreIndex() const noexcept
    {
        return pinnedReferenceGenreIndex.load(std::memory_order_relaxed);
    }

    SimpleLockFreeQueue<AnalysisFrame, 512> uiFifo;

    juce::AudioProcessorValueTreeState apvts;

  private:
    struct AnalysisTapBuffer
    {
        static constexpr int kMaxChannels = 2;
        static constexpr int kMaxSamples = 2048;

        int numChannels = 0;
        int numSamples = 0;
        double sampleRate = 48000.0;
        double tSec = 0.0;
        std::array<std::array<float, kMaxSamples>, kMaxChannels> data{};
    };

    void analysisLoop();

    std::unique_ptr<audiosynth::dsp::AnalysisEngine> analysis;
    std::atomic<uint64_t> frameCounter{0};
    std::atomic<double> runtimeSampleRate{48000.0};
    std::atomic<int> runtimeBlockSize{512};
    std::atomic<bool> analysisNeedsPrepare{true};
    std::atomic<bool> analysisStopping{false};
    std::atomic<int> pinnedReferenceGenreIndex{0};
    SimpleLockFreeQueue<AnalysisTapBuffer, 64> analysisTapFifo;
    std::thread analysisWorker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoreSynthAudioProcessor)
};
