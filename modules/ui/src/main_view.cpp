#include "dawai/ui/main_view.hpp"

#include "dawai/advisory_layer/api_key_store.hpp"
#include "dawai/advisory_layer/local_brain_adviser.hpp"
#include "dawai/aifr3d_core/analysis_writer.hpp"
#include "dawai/aifr3d_core/feature_ingestion.hpp"
#include "dawai/aifr3d_core/fix_list.hpp"
#include "dawai/aifr3d_core/session_store.hpp"
#include "dawai/aifr3d_core/scoring.hpp"
#include "dawai/aifr3d_core/system_contract.hpp"
#include "dawai/ui/theme/theme.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <fstream>
#include <optional>

namespace dawai::ui
{

namespace
{
constexpr auto kMissingKeyFeedback = "OpenAI API key missing — AIFR3D chat unavailable";
constexpr auto kOfflineFeedback = "OpenAI request failed — AIFR3D chat unavailable";

bool hasProgramMaterial(const dawai::metering::MeterSnapshot& snapshot)
{
    return snapshot.loudness.shortTermLufs > -60.0 || snapshot.dynamics.transientDensity > 0.01 ||
           snapshot.loudness.truePeakDbtp > -50.0;
}

juce::String advisoryUnavailableMessage()
{
    return dawai::advisory_layer::ApiKeyStore::loadOpenAiKey().empty()
               ? juce::String(kMissingKeyFeedback)
               : juce::String(kOfflineFeedback);
}

juce::String prettyGenre(const std::string& genre)
{
    if (genre == "hip-hop")
    {
        return "Hip-Hop";
    }
    if (genre == "edm")
    {
        return "EDM";
    }
    if (genre == "dubstep")
    {
        return "Dubstep";
    }
    if (genre == "rap")
    {
        return "Rap";
    }
    if (genre == "rock")
    {
        return "Rock";
    }
    if (genre == "pop")
    {
        return "Pop";
    }
    return "Unknown";
}

std::vector<MixCandle> defaultCandles(std::size_t count)
{
    std::vector<MixCandle> candles(count);
    for (auto& candle : candles)
    {
        candle.open = 0.5F;
        candle.close = 0.5F;
        candle.high = 0.52F;
        candle.low = 0.48F;
    }
    return candles;
}

std::vector<std::string> loadModelOptions()
{
    return {"gpt-5.2"};
}

juce::File standaloneLayoutStateFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("AIFR3D")
        .getChildFile("standalone_layout_variant_2_2_4.txt");
}

int loadPersistedStandaloneLayoutVariant()
{
    const auto file = standaloneLayoutStateFile();
    if (!file.existsAsFile())
    {
        return 1;
    }
    return juce::jlimit(1, 4, file.loadFileAsString().trim().getIntValue());
}

void savePersistedStandaloneLayoutVariant(int variant)
{
    const auto file = standaloneLayoutStateFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(juce::String(juce::jlimit(1, 4, variant)));
}

} // namespace

juce::Image MainView::loadBrandLogo() const
{
    if (const auto envPath = juce::SystemStats::getEnvironmentVariable("AIFR3D_LOGO_PATH", "");
        envPath.isNotEmpty())
    {
        const juce::File envFile(envPath);
        if (envFile.existsAsFile())
        {
            if (auto image = juce::ImageFileFormat::loadFrom(envFile); image.isValid())
            {
                return image;
            }
        }
    }

    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    const std::array<juce::File, 11> candidates{
        home.getChildFile(".local/share/icons/hicolor/512x512/apps/aifr3d.png"),
        home.getChildFile(".local/share/icons/hicolor/256x256/apps/aifr3d.png"),
        cwd.getChildFile("assets/icons/aifred_logo.png"),
        cwd.getChildFile("../assets/icons/aifred_logo.png"),
        cwd.getChildFile("assets/icons/aifred_logo.jpg"),
        cwd.getChildFile("../assets/icons/aifred_logo.jpg"),
        home.getChildFile("Pictures/North3rnLight3r_Brand_Assets/Aifred_vst_logo.jpg"),
        home.getChildFile("Pictures/North3rnLight3r_Brand_Assets/Aifred_vst_logo.png"),
        home.getChildFile(
            "Pictures/aifred-series_facebook_content/file_00000000b80c720cb4b68df64175a745.png"),
        cwd.getChildFile("assets/branding/aifred_logo.png"),
        cwd.getChildFile("../assets/branding/aifred_logo.png"),
    };

    for (const auto& candidate : candidates)
    {
        if (candidate.existsAsFile())
        {
            if (auto image = juce::ImageFileFormat::loadFrom(candidate); image.isValid())
            {
                return image;
            }
        }
    }
    return {};
}

bool MainView::loadReferencePool()
{
    const std::array<std::filesystem::path, 2> manifests{
        "assets/reference_pools/pro_25x6/pool_manifest.json",
        "assets/reference_pools/canonical_25x5/pool_manifest.json",
    };

    for (const auto& manifest : manifests)
    {
        if (!std::filesystem::exists(manifest))
        {
            continue;
        }

        const auto root = manifest.parent_path();
        if (m_genreDetector.load(manifest, root))
        {
            m_poolLabel.setText("Reference pool: " + juce::String(static_cast<int>(m_genreDetector.referenceCount())) +
                                    " tracks",
                                juce::dontSendNotification);
            return true;
        }
    }

    m_poolLabel.setText("Reference pool: unavailable", juce::dontSendNotification);
    return false;
}

void MainView::loadPersistentCandles()
{
    dawai::aifr3d_core::SessionStore store(m_candleStatePath, kSessionCandleCount);
    const auto state = store.load();
    m_firstSessionAverage = state.firstSessionBaseline;
    m_candleWrite = state.writeIndex % m_candleBuffer.size();

    for (std::size_t i = 0; i < m_candleBuffer.size() && i < state.candles.size(); ++i)
    {
        m_candleBuffer[i].open = juce::jlimit(0.0F, 1.0F, state.candles[i].open);
        m_candleBuffer[i].close = juce::jlimit(0.0F, 1.0F, state.candles[i].close);
        m_candleBuffer[i].high = juce::jlimit(0.0F, 1.0F, state.candles[i].high);
        m_candleBuffer[i].low = juce::jlimit(0.0F, 1.0F, state.candles[i].low);
        m_candleBuffer[i].flags = state.candles[i].flags;
    }
}

void MainView::persistCandlesIfNeeded()
{
    if (!m_candleDirty)
    {
        return;
    }

    if (++m_candlePersistTick < 18)
    {
        return;
    }
    m_candlePersistTick = 0;

    dawai::aifr3d_core::SessionStore store(m_candleStatePath, kSessionCandleCount);
    dawai::aifr3d_core::SessionStoreData state;
    state.firstSessionBaseline = m_firstSessionAverage;
    state.maxSessions = kSessionCandleCount;
    state.writeIndex = m_candleWrite % kSessionCandleCount;
    state.candles.reserve(m_candleBuffer.size());

    for (const auto& candle : m_candleBuffer)
    {
        dawai::aifr3d_core::SessionCandleRecord record;
        record.open = candle.open;
        record.close = candle.close;
        record.high = candle.high;
        record.low = candle.low;
        record.flags = candle.flags;
        state.candles.push_back(record);
    }

    (void)store.save(std::move(state));
    store.appendLogLine("standalone session candle store persisted");
    m_candleDirty = false;
}

void MainView::pushCandle(double alignment01, double signalStability, double decisionMean,
                          double phaseRisk)
{
    if (m_candleBuffer.empty())
    {
        return;
    }

    const bool playing = (m_playback.transportState() == dawai::audio_engine::TransportState::Playing);
    const double nowSec = m_playback.transportSeconds();
    const float align = juce::jlimit(0.0F, 1.0F, static_cast<float>(alignment01));
    const float baseline = (m_firstSessionAverage < 0.0F) ? 0.5F : m_firstSessionAverage;
    const float referenceDelta = juce::jlimit(-1.0F, 1.0F, align - baseline);
    const float stability = juce::jlimit(-0.5F, 0.5F, static_cast<float>(signalStability) - 0.5F);
    const float decision = juce::jlimit(-0.5F, 0.5F, static_cast<float>(decisionMean) - 0.5F);
    const float signaturePrice =
        juce::jlimit(0.0F, 1.0F, 0.5F + 0.30F * referenceDelta + 0.10F * decision + 0.08F * stability);

    if (playing && !m_liveSessionActive)
    {
        const std::size_t prev = (m_candleWrite == 0) ? (m_candleBuffer.size() - 1) : (m_candleWrite - 1);
        const float prevClose = m_candleBuffer[prev].close;
        m_liveOpen = (prevClose <= 0.001F) ? baseline : prevClose;
        m_liveClose = m_liveOpen;
        m_liveHigh = m_liveOpen;
        m_liveLow = m_liveOpen;
        m_liveFlags = 0u;
        m_liveSamples = 0;
        m_liveDeltaSum = 0.0F;
        m_liveSessionStartSec = nowSec;
        m_liveSessionActive = true;
    }

    if (m_liveSessionActive && playing)
    {
        m_liveClose = signaturePrice;
        m_liveHigh = juce::jmax(m_liveHigh, signaturePrice);
        m_liveLow = juce::jmin(m_liveLow, signaturePrice);
        m_liveFlags |= (phaseRisk > 0.35) ? (1u << 4) : 0u;
        ++m_liveSamples;
        m_liveDeltaSum += referenceDelta;
    }

    if (!m_liveSessionActive || playing)
    {
        return;
    }

    // Session closes when transport stops; each candle is one previous mix session.
    const double durationSec = nowSec - m_liveSessionStartSec;
    if (m_liveSamples < 8 || durationSec < 20.0)
    {
        m_liveSessionActive = false;
        return;
    }

    MixCandle candle;
    candle.open = m_liveOpen;
    candle.close = m_liveClose;
    candle.high = m_liveHigh;
    candle.low = m_liveLow;
    candle.flags = m_liveFlags;

    if (m_firstSessionAverage < 0.0F)
    {
        m_firstSessionAverage = candle.close;
    }

    m_lastDeltaPct = (m_liveDeltaSum / static_cast<float>(juce::jmax(1, m_liveSamples))) * 100.0F;
    m_candleBuffer[m_candleWrite] = candle;
    m_candleWrite = (m_candleWrite + 1) % m_candleBuffer.size();
    m_candleDirty = true;
    m_liveSessionActive = false;
}

MainView::MainView()
    : m_playback(5), m_advisory(dawai::advisory_layer::LocalBrainAdviser::createFromEnvironment())
{
    setLookAndFeel(&m_lookAndFeel);

    m_headerLabel.setText("AIFR3D Standalone", juce::dontSendNotification);
    m_headerLabel.setFont(dawai::ui::theme::headingFont().withHeight(24.0F));
    m_brandLogo = loadBrandLogo();

    m_genreLabel.setText("Auto Genre: Unknown", juce::dontSendNotification);
    m_genreLabel.setColour(juce::Label::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_genreLabel.setJustificationType(juce::Justification::centredLeft);
    m_genreLabel.setFont(dawai::ui::theme::bodyFont().withHeight(15.5F));

    m_poolLabel.setText("Reference pool: loading...", juce::dontSendNotification);
    m_poolLabel.setColour(juce::Label::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_poolLabel.setJustificationType(juce::Justification::centredLeft);
    m_poolLabel.setFont(dawai::ui::theme::bodyFont().withHeight(15.5F));
    const auto modelOptions = loadModelOptions();
    int modelId = 1;
    for (const auto& model : modelOptions)
    {
        m_modelBox.addItem(model, modelId++);
    }
    m_modelBox.setSelectedId(1, juce::dontSendNotification);
    m_modelBox.setTooltip("Model locked to GPT-5.2");
    m_modelBox.setEnabled(false);
    m_layoutLabel.setText("Theme Layout", juce::dontSendNotification);
    m_layoutLabel.setJustificationType(juce::Justification::centredRight);
    m_layoutLabel.setColour(juce::Label::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_layoutLabel.setFont(dawai::ui::theme::bodyFont().withHeight(15.0F));
    m_layoutBox.addItem("A", 1);
    m_layoutBox.addItem("B", 2);
    m_layoutBox.addItem("C", 3);
    m_layoutBox.addItem("D", 4);
    m_layoutVariant = loadPersistedStandaloneLayoutVariant();
    m_layoutBox.setSelectedId(m_layoutVariant, juce::dontSendNotification);
    m_layoutBox.onChange = [this]
    {
        m_layoutVariant = juce::jlimit(1, 4, m_layoutBox.getSelectedId());
        savePersistedStandaloneLayoutVariant(m_layoutVariant);
        resized();
    };

    // Subtle mascot marker in header corner.
    m_mascotHeader.setText("<>", juce::dontSendNotification);
    m_mascotHeader.setJustificationType(juce::Justification::centredRight);
    m_mascotHeader.setFont(dawai::ui::theme::headingFont().withHeight(18.0F));

    addAndMakeVisible(m_headerLabel);
    addAndMakeVisible(m_genreLabel);
    addAndMakeVisible(m_poolLabel);
    addAndMakeVisible(m_modelBox);
    addAndMakeVisible(m_layoutLabel);
    addAndMakeVisible(m_layoutBox);
    addAndMakeVisible(m_mascotHeader);
    addAndMakeVisible(m_tabMeter);
    addAndMakeVisible(m_tabReference);
    addAndMakeVisible(m_tabCompare);
    addAndMakeVisible(m_tabStudio);
    addAndMakeVisible(m_tabHint);
    addAndMakeVisible(m_transport);
    addAndMakeVisible(m_timeline);
    addAndMakeVisible(m_aifr3d);
    addAndMakeVisible(m_candles);
    addAndMakeVisible(m_applyButton);
    addAndMakeVisible(m_previewButton);
    addAndMakeVisible(m_undoButton);
    addAndMakeVisible(m_cpuLabel);

    m_applyButton.setColour(juce::TextButton::buttonColourId, dawai::ui::theme::Palette::accent());
    m_previewButton.setColour(juce::TextButton::buttonColourId,
                              dawai::ui::theme::Palette::accentMuted());
    m_undoButton.setColour(juce::TextButton::buttonColourId, dawai::ui::theme::Palette::panelAlt());

    m_applyButton.setTooltip("Apply conservative, non-destructive suggestion chain");
    m_previewButton.setTooltip("Preview proposed chain without committing");
    m_undoButton.setTooltip("Undo last suggestion action");

    m_cpuLabel.setJustificationType(juce::Justification::centredRight);
    m_cpuLabel.setColour(juce::Label::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_cpuLabel.setFont(dawai::ui::theme::bodyFont().withHeight(14.5F));

    for (auto* tab : {&m_tabMeter, &m_tabReference, &m_tabCompare, &m_tabStudio})
    {
        tab->setClickingTogglesState(true);
        tab->setRadioGroupId(72);
    }
    m_tabMeter.setToggleState(true, juce::dontSendNotification);
    m_tabHint.setText("Analyze tab: live DSP cockpit with current loudness, tone, stereo, and punch",
                      juce::dontSendNotification);
    m_tabHint.setColour(juce::Label::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_tabHint.setJustificationType(juce::Justification::centredLeft);
    m_tabHint.setFont(dawai::ui::theme::bodyFont().withHeight(14.5F));

    m_tabMeter.onClick = [this]
    {
        m_activeTab = ViewTab::Meter;
        m_aifr3d.setMode(Aifr3dPanel::Mode::True);
        m_tabHint.setText("Analyze tab: realtime LUFS, peak, stereo, and insight",
                          juce::dontSendNotification);
        resized();
    };
    m_tabReference.onClick = [this]
    {
        m_activeTab = ViewTab::Reference;
        m_aifr3d.setMode(Aifr3dPanel::Mode::Reference);
        m_tabHint.setText("Reference tab: live mix vs canonical genre/reference corridor",
                          juce::dontSendNotification);
        resized();
    };
    m_tabCompare.onClick = [this]
    {
        m_activeTab = ViewTab::Compare;
        m_aifr3d.setMode(Aifr3dPanel::Mode::Compare);
        m_tabHint.setText("Compare tab: capture Mix A and Mix B or compare Mix A against live",
                          juce::dontSendNotification);
        resized();
    };
    m_tabStudio.onClick = [this]
    {
        m_activeTab = ViewTab::Studio;
        m_aifr3d.setMode(Aifr3dPanel::Mode::True);
        m_tabHint.setText("Studio tab: 5-channel mini mixer and VST insert host", juce::dontSendNotification);
        resized();
    };

    m_previewButton.setClickingTogglesState(true);

    m_applyButton.onClick = [this]
    {
        const auto session = m_playback.sessionState();
        if (session.tracks.empty())
        {
            m_poolLabel.setText("Apply skipped: no active tracks", juce::dontSendNotification);
            return;
        }

        m_undoSnapshot = session;
        m_hasUndoSnapshot = true;

        const float currentDb = session.tracks.front().faderDb;
        const float targetDb = juce::jlimit(-24.0F, 12.0F, currentDb - 0.75F);
        m_playback.setTrackFaderDb(0, targetDb);
        m_poolLabel.setText("Applied conservative level move on Track 1", juce::dontSendNotification);
    };

    m_previewButton.onClick = [this]
    {
        m_previewMode = m_previewButton.getToggleState();
        m_playback.setTrackSolo(0, m_previewMode);
        m_poolLabel.setText(m_previewMode ? "Preview mode: Track 1 solo enabled"
                                          : "Preview mode: Track 1 solo disabled",
                            juce::dontSendNotification);
    };

    m_undoButton.onClick = [this]
    {
        if (!m_hasUndoSnapshot)
        {
            m_poolLabel.setText("Undo skipped: no snapshot captured", juce::dontSendNotification);
            return;
        }

        m_playback.setSessionState(m_undoSnapshot);
        m_hasUndoSnapshot = false;
        m_poolLabel.setText("Undo complete: prior session state restored", juce::dontSendNotification);
    };

    m_transport.onPlay = [this] { m_playback.play(); };
    m_transport.onStop = [this] { m_playback.stop(); };
    m_transport.onPause = [this] { m_playback.pause(); };
    m_transport.onLoadTrack1 = [this]
    {
        juce::FileChooser chooser("Load audio file", {}, "*.wav;*.aiff;*.aif");
        if (chooser.browseForFileToOpen())
        {
            m_playback.loadAudioToTrack(chooser.getResult().getFullPathName().toStdString(), 0);
        }
    };

    m_playback.initialiseAudio();

    m_candleBuffer = defaultCandles(kSessionCandleCount);
    loadPersistentCandles();
    m_referenceReady = loadReferencePool();

    m_mixerStrips.reserve(5);
    for (int i = 0; i < 5; ++i)
    {
        auto strip = std::make_unique<MixerStrip>(i);
        strip->onMute = [this](int track, bool mute) { m_playback.setTrackMute(track, mute); };
        strip->onSolo = [this](int track, bool solo) { m_playback.setTrackSolo(track, solo); };
        strip->onFader = [this](int track, float db) { m_playback.setTrackFaderDb(track, db); };
        strip->onPan = [this](int track, float pan) { m_playback.setTrackPan(track, pan); };
        strip->onInsertBypass = [this](int track, int slot, bool bypass)
        { m_playback.setTrackInsertBypass(track, slot, bypass); };
        strip->onInsertLoad = [this](int track, int slot)
        {
            juce::FileChooser chooser("Load VST3 insert", {}, "*.vst3");
            if (!chooser.browseForFileToOpen())
            {
                return;
            }

            juce::String error;
            if (m_playback.loadVst3ToTrackInsert(
                    chooser.getResult().getFullPathName().toStdString(), track, slot, error))
            {
                m_poolLabel.setText("Loaded VST3 insert on Track " + juce::String(track + 1) +
                                        " Slot " + juce::String(slot + 1),
                                    juce::dontSendNotification);
            }
            else
            {
                m_poolLabel.setText("VST3 load failed: " + error, juce::dontSendNotification);
            }
        };

        addAndMakeVisible(*strip);
        m_mixerStrips.push_back(std::move(strip));
    }

    startTimerHz(60);
}

MainView::~MainView()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MainView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const float pulse = 0.45f + 0.55f * m_visualPulse;
    const float shimmer = 0.5f + 0.5f * std::sin(m_visualPhase * 0.9f);
    juce::ColourGradient bg(dawai::ui::theme::Palette::panel().brighter(0.04f), bounds.getX(),
                            bounds.getY(), dawai::ui::theme::Palette::matteBlack(), bounds.getX(),
                            bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(dawai::ui::theme::Palette::stroke());
    g.drawRoundedRectangle(bounds.reduced(1.0f), 10.0f, 1.0f);

    juce::ColourGradient halo(dawai::ui::theme::Palette::accent().withAlpha(0.0f), bounds.getX(),
                              bounds.getY(),
                              dawai::ui::theme::Palette::accent().withAlpha(0.14f * pulse),
                              bounds.getCentreX(), bounds.getCentreY(), true);
    halo.addColour(1.0, dawai::ui::theme::Palette::accentPurple().withAlpha(0.0f));
    g.setGradientFill(halo);
    g.fillRoundedRectangle(bounds.reduced(2.0f), 10.0f);

    if (m_brandLogo.isValid())
    {
        const auto logoBounds = juce::Rectangle<int>(8, 5, 24, 24);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageWithin(m_brandLogo, logoBounds.getX(), logoBounds.getY(), logoBounds.getWidth(),
                          logoBounds.getHeight(),
                          juce::RectanglePlacement::centred |
                              juce::RectanglePlacement::onlyReduceInSize);
        auto glow = logoBounds.toFloat().expanded(1.0f + 2.0f * shimmer);
        g.setColour(dawai::ui::theme::Palette::accent().withAlpha(0.10f + 0.10f * shimmer));
        g.drawRoundedRectangle(glow, 7.0f, 1.0f);
    }
}

void MainView::resized()
{
    auto area = getLocalBounds().reduced(8);

    const bool compactHeader = area.getWidth() < 1280;
    const bool stackedHeader = area.getWidth() < 1120;
    auto header = area.removeFromTop(stackedHeader ? 126 : compactHeader ? 98 : 68);

    if (!compactHeader)
    {
        auto leftHeader = header.removeFromLeft(500);
        m_headerLabel.setBounds(leftHeader.removeFromTop(24));
        m_genreLabel.setBounds(leftHeader.removeFromTop(12));
        m_poolLabel.setBounds(leftHeader.removeFromTop(12));
        auto rightHeader = header.removeFromRight(460);
        m_modelBox.setBounds(rightHeader.removeFromTop(28).reduced(0, 3));
        auto layoutRow = rightHeader.removeFromTop(24);
        m_layoutLabel.setBounds(layoutRow.removeFromLeft(110));
        m_layoutBox.setBounds(layoutRow.removeFromLeft(84));
        layoutRow.removeFromLeft(8);
        m_tabHint.setBounds(rightHeader.reduced(0, 1));
        auto tabRow = header.removeFromLeft(380);
        m_tabMeter.setBounds(tabRow.removeFromLeft(84).reduced(2));
        m_tabReference.setBounds(tabRow.removeFromLeft(102).reduced(2));
        m_tabCompare.setBounds(tabRow.removeFromLeft(98).reduced(2));
        m_tabStudio.setBounds(tabRow.removeFromLeft(86).reduced(2));
        m_mascotHeader.setBounds(header.removeFromRight(40));
    }
    else
    {
        auto row1 = header.removeFromTop(28);
        auto row1Left = row1.removeFromLeft(juce::jmax(280, row1.getWidth() / 2));
        m_headerLabel.setBounds(row1Left);
        m_mascotHeader.setBounds(row1.removeFromRight(34));
        m_modelBox.setBounds(row1.removeFromRight(124).reduced(0, 3));
        row1.removeFromRight(6);
        m_layoutBox.setBounds(row1.removeFromRight(82).reduced(0, 1));
        row1.removeFromRight(6);
        m_layoutLabel.setBounds(row1.removeFromRight(108));

        header.removeFromTop(4);
        auto row2 = header.removeFromTop(28);
        if (stackedHeader)
        {
            m_genreLabel.setBounds(row2.removeFromTop(12));
            m_poolLabel.setBounds(row2.removeFromTop(12));
        }
        else
        {
            auto leftMeta = row2.removeFromLeft(row2.getWidth() / 2);
            m_genreLabel.setBounds(leftMeta);
            m_poolLabel.setBounds(row2);
        }

        header.removeFromTop(4);
        auto tabRow = header.removeFromTop(28);
        const int tabWidth = juce::jmax(76, (tabRow.getWidth() - 18) / 4);
        m_tabMeter.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2));
        tabRow.removeFromLeft(6);
        m_tabReference.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2));
        tabRow.removeFromLeft(6);
        m_tabCompare.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2));
        tabRow.removeFromLeft(6);
        m_tabStudio.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2));

        header.removeFromTop(4);
        m_tabHint.setBounds(header.removeFromTop(22));
    }

    area.removeFromTop(6);
    m_transport.setBounds(area.removeFromTop(40));
    area.removeFromTop(6);

    switch (m_layoutVariant)
    {
    case 2:
        layoutVariantB(area);
        break;
    case 3:
        layoutVariantC(area);
        break;
    case 4:
        layoutVariantD(area);
        break;
    default:
        layoutVariantA(area);
        break;
    }
}

void MainView::layoutVariantA(juce::Rectangle<int> area)
{
    auto topSplit = area.removeFromTop(area.proportionOfHeight(0.52F));
    auto leftTop = topSplit.removeFromLeft(area.proportionOfWidth(0.60F));
    m_timeline.setBounds(leftTop);

    topSplit.removeFromLeft(6);
    auto rightTop = topSplit;
    m_aifr3d.setBounds(rightTop.removeFromTop(topSplit.proportionOfHeight(0.72F)));
    rightTop.removeFromTop(6);
    m_candles.setBounds(rightTop);

    area.removeFromTop(6);
    const bool studioTab = (m_activeTab == ViewTab::Studio);
    m_candles.setVisible(!studioTab);
    for (auto& strip : m_mixerStrips)
    {
        strip->setVisible(studioTab);
    }
    m_applyButton.setVisible(studioTab);
    m_previewButton.setVisible(studioTab);
    m_undoButton.setVisible(studioTab);

    if (!studioTab)
    {
        auto candlesArea = area.reduced(0, 2);
        m_candles.setBounds(candlesArea);
        m_cpuLabel.setBounds(area.removeFromBottom(24));
        return;
    }

    auto mixerArea = area.removeFromTop(area.proportionOfHeight(0.72F));
    const int stripWidth = juce::jmax(82, mixerArea.getWidth() / static_cast<int>(m_mixerStrips.size()) - 4);
    for (auto& strip : m_mixerStrips)
    {
        strip->setBounds(mixerArea.removeFromLeft(stripWidth));
        mixerArea.removeFromLeft(4);
    }

    area.removeFromTop(4);
    auto actions = area.removeFromTop(30);
    m_applyButton.setBounds(actions.removeFromLeft(90));
    actions.removeFromLeft(6);
    m_previewButton.setBounds(actions.removeFromLeft(100));
    actions.removeFromLeft(6);
    m_undoButton.setBounds(actions.removeFromLeft(90));
    m_cpuLabel.setBounds(actions.removeFromRight(240));
}

void MainView::layoutVariantB(juce::Rectangle<int> area)
{
    const bool studioTab = (m_activeTab == ViewTab::Studio);
    m_candles.setVisible(!studioTab);
    for (auto& strip : m_mixerStrips)
    {
        strip->setVisible(studioTab);
    }
    m_applyButton.setVisible(studioTab);
    m_previewButton.setVisible(studioTab);
    m_undoButton.setVisible(studioTab);

    auto top = area.removeFromTop(area.proportionOfHeight(studioTab ? 0.44F : 0.50F));
    auto topLeft = top.removeFromLeft(top.proportionOfWidth(0.52F));
    m_timeline.setBounds(topLeft);
    top.removeFromLeft(6);
    m_aifr3d.setBounds(top);

    area.removeFromTop(6);
    if (!studioTab)
    {
        m_candles.setBounds(area.removeFromTop(area.proportionOfHeight(0.86F)));
        m_cpuLabel.setBounds(area.removeFromBottom(24));
        return;
    }

    auto mixerArea = area.removeFromTop(area.proportionOfHeight(0.74F));
    const int stripWidth = juce::jmax(84, mixerArea.getWidth() / static_cast<int>(m_mixerStrips.size()) - 5);
    for (auto& strip : m_mixerStrips)
    {
        strip->setBounds(mixerArea.removeFromLeft(stripWidth));
        mixerArea.removeFromLeft(5);
    }

    area.removeFromTop(4);
    auto actions = area.removeFromTop(30);
    m_applyButton.setBounds(actions.removeFromLeft(90));
    actions.removeFromLeft(6);
    m_previewButton.setBounds(actions.removeFromLeft(100));
    actions.removeFromLeft(6);
    m_undoButton.setBounds(actions.removeFromLeft(90));
    m_cpuLabel.setBounds(actions.removeFromRight(240));
}

void MainView::layoutVariantC(juce::Rectangle<int> area)
{
    const bool studioTab = (m_activeTab == ViewTab::Studio);
    m_candles.setVisible(!studioTab);
    for (auto& strip : m_mixerStrips)
    {
        strip->setVisible(studioTab);
    }
    m_applyButton.setVisible(studioTab);
    m_previewButton.setVisible(studioTab);
    m_undoButton.setVisible(studioTab);

    auto rails = area.removeFromTop(area.proportionOfHeight(studioTab ? 0.48F : 0.56F));
    auto leftRail = rails.removeFromLeft(juce::jmax(280, rails.proportionOfWidth(0.38F)));
    auto midRail = rails.removeFromLeft(juce::jmax(250, rails.proportionOfWidth(0.46F)));
    rails.removeFromLeft(6);
    auto rightRail = rails;

    m_timeline.setBounds(leftRail);
    m_aifr3d.setBounds(midRail.reduced(3));
    m_candles.setBounds(rightRail.reduced(3));

    area.removeFromTop(6);
    if (!studioTab)
    {
        m_candles.setBounds(area.removeFromTop(area.proportionOfHeight(0.84F)));
        m_cpuLabel.setBounds(area.removeFromBottom(24));
        return;
    }

    auto mixerArea = area.removeFromTop(area.proportionOfHeight(0.72F));
    const int stripWidth = juce::jmax(80, mixerArea.getWidth() / static_cast<int>(m_mixerStrips.size()) - 4);
    for (auto& strip : m_mixerStrips)
    {
        strip->setBounds(mixerArea.removeFromLeft(stripWidth));
        mixerArea.removeFromLeft(4);
    }
    area.removeFromTop(4);
    auto actions = area.removeFromTop(30);
    m_applyButton.setBounds(actions.removeFromLeft(90));
    actions.removeFromLeft(6);
    m_previewButton.setBounds(actions.removeFromLeft(100));
    actions.removeFromLeft(6);
    m_undoButton.setBounds(actions.removeFromLeft(90));
    m_cpuLabel.setBounds(actions.removeFromRight(240));
}

void MainView::layoutVariantD(juce::Rectangle<int> area)
{
    const bool studioTab = (m_activeTab == ViewTab::Studio);
    m_candles.setVisible(!studioTab);
    for (auto& strip : m_mixerStrips)
    {
        strip->setVisible(studioTab);
    }
    m_applyButton.setVisible(studioTab);
    m_previewButton.setVisible(studioTab);
    m_undoButton.setVisible(studioTab);

    auto top = area.removeFromTop(area.proportionOfHeight(studioTab ? 0.42F : 0.50F));
    auto topLeft = top.removeFromLeft(top.proportionOfWidth(0.50F));
    m_candles.setBounds(topLeft);
    top.removeFromLeft(6);
    m_aifr3d.setBounds(top);

    area.removeFromTop(6);
    if (!studioTab)
    {
        m_timeline.setBounds(area.removeFromTop(area.proportionOfHeight(0.82F)));
        m_cpuLabel.setBounds(area.removeFromBottom(24));
        return;
    }

    m_timeline.setBounds(area.removeFromTop(area.proportionOfHeight(0.26F)));
    area.removeFromTop(6);
    auto mixerArea = area.removeFromTop(area.proportionOfHeight(0.66F));
    const int stripWidth = juce::jmax(80, mixerArea.getWidth() / static_cast<int>(m_mixerStrips.size()) - 4);
    for (auto& strip : m_mixerStrips)
    {
        strip->setBounds(mixerArea.removeFromLeft(stripWidth));
        mixerArea.removeFromLeft(4);
    }
    area.removeFromTop(4);
    auto actions = area.removeFromTop(30);
    m_applyButton.setBounds(actions.removeFromLeft(90));
    actions.removeFromLeft(6);
    m_previewButton.setBounds(actions.removeFromLeft(100));
    actions.removeFromLeft(6);
    m_undoButton.setBounds(actions.removeFromLeft(90));
    m_cpuLabel.setBounds(actions.removeFromRight(240));
}

void MainView::timerCallback()
{
    m_transport.setTimeSeconds(m_playback.transportSeconds());

    const auto session = m_playback.sessionState();
    m_timeline.setSession(session);
    m_timeline.setPlayheadSeconds(m_playback.transportSeconds());

    const auto snapshot = m_playback.lastMeterSnapshot();
    m_aifr3d.setMeterSnapshot(snapshot);
    const bool liveProgramMaterial = hasProgramMaterial(snapshot);
    const float motionEnergy =
        liveProgramMaterial
            ? juce::jlimit(0.0F, 1.0F,
                           0.42F * static_cast<float>(snapshot.stereo.width) +
                               0.28F *
                                   juce::jlimit(0.0F, 1.0F,
                                                static_cast<float>(snapshot.dynamics.transientDensity)) +
                               0.30F *
                                   juce::jlimit(0.0F, 1.0F,
                                                static_cast<float>((snapshot.loudness.shortTermLufs +
                                                                    30.0) /
                                                                   24.0)))
            : 0.0F;
    m_visualPhase += liveProgramMaterial ? (0.030F + motionEnergy * 0.022F) : 0.010F;
    if (m_visualPhase > juce::MathConstants<float>::twoPi)
    {
        m_visualPhase -= juce::MathConstants<float>::twoPi;
    }
    const float shimmer = 0.5F + 0.5F * std::sin(m_visualPhase * 1.3F);
    m_visualPulse = liveProgramMaterial
                        ? juce::jlimit(0.0F, 1.0F,
                                       0.12F + 0.44F * motionEnergy + 0.08F * shimmer)
                        : juce::jlimit(0.0F, 1.0F, 0.012F + 0.006F * shimmer);
    m_aifr3d.setAnimationPhase(m_visualPhase, m_visualPulse);

    if (!liveProgramMaterial)
    {
        m_genreLabel.setText("Auto Genre: waiting for program material",
                             juce::dontSendNotification);
        m_candles.setAnimationPhase(m_visualPhase, m_visualPulse);

        const auto cpu = m_playback.profiler().snapshotCpuMs();
        double sum = 0.0;
        for (const auto& [_, value] : cpu)
        {
            sum += value;
        }
        m_cpuLabel.setText("Profiler: " + juce::String(sum, 2) + " ms",
                           juce::dontSendNotification);
        repaint();
        return;
    }

    if ((++m_analysisTick % 6) != 0)
    {
        m_candles.setAnimationPhase(m_visualPhase, m_visualPulse);
        repaint();
        return;
    }

    dawai::reference_engine::ReferenceProfile reference;
    dawai::reference_engine::ReferenceProfile targetProfile;
    std::string detectedGenre = "unknown";
    double detectedConfidence = 0.0;

    if (m_referenceReady)
    {
        if (auto detection = m_genreDetector.detect(snapshot); detection.has_value())
        {
            reference = detection->referenceMatch;
            targetProfile = detection->referenceMean;
            detectedGenre = detection->genre;
            detectedConfidence = detection->confidence;
        }
    }

    if (reference.id.empty())
    {
        reference.id = "fallback";
        reference.title = "Fallback";
        reference.spectrumBandsDb = snapshot.spectrum.averagedBinsDb;
        reference.integratedLufs = snapshot.loudness.integratedLufs - 1.0;
        reference.truePeakDbtp = snapshot.loudness.truePeakDbtp - 0.5;
        reference.crestFactorDistribution = {snapshot.dynamics.crestFactorDb + 0.5};
        reference.stereoWidth = snapshot.stereo.width + 0.1;
        targetProfile = reference;
    }

    if (targetProfile.id.empty())
    {
        targetProfile = reference;
    }
    m_aifr3d.setReferenceTruePeakDbtp(reference.truePeakDbtp);

    m_lastDetectedGenre = detectedGenre;
    m_lastGenreConfidence = detectedConfidence;
    m_genreLabel.setText("Auto Genre: " + prettyGenre(detectedGenre) + " (" +
                             juce::String(static_cast<int>(std::round(detectedConfidence * 100.0))) +
                             "% match)",
                         juce::dontSendNotification);

    dawai::aifr3d_core::FeatureIngestor ingestor;
    auto features = ingestor.ingest(snapshot, reference, targetProfile);

    dawai::aifr3d_core::ScoringEngine scoringEngine;
    const auto scoring = scoringEngine.score(features);

    dawai::aifr3d_core::FixListGenerator fixGenerator;
    const auto fixList = fixGenerator.generate(features, scoring);

    dawai::aifr3d_core::AnalysisWriter writer;
    const auto liveAnalysisJson = writer.buildResultJson(features, scoring);
    const auto liveFixJson = writer.buildFixListJson(features, scoring, fixList);
    writer.write("analysis", features, scoring, fixList);

    const double alignment01 = juce::jlimit(0.0, 1.0, scoring.overallRating / 10.0);
    double signalClarityMean = 0.5;
    double decisionMean = alignment01;
    if (!scoring.categoryScores.empty())
    {
        double claritySum = 0.0;
        double scoreSum = 0.0;
        for (const auto& cat : scoring.categoryScores)
        {
            claritySum += cat.signalClarity;
            scoreSum += cat.score;
        }
        signalClarityMean = claritySum / static_cast<double>(scoring.categoryScores.size());
        decisionMean = juce::jlimit(0.0, 1.0, (scoreSum / static_cast<double>(scoring.categoryScores.size())) / 10.0);
    }
    const double phaseRisk = juce::jlimit(0.0, 1.0, (1.0 - snapshot.stereo.correlation) * 0.5);

    pushCandle(alignment01, signalClarityMean, decisionMean, phaseRisk);
    persistCandlesIfNeeded();
    m_candles.setCandles(m_candleBuffer, m_candleWrite,
                         (m_firstSessionAverage < 0.0F ? 0.5F : m_firstSessionAverage),
                         m_lastDeltaPct);
    m_candles.setAnimationPhase(m_visualPhase, m_visualPulse);

    const double advisoryNowSec = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const bool advisoryPeriodic = (advisoryNowSec - m_lastAdvisoryRequestSec) >= 3.0;
    const bool advisoryReactive = (m_liveFlags != m_lastAdvisoryFlags) &&
                                  ((advisoryNowSec - m_lastAdvisoryRequestSec) >= 1.0);

    if ((advisoryPeriodic || advisoryReactive) && !m_advisoryInFlight.exchange(true))
    {
        m_lastAdvisoryRequestSec = advisoryNowSec;
        m_lastAdvisoryFlags = m_liveFlags;
        m_advisory.submit(
            {"analysis",
             {detectedGenre,
              "translation",
              std::string("preserve punch; contract=") + dawai::aifr3d_core::contract::kSystemId +
                  "; metrics=" + dawai::aifr3d_core::contract::kMetricMathPack,
              "desktop-main",
              "refresh insight from the current DSP-linked audio buffer",
              m_modelBox.getText().toStdString(),
              {},
              liveAnalysisJson,
              liveFixJson},
             [this](std::optional<dawai::advisory_layer::AdvisoryOutput> output)
             {
                 m_advisoryInFlight.store(false);
                 juce::MessageManager::callAsync(
                     [this, output]
                     {
                         if (output)
                         {
                             m_aifr3d.setInsight(*output);
                             return;
                         }

                         m_aifr3d.appendInsightStatus(advisoryUnavailableMessage());
                     });
             }});
    }

    const auto cpu = m_playback.profiler().snapshotCpuMs();
    double sum = 0.0;
    for (const auto& [_, value] : cpu)
    {
        sum += value;
    }
    m_cpuLabel.setText("Profiler: " + juce::String(sum, 2) + " ms", juce::dontSendNotification);
    repaint();
}

} // namespace dawai::ui
