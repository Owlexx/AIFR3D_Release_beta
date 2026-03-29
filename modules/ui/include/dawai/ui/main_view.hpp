#pragma once

#include "dawai/advisory_layer/advisory_service.hpp"
#include "dawai/audio_engine/playback_engine.hpp"
#include "dawai/reference_engine/genre_detector.hpp"
#include "dawai/ui/aifr3d_panel.hpp"
#include "dawai/ui/candlestick_panel.hpp"
#include "dawai/ui/mixer_strip.hpp"
#include "dawai/ui/theme/look_and_feel.hpp"
#include "dawai/ui/timeline_component.hpp"
#include "dawai/ui/transport_bar.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

namespace dawai::ui
{

class MainView : public juce::Component, private juce::Timer
{
  public:
    MainView();
    ~MainView() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

  private:
    enum class ViewTab
    {
        Meter,
        Reference,
        Compare,
        Studio
    };

    juce::Image loadBrandLogo() const;
    void timerCallback() override;
    void layoutVariantA(juce::Rectangle<int> area);
    void layoutVariantB(juce::Rectangle<int> area);
    void layoutVariantC(juce::Rectangle<int> area);
    void layoutVariantD(juce::Rectangle<int> area);

    bool loadReferencePool();
    void loadPersistentCandles();
    void persistCandlesIfNeeded();
    void pushCandle(double alignment01, double signalStability, double decisionMean, double phaseRisk);

    audio_engine::PlaybackEngine m_playback;
    advisory_layer::AdvisoryService m_advisory;
    reference_engine::GenreDetector m_genreDetector;

    theme::DawLookAndFeel m_lookAndFeel;

    juce::Label m_headerLabel;
    juce::Label m_genreLabel;
    juce::Label m_poolLabel;
    juce::ComboBox m_modelBox;
    juce::Label m_layoutLabel;
    juce::ComboBox m_layoutBox;
    juce::Label m_mascotHeader;
    juce::Image m_brandLogo;
    ViewTab m_activeTab = ViewTab::Meter;

    juce::TextButton m_tabMeter{"Meter"};
    juce::TextButton m_tabReference{"Reference"};
    juce::TextButton m_tabCompare{"Compare"};
    juce::TextButton m_tabStudio{"Studio"};
    juce::Label m_tabHint;

    TransportBar m_transport;
    TimelineComponent m_timeline;
    Aifr3dPanel m_aifr3d;
    CandlestickPanel m_candles;

    std::vector<std::unique_ptr<MixerStrip>> m_mixerStrips;

    juce::TextButton m_applyButton{"APPLY"};
    juce::TextButton m_previewButton{"PREVIEW"};
    juce::TextButton m_undoButton{"UNDO"};

    juce::Label m_cpuLabel;
    int m_analysisTick = 0;
    float m_visualPhase = 0.0F;
    float m_visualPulse = 0.0F;
    std::atomic<bool> m_advisoryInFlight{false};
    double m_lastAdvisoryRequestSec = -100.0;
    uint32_t m_lastAdvisoryFlags = 0;

    std::vector<MixCandle> m_candleBuffer;
    std::size_t m_candleWrite = 0;
    static constexpr std::size_t kSessionCandleCount = 10;
    float m_firstSessionAverage = -1.0F;
    float m_lastDeltaPct = 0.0F;
    bool m_candleDirty = false;
    int m_candlePersistTick = 0;
    std::filesystem::path m_candleStatePath{"analysis/session_memory/standalone_session_store_v1.json"};
    bool m_liveSessionActive = false;
    double m_liveSessionStartSec = 0.0;
    float m_liveOpen = 0.5F;
    float m_liveClose = 0.5F;
    float m_liveHigh = 0.5F;
    float m_liveLow = 0.5F;
    uint32_t m_liveFlags = 0;
    int m_liveSamples = 0;
    float m_liveDeltaSum = 0.0F;
    float m_liveLufsSum = 0.0F;
    float m_liveTruePeakSum = 0.0F;
    float m_liveCrestSum = 0.0F;
    float m_liveWidthSum = 0.0F;
    float m_liveCorrSum = 0.0F;
    float m_liveTransientSum = 0.0F;

    bool m_referenceReady = false;
    std::string m_lastDetectedGenre{"unknown"};
    double m_lastGenreConfidence = 0.0;

    bool m_previewMode = false;
    bool m_hasUndoSnapshot = false;
    audio_engine::SessionState m_undoSnapshot;
    int m_layoutVariant = 1;
};

} // namespace dawai::ui
