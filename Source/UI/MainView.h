#pragma once

#include <JuceHeader.h>

#include "../Plugin/PluginProcessor.h"
#include "ApprovalHalo.h"
#include "CandleStoryline.h"
#include "FixListPanel.h"
#include "MixTipsPanel.h"
#include "MixSignatureMeter.h"
#include "StereoArcView.h"
#include "TopBar.h"
#include "UiModel.h"
#include "coresynth/advisory_layer/advisory_service.hpp"

#include <filesystem>
#include <array>
#include <atomic>
#include <deque>
#include <vector>

class MainView : public juce::Component, private juce::Timer
{
  public:
    explicit MainView(CoreSynthAudioProcessor& p);
    ~MainView() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

  private:
    enum class ModePane
    {
        Analyze,
        Compare,
        Reference
    };

    enum class InsightPane
    {
        FixList,
        MixTips
    };

    void timerCallback() override;
    void layoutVariantA(juce::Rectangle<int> area);
    void layoutVariantB(juce::Rectangle<int> area);
    void layoutVariantC(juce::Rectangle<int> area);
    void layoutVariantD(juce::Rectangle<int> area);
    void layoutModeMenus(juce::Rectangle<int> area);
    void applyModePaneVisibility();
    void applyInsightPaneVisibility();
    void triggerAdvisory(const AnalysisFrame& frame, const juce::String& prompt,
                         const std::vector<std::filesystem::path>& attachments, bool appendToChat);
    void captureDebugFrame(const AnalysisFrame& frame);
    void flushDebugLog(bool force);
    void appendChatHistoryEntry(const juce::String& entry);
    void rememberInsightEntry(const juce::String& entry);

    CoreSynthAudioProcessor& processor;
    UiModel model;
    coresynth::advisory_layer::AdvisoryService advisory;
    std::atomic<bool> advisoryInFlight{false};
    AnalysisFrame lastFrame;
    bool hasLastFrame = false;
    std::vector<std::filesystem::path> chatAttachments;

    TopBar topBar;
    MixSignatureMeter mixSignatureMeter;
    ApprovalHalo approvalHalo;
    CandleStoryline candleStory;
    StereoArcView stereoArc;
    FixListPanel fixList;
    MixTipsPanel mixTipsPanel;

    juce::TextEditor chatPrompt;
    juce::TextEditor chatLog;
    juce::TextEditor apiKeyEntry;
    juce::TextButton chatAskButton{"Ask AI"};
    juce::TextButton chatAttachButton{"Attach File"};
    juce::TextButton apiKeySaveButton{"Link API Key"};
    juce::TextButton apiKeyClearButton{"Clear Key"};
    juce::TextButton fixListPaneButton{"Fix List"};
    juce::TextButton mixTipsPaneButton{"Mix Tips"};
    juce::TextButton clearFixListButton{"Clear Fixes"};
    juce::ToggleButton debugToggle{"Debug"};
    juce::ComboBox chatModelBox;
    juce::ComboBox pulseModeBox;
    juce::Label chatStatus;
    juce::Rectangle<int> chatPanelBounds;
    ModePane activeModePane = ModePane::Analyze;
    InsightPane activeInsightPane = InsightPane::FixList;

    juce::Label referenceMenuLabel;
    juce::ComboBox referenceGenreBox;
    juce::TextButton referencePinButton{"Pin Reference"};

    juce::Label compareMenuLabel;
    juce::TextButton comparePremixButton{"Load Premix"};
    juce::TextButton compareMixButton{"Load Mix"};
    juce::TextButton compareRunButton{"Run Compare"};
    juce::Label compareMenuStatus;

    bool debugLoggingEnabled = false;
    std::array<AnalysisFrame, 240> debugFrameRing{};
    std::deque<juce::String> chatHistoryEntries;
    std::deque<juce::String> recentInsightMemory;
    std::size_t debugFrameWrite = 0;
    std::size_t debugFrameCount = 0;
    int debugFlushTick = 0;
    std::filesystem::path debugLogPath{"analysis/debug/ui_frame_debug.log"};
    int activeLayoutVariant = 1;
    double lastAutoAdvisorySec = -100.0;
    uint32_t lastAutoAdvisoryFlags = 0;
    juce::String lastLiveAdvisorySignature;
    juce::String lastFixMemorySignature;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};
