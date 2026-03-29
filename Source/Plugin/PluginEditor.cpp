#include "PluginEditor.h"

#include "Theme.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>

namespace
{
juce::File pluginWindowStateFile()
{
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("AIFR3D")
        .getChildFile("plugin_window_state_2_3.txt");
}

juce::Rectangle<int> loadPersistedPluginBounds()
{
    const auto file = pluginWindowStateFile();
    if (!file.existsAsFile())
    {
        return {0, 0, 1280, 720};
    }

    juce::StringArray parts;
    parts.addTokens(file.loadFileAsString(), ",", "");
    if (parts.size() < 2)
    {
        return {0, 0, 1280, 720};
    }

    const int width = juce::jlimit(1180, 1920, parts[0].trim().getIntValue());
    const int height = juce::jlimit(720, 1080, parts[1].trim().getIntValue());
    return {0, 0, width, height};
}

void savePersistedPluginBounds(juce::Rectangle<int> bounds)
{
    const auto file = pluginWindowStateFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(juce::String(bounds.getWidth()) + "," + juce::String(bounds.getHeight()));
}

juce::Image loadStartupMascot()
{
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    const std::array<juce::File, 11> candidates{
        cwd.getChildFile("assets/branding/mascot_splash_2_2_4_beta.png"),
        cwd.getChildFile("../assets/branding/mascot_splash_2_2_4_beta.png"),
        cwd.getChildFile("apps/website/assets/brand/hero_mascot.png"),
        cwd.getChildFile("../apps/website/assets/brand/hero_mascot.png"),
        cwd.getChildFile("assets/branding/north3rnlight3r_mascot.png"),
        cwd.getChildFile("../assets/branding/north3rnlight3r_mascot.png"),
        cwd.getChildFile("assets/icons/aifred_logo.png"),
        cwd.getChildFile("../assets/icons/aifred_logo.png"),
        home.getChildFile(".local/share/aifr3d/assets/mascot_splash_2_2_4_beta.png"),
        home.getChildFile("Pictures/North3rnLight3r_Brand_Assets/North3rnLight3rrMascot.png"),
        home.getChildFile(".local/share/icons/hicolor/512x512/apps/aifr3d.png"),
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

void drawMascotHero(juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> area)
{
    if (!image.isValid())
    {
        return;
    }

    auto glowArea = area.reduced(area.getWidth() * 0.06f, area.getHeight() * 0.06f);
    juce::ColourGradient glow(juce::Colour(0xff47d7ff).withAlpha(0.28f), glowArea.getCentreX(),
                              glowArea.getCentreY(), juce::Colours::transparentBlack,
                              glowArea.getX(), glowArea.getBottom(), true);
    g.setGradientFill(glow);
    g.fillEllipse(glowArea.expanded(14.0f));

    juce::Path clipPath;
    clipPath.addEllipse(glowArea);
    g.saveState();
    g.reduceClipRegion(clipPath);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    const auto drawArea = glowArea.expanded(static_cast<int>(glowArea.getWidth() * 0.12f),
                                            static_cast<int>(glowArea.getHeight() * 0.12f))
                              .withCentre(glowArea.getCentre());
    g.drawImageWithin(image, drawArea.toNearestInt().getX(), drawArea.toNearestInt().getY(),
                      drawArea.toNearestInt().getWidth(), drawArea.toNearestInt().getHeight(),
                      juce::RectanglePlacement::centred);
    g.restoreState();
}

void writeTutorialSeenFlag(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << "seen\n";
}

class FirstRunTutorialOverlay : public juce::Component
{
  public:
    explicit FirstRunTutorialOverlay(std::function<void()> onDismiss)
        : m_onDismiss(std::move(onDismiss)), m_mascot(loadStartupMascot())
    {
        addAndMakeVisible(m_closeButton);
        m_closeButton.setButtonText("Start Mixing");
        m_closeButton.onClick = [this]
        {
            if (m_onDismiss)
            {
                m_onDismiss();
            }
        };
    }

    void paint(juce::Graphics& g) override
    {
        auto& theme = Theme::instance();
        g.fillAll(theme.bg.withAlpha(0.92f));

        auto panel = getLocalBounds().toFloat().reduced(48.0f, 34.0f);
        juce::ColourGradient fill(theme.panel.brighter(0.05f), panel.getX(), panel.getY(),
                                  theme.panelAlt.darker(0.18f), panel.getX(),
                                  panel.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(panel, theme.cornerRadius + 6.0f);
        g.setColour(theme.panelStroke);
        g.drawRoundedRectangle(panel.reduced(1.0f), theme.cornerRadius + 6.0f, 1.2f);

        auto content = panel.toNearestInt().reduced(28, 24);
        auto heroRow = content.removeFromTop(198);
        auto copy = heroRow.removeFromLeft(static_cast<int>(heroRow.getWidth() * 0.58f));

        g.setColour(theme.text);
        g.setFont(theme.titleFont().withHeight(32.0f).boldened());
        g.drawText("AIFR3D 2.3", copy.removeFromTop(36), juce::Justification::left);
        g.setColour(theme.textSecondary);
        g.setFont(theme.bodyFont().withHeight(17.0f));
        g.drawFittedText(
            "First-run guide. The halo is now the fast diagnostic read, the candlestick meters show "
            "measured drift over time, and Mix Tips gives metric-specific coaching without leaving the UI.",
            copy.removeFromTop(72), juce::Justification::topLeft, 4);

        auto bullets = copy.removeFromTop(90);
        g.setFont(theme.bodyFont().withHeight(16.0f));
        g.setColour(theme.text);
        g.drawFittedText(
            "1. Halo: top=tone, right=stereo, bottom=loudness, left=dynamics.\n"
            "2. Pulse ring: switch between loudness, dynamics, and stereo motion.\n"
            "3. Mix Signature: candlestick meters for loudness, peak, width, and dynamic range.\n"
            "4. Fix List: live dominant issues. Mix Tips: constant learning tabs per metric.\n"
            "5. Chat: persistent OpenAI session history tied to current DSP metrics.",
            bullets, juce::Justification::topLeft, 6);

        auto imageArea = heroRow.reduced(16, 0).toFloat();
        drawMascotHero(g, m_mascot, imageArea);

        auto cards = content;
        const int gap = 12;
        const int cardWidth = (cards.getWidth() - gap) / 2;
        const int cardHeight = 132;
        drawCard(g, juce::Rectangle<int>(cards.getX(), cards.getY(), cardWidth, cardHeight),
                 "GUI Layers",
                 "Background analyzer = user vs reference curve. Halo = fast diagnostic. Pulse ring = live motion. Candles = tracked session movement.");
        drawCard(g, juce::Rectangle<int>(cards.getX() + cardWidth + gap, cards.getY(), cardWidth, cardHeight),
                 "Issue Flow",
                 "Raw metrics feed one diagnostic layer. Mud, harshness, missing bass, width collapse, and loudness problems now update the fix logic directly.");
        drawCard(g, juce::Rectangle<int>(cards.getX(), cards.getY() + cardHeight + gap, cardWidth, cardHeight),
                 "Mix Tips Tabs",
                 "Open Frequency Balance, Integrated Loudness, Dynamic Range, or Stereo Width and read stable, scrollable guidance without interrupting playback.");
        drawCard(g,
                 juce::Rectangle<int>(cards.getX() + cardWidth + gap, cards.getY() + cardHeight + gap,
                                      cardWidth, cardHeight),
                 "Workflow",
                 "Play audio, inspect the dominant halo zone, confirm it with the candlestick meters, then use Fix List or Mix Tips before making the next move.");
    }

    void resized() override
    {
        auto footer = getLocalBounds().reduced(76, 56).removeFromBottom(42);
        m_closeButton.setBounds(footer.removeFromRight(200));
    }

  private:
    void drawCard(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title,
                  const juce::String& body)
    {
        auto& theme = Theme::instance();
        g.setColour(theme.bg.withAlpha(0.88f));
        g.fillRoundedRectangle(area.toFloat(), theme.cornerRadius * 0.65f);
        g.setColour(theme.panelStroke);
        g.drawRoundedRectangle(area.toFloat(), theme.cornerRadius * 0.65f, 1.0f);

        auto text = area.reduced(14, 12);
        g.setColour(theme.teal);
        g.setFont(theme.bodyFont().boldened().withHeight(15.5f));
        g.drawText(title, text.removeFromTop(20), juce::Justification::left);
        g.setColour(theme.textSecondary);
        g.setFont(theme.bodyFont().withHeight(14.0f));
        g.drawFittedText(body, text, juce::Justification::topLeft, 7);
    }

    std::function<void()> m_onDismiss;
    juce::TextButton m_closeButton;
    juce::Image m_mascot;
};

class StartupSplashOverlay : public juce::Component, private juce::Timer
{
  public:
    explicit StartupSplashOverlay(std::function<void()> onDismiss)
        : m_onDismiss(std::move(onDismiss)), m_mascot(loadStartupMascot())
    {
        startTimerHz(60);
    }

    void paint(juce::Graphics& g) override
    {
        auto& theme = Theme::instance();
        auto bounds = getLocalBounds().toFloat();
        g.fillAll(theme.bg.withAlpha(0.96f));

        juce::ColourGradient bg(theme.panel.brighter(0.08f), bounds.getCentreX(), bounds.getY(),
                                theme.panelAlt.darker(0.22f), bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(bounds.reduced(18.0f), theme.cornerRadius + 10.0f);

        auto content = getLocalBounds().reduced(52, 44);
        auto imageArea = content.removeFromTop(390).toFloat();
        drawMascotHero(g, m_mascot, imageArea.withTrimmedTop(14.0f).withTrimmedBottom(8.0f));

        g.setColour(theme.text);
        g.setFont(theme.titleFont().withHeight(34.0f).boldened());
        g.drawText("AIFR3D 2.3", content.removeFromTop(38), juce::Justification::centred);
        g.setColour(theme.textSecondary);
        g.setFont(theme.bodyFont().withHeight(17.0f));
        g.drawFittedText("Splash loads first. Tutorial follows on first run. Click anywhere to continue.",
                         content.removeFromTop(30), juce::Justification::centred, 2);
        g.drawFittedText("Updated halo, pulse modes, candlestick meters, persistent fix history, and Mix Tips tabs are included in this build.",
                         content.removeFromTop(30), juce::Justification::centred, 2);
    }

    void resized() override
    {
        // No specific resize logic for splash overlay, it covers the whole editor.
    }

  private:
    std::function<void()> m_onDismiss;
    juce::Image m_mascot;
    juce::Atomic<float> m_alpha { 0.0f };
};

//==============================================================================DawAiAudioProcessorEditor::DawAiAudioProcessorEditor(DawAiAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), mainView(p.apvts),
      mainViewV25(std::make_unique<MainView_v25>(p.apvts)),
      haloA(std::make_unique<ApprovalHalo>(p.apvts)),
      haloB(std::make_unique<ApprovalHalo>(p.apvts))
{
    Theme::instance().addChangeListener(this);
    addAndMakeVisible(mainView);
    addAndMakeVisible(*mainViewV25);
    addAndMakeVisible(*haloA);
    addAndMakeVisible(*haloB);

    mainViewV25->setVisible(isV25Mode);
    haloA->setVisible(isCompareMode);
    haloB->setVisible(isCompareMode);

    setSize(loadPersistedPluginBounds().getWidth(), loadPersistedPluginBounds().getHeight());
    setResizable(true, true);
    setResizeLimits(1180, 720, 1920, 1080);

    showStartupSplash();
    maybeShowFirstRunTutorial();

    applyTheme();
    updateHaloLayout();
    connectWebSocket(); // Plain English Summary: Connects to the backend WebSocket for real-time DSP data streaming.
}
DawAiAudioProcessorEditor::~DawAiAudioProcessorEditor()
{
    Theme::instance().removeChangeListener(this);
    savePersistedPluginBounds(getLocalBounds());
    disconnectWebSocket(); // Plain English Summary: Disconnects the WebSocket when the editor is closed.
}

void DawAiAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto& theme = Theme::instance();
    g.fillAll(theme.bg);
}

void DawAiAudioProcessorEditor::resized()
{
    if (splashOverlay)
    {
        splashOverlay->setBounds(getLocalBounds());
    }
    if (tutorialOverlay)
    {
        tutorialOverlay->setBounds(getLocalBounds());
    }

    savePersistedPluginBounds(getLocalBounds());

    updateHaloLayout();
}

void DawAiAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &Theme::instance())
    {
        applyTheme();
    }
      // Plain English Summary: This section handles incoming DSP analysis frames from the audio processor.
    // It updates the UI elements (Halo, Candlestick meters) with the latest audio metrics and sends them to the backend.
    while (processor.uiFifo.getNumAvailable() > 0)
    {
        aifred::dsp::AnalysisFrame frame;
        if (processor.uiFifo.pop(frame))
        {
            // Update UI components with frame data
            mainView.update(frame);
            mainViewV25->update(frame);
            haloA->update(frame);
            haloB->update(frame);

            // Send DSP metrics over WebSocket to the backend for AI processing
            sendDspMetrics(frame);
        }
    }
}

void DawAiAudioProcessorEditor::updateHaloLayout()
{
    if (isCompareMode)
    {
        // Layout for compare mode (side-by-side halos)
        auto bounds = getLocalBounds().reduced(20);
        auto halfWidth = bounds.getWidth() / 2;
        haloA->setBounds(bounds.removeFromLeft(halfWidth).reduced(10));
        haloB->setBounds(bounds.reduced(10));
        mainView.setVisible(false);
        mainViewV25->setVisible(false);
        haloA->setVisible(true);
        haloB->setVisible(true);
    }
    else if (isV25Mode)
    {
        // Layout for V2.2.5 mode
        mainViewV25->setBounds(getLocalBounds());
        mainView.setVisible(false);
        mainViewV25->setVisible(true);
        haloA->setVisible(false);
        haloB->setVisible(false);
    }
    else
    {
        // Layout for V2.2.4 mode
        mainView.setBounds(getLocalBounds());
        mainView.setVisible(true);
        mainViewV25->setVisible(false);
        haloA->setVisible(false);
        haloB->setVisible(false);
    }
}

void DawAiAudioProcessorEditor::setCompareMode(bool compareMode)
{
    isCompareMode = compareMode;
    updateHaloLayout();
}

void DawAiAudioProcessorEditor::showStartupSplash()
{
    splashOverlay = std::make_unique<StartupSplashOverlay>([this]
    {
        splashOverlay = nullptr;
        resized();
    });
    addAndMakeVisible(*splashOverlay);
}

void DawAiAudioProcessorEditor::maybeShowFirstRunTutorial()
{
    if (std::filesystem::exists(tutorialFlagPath()))
    {
        return;
    }

    tutorialOverlay = std::make_unique<FirstRunTutorialOverlay>([this]
    {
        tutorialOverlay = nullptr;
        writeTutorialSeenFlag(tutorialFlagPath());
        resized();
    });
    addAndMakeVisible(*tutorialOverlay);
}

std::filesystem::path DawAiAudioProcessorEditor::tutorialFlagPath()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("AIFR3D")
        .getChildFile("tutorial_seen.flag")
        .getFullPathName().toStdString();
}

void DawAiAudioProcessorEditor::applyTheme()
{
    auto& theme = Theme::instance();
    // Apply theme to mainView
    mainView.applyTheme(theme);
    mainViewV25->applyTheme(theme);
    haloA->applyTheme(theme);
    haloB->applyTheme(theme);

    // Set background color of the editor
    if (auto* peer = getPeer())
    {
        peer->setBackgroundColour(theme.bg);
    }
}

// Plain English Summary: Establishes a WebSocket connection to the backend server.
// This connection is used to send real-time DSP analysis data for AI feedback.
void DawAiAudioProcessorEditor::connectWebSocket()
{
    // TODO: Get WebSocket URL from a configuration or processor parameter
    // For now, hardcode to the local backend. In a real scenario, this would be dynamic.
    juce::URL wsUrl("ws://localhost:3000/ws/vst"); 

    webSocket = std::make_unique<juce::WebSockets::WebSocket>();
    if (webSocket->connect(wsUrl))
    {
        DBG("WebSocket connected successfully!");
    }
    else
    {
        DBG("WebSocket connection failed!");
    }
}

// Plain English Summary: Closes the active WebSocket connection.
void DawAiAudioProcessorEditor::disconnectWebSocket()
{
    if (webSocket && webSocket->isConnected())
    {
        webSocket->disconnect();
        DBG("WebSocket disconnected.");
    }
    webSocket = nullptr;
}

// Plain English Summary: Sends DSP analysis metrics to the backend via WebSocket.
// This data is used by the Aifred AI Brain for context-aware feedback.
void DawAiAudioProcessorEditor::sendDspMetrics(const aifred::dsp::AnalysisFrame& frame)
{
    if (webSocket && webSocket->isConnected())
    {
        juce::DynamicObject::Ptr dspData = new juce::DynamicObject();
        dspData->setProperty("loudness", frame.loudness);
        dspData->setProperty("dynamics", frame.dynamics);
        dspData->setProperty("tone", frame.tone);
        dspData->setProperty("stereo", frame.stereo);
        dspData->setProperty("transient", frame.transient);
        dspData->setProperty("bpm", frame.bpm);
        dspData->setProperty("summary", frame.summary);

        juce::var message = new juce::DynamicObject();
        message->setProperty("type", "DSP_UPDATE");
        message->setProperty("metrics", dspData);

        webSocket->sendTextMessage(juce::JSON::toString(message));
    }
}

// Plain English Summary: Establishes a WebSocket connection to the backend server.
// This connection is used to send real-time DSP analysis data for AI feedback.
void DawAiAudioProcessorEditor::connectWebSocket()
{
    // TODO: Get WebSocket URL from a configuration or processor parameter
    // For now, hardcode to the local backend. In a real scenario, this would be dynamic.
    juce::URL wsUrl("ws://localhost:3000/ws/vst"); 

    webSocket = std::make_unique<juce::WebSockets::WebSocket>();
    if (webSocket->connect(wsUrl))
    {
        DBG("WebSocket connected successfully!");
    }
    else
    {
        DBG("WebSocket connection failed!");
    }
}

// Plain English Summary: Closes the active WebSocket connection.
void DawAiAudioProcessorEditor::disconnectWebSocket()
{
    if (webSocket && webSocket->isConnected())
    {
        webSocket->disconnect();
        DBG("WebSocket disconnected.");
    }
    webSocket = nullptr;
}

// Plain English Summary: Sends DSP analysis metrics to the backend via WebSocket.
// This data is used by the Aifred AI Brain for context-aware feedback.
void DawAiAudioProcessorEditor::sendDspMetrics(const aifred::dsp::AnalysisFrame& frame)
{
    if (webSocket && webSocket->isConnected())
    {
        juce::DynamicObject::Ptr dspData = new juce::DynamicObject();
        dspData->setProperty("loudness", frame.loudness);
        dspData->setProperty("dynamics", frame.dynamics);
        dspData->setProperty("tone", frame.tone);
        dspData->setProperty("stereo", frame.stereo);
        dspData->setProperty("transient", frame.transient);
        dspData->setProperty("bpm", frame.bpm);
        dspData->setProperty("summary", frame.summary);

        juce::var message = new juce::DynamicObject();
        message->setProperty("type", "DSP_UPDATE");
        message->setProperty("metrics", dspData);

        webSocket->sendTextMessage(juce::JSON::toString(message));
    }
}
