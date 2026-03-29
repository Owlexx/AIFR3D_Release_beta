#include "MainView.h"

#include "../Plugin/Theme.h"
#include "dawai/advisory_layer/api_key_store.hpp"
#include "dawai/advisory_layer/local_brain_adviser.hpp"
#include "dawai/aifr3d_core/system_contract.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <cmath>
#include <array>
#include <iomanip>
#include <string>

namespace
{
constexpr auto kMissingKeyFeedback = "OpenAI API key missing — AIFR3D chat unavailable";
constexpr auto kOfflineFeedback = "OpenAI request failed — AIFR3D chat unavailable";
constexpr int kUiRefreshHz = 60;
constexpr auto kLockedChatModel = "gpt-5.2";

juce::File pluginLayoutStateFile()
{
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("AIFR3D")
        .getChildFile("plugin_layout_variant_2_3.txt");
}

int loadPersistedLayoutVariant(int fallbackVariant)
{
    const auto file = pluginLayoutStateFile();
    if (!file.existsAsFile())
    {
        return juce::jlimit(1, 4, fallbackVariant);
    }
    const int parsed = file.loadFileAsString().trim().getIntValue();
    return juce::jlimit(1, 4, parsed > 0 ? parsed : fallbackVariant);
}

void savePersistedLayoutVariant(int variant)
{
    const auto file = pluginLayoutStateFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(juce::String(juce::jlimit(1, 4, variant)));
}

int topBarHeightForWidth(int width)
{
    return width < 1180 ? 92 : 64;
}

struct FixedControl
{
    juce::Component* component = nullptr;
    int width = 0;
};

juce::String advisoryUnavailableMessage()
{
    return dawai::advisory_layer::ApiKeyStore::loadOpenAiKey().empty()
               ? juce::String(kMissingKeyFeedback)
               : juce::String(kOfflineFeedback);
}

std::string genreFromIndex(uint8_t index)
{
    switch (index)
    {
    case 1:
        return "Pop";
    case 2:
        return "EDM";
    case 3:
        return "Hip-Hop";
    case 4:
        return "Rap";
    case 5:
        return "Dubstep";
    case 6:
        return "Rock";
    default:
        return "Unknown";
    }
}

std::vector<std::string> loadChatModelOptions()
{
    return {kLockedChatModel};
}

int uiVariant()
{
#if AIFRED_UI_VARIANT == 2
    return 2;
#elif AIFRED_UI_VARIANT == 3
    return 3;
#elif AIFRED_UI_VARIANT == 4
    return 4;
#else
    return 1;
#endif
}

bool smoothedMoving(const juce::SmoothedValue<float>& value, float epsilon = 0.001f)
{
    return std::abs(value.getTargetValue() - value.getCurrentValue()) > epsilon;
}

bool uiAnimationActive(const UiModel& model)
{
    return model.visualPulse > 0.02f || smoothedMoving(model.behaviorIndex) ||
           smoothedMoving(model.mixAlignment) || smoothedMoving(model.signalClarity) ||
           smoothedMoving(model.signalStability) || smoothedMoving(model.userMixSignature) ||
           smoothedMoving(model.refMixSignature) || smoothedMoving(model.tone) ||
           smoothedMoving(model.dynamics) || smoothedMoving(model.space) ||
           smoothedMoving(model.punch) || smoothedMoving(model.balance) ||
           smoothedMoving(model.midEnergy) || smoothedMoving(model.sideEnergy) ||
           smoothedMoving(model.correlation01) || smoothedMoving(model.subMono) ||
           smoothedMoving(model.phaseRisk);
}

bool advisoryReady(const AnalysisFrame& frame)
{
    return frame.analysisStateCode == kAnalysisStateValidScored && frame.scoredResultValid != 0;
}

int pinnedGenreIndexFromSelection(int selectedId)
{
    switch (selectedId)
    {
    case 2:
        return 1;
    case 3:
        return 2;
    case 4:
        return 3;
    case 5:
        return 4;
    case 6:
        return 5;
    case 7:
        return 6;
    default:
        return 0;
    }
}

int layoutWrappedControls(juce::Rectangle<int> area, std::initializer_list<FixedControl> controls,
                         juce::Component* flexible = nullptr, int flexibleMinWidth = 160,
                         int rowHeight = 28, int gap = 6)
{
    if (area.isEmpty())
    {
        for (const auto& control : controls)
        {
            if (control.component != nullptr)
            {
                control.component->setBounds({});
            }
        }
        if (flexible != nullptr)
        {
            flexible->setBounds({});
        }
        return 0;
    }

    int x = area.getX();
    int y = area.getY();
    const auto nextRow = [&]
    {
        x = area.getX();
        y += rowHeight + gap;
    };

    for (const auto& control : controls)
    {
        if (control.component == nullptr)
        {
            continue;
        }

        const int width = juce::jlimit(0, area.getWidth(), control.width);
        if (x != area.getX() && x + width > area.getRight())
        {
            nextRow();
        }

        control.component->setBounds(x, y, width, rowHeight);
        x += width + gap;
    }

    if (flexible != nullptr)
    {
        int remaining = area.getRight() - x;
        if (remaining < flexibleMinWidth && x != area.getX())
        {
            nextRow();
            remaining = area.getWidth();
        }
        flexible->setBounds(x, y, juce::jmax(0, remaining), rowHeight);
    }

    return (y - area.getY()) + rowHeight;
}

int layoutApiControls(juce::Rectangle<int> area, juce::TextEditor& apiKeyEntry,
                      juce::Button& apiKeySaveButton, juce::Button& apiKeyClearButton)
{
    constexpr int gap = 6;
    constexpr int rowHeight = 28;

    if (area.isEmpty())
    {
        apiKeyEntry.setBounds({});
        apiKeySaveButton.setBounds({});
        apiKeyClearButton.setBounds({});
        return 0;
    }

    if (area.getWidth() >= 480)
    {
        auto row = area.removeFromTop(rowHeight);
        const int trailingWidth = 118 + gap + 82;
        apiKeyEntry.setBounds(
            row.removeFromLeft(juce::jmax(220, row.getWidth() - trailingWidth)).reduced(1));
        row.removeFromLeft(gap);
        apiKeySaveButton.setBounds(row.removeFromLeft(118));
        row.removeFromLeft(gap);
        apiKeyClearButton.setBounds(row.removeFromLeft(82));
        return rowHeight;
    }

    apiKeyEntry.setBounds(area.removeFromTop(rowHeight).reduced(1));
    area.removeFromTop(gap);
    auto buttons = area.removeFromTop(rowHeight);
    apiKeySaveButton.setBounds(buttons.removeFromLeft(118));
    buttons.removeFromLeft(gap);
    apiKeyClearButton.setBounds(buttons.removeFromLeft(82));
    return rowHeight * 2 + gap;
}

void layoutChatPanel(juce::Rectangle<int> area, juce::TextEditor& apiKeyEntry,
                     juce::Button& apiKeySaveButton, juce::Button& apiKeyClearButton,
                     juce::TextEditor& chatLog, juce::TextEditor& chatPrompt,
                     juce::ComboBox& pulseModeBox, juce::Button& clearFixListButton,
                     juce::Button& chatAttachButton, juce::ToggleButton& debugToggle,
                     juce::Button& chatAskButton, juce::Label& chatStatus)
{
    auto inner = area.reduced(8, 12);
    inner.removeFromTop(20);

    const int apiHeight = inner.getWidth() >= 480 ? 30 : 64;
    layoutApiControls(inner.removeFromTop(apiHeight), apiKeyEntry, apiKeySaveButton,
                      apiKeyClearButton);

    inner.removeFromTop(6);
    auto promptArea = inner.removeFromBottom(58);
    const int controlsHeight = inner.getWidth() >= 640 ? 28 : (inner.getWidth() >= 420 ? 62 : 96);
    layoutWrappedControls(inner.removeFromBottom(controlsHeight),
                          {{&pulseModeBox, 156},
                           {&clearFixListButton, 106},
                           {&chatAttachButton, 118},
                           {&debugToggle, 74},
                           {&chatAskButton, 92}},
                          &chatStatus, 180);

    chatLog.setBounds(inner.reduced(2));
    chatPrompt.setBounds(promptArea.reduced(2));
}

} // namespace

MainView::MainView(DawAiAudioProcessor& p)
    : processor(p), advisory(dawai::advisory_layer::LocalBrainAdviser::createFromEnvironment())
{
    activeLayoutVariant = loadPersistedLayoutVariant(uiVariant());
    addAndMakeVisible(topBar);
    addAndMakeVisible(mixSignatureMeter);
    addAndMakeVisible(approvalHalo);
    addAndMakeVisible(candleStory);
    addAndMakeVisible(stereoArc);
    addAndMakeVisible(fixList);
    addAndMakeVisible(mixTipsPanel);
    addAndMakeVisible(chatLog);
    addAndMakeVisible(chatPrompt);
    addAndMakeVisible(apiKeyEntry);
    addAndMakeVisible(chatAskButton);
    addAndMakeVisible(chatAttachButton);
    addAndMakeVisible(apiKeySaveButton);
    addAndMakeVisible(apiKeyClearButton);
    addAndMakeVisible(fixListPaneButton);
    addAndMakeVisible(mixTipsPaneButton);
    addAndMakeVisible(clearFixListButton);
    addAndMakeVisible(debugToggle);
    addAndMakeVisible(pulseModeBox);
    addAndMakeVisible(chatStatus);
    addAndMakeVisible(referenceMenuLabel);
    addAndMakeVisible(referenceGenreBox);
    addAndMakeVisible(referencePinButton);
    addAndMakeVisible(compareMenuLabel);
    addAndMakeVisible(comparePremixButton);
    addAndMakeVisible(compareMixButton);
    addAndMakeVisible(compareRunButton);
    addAndMakeVisible(compareMenuStatus);

    topBar.onModeChanged = [this](TopBar::Mode mode)
    {
        switch (mode)
        {
        case TopBar::Mode::Compare:
            activeModePane = ModePane::Compare;
            model.setPresentationMode(UiPresentationMode::Compare);
            break;
        case TopBar::Mode::Reference:
            activeModePane = ModePane::Reference;
            model.setPresentationMode(UiPresentationMode::Reference);
            break;
        case TopBar::Mode::Analyze:
        default:
            activeModePane = ModePane::Analyze;
            model.setPresentationMode(UiPresentationMode::Analyze);
            break;
        }
        applyModePaneVisibility();
        resized();
        repaint();
    };
    topBar.onLayoutVariantChanged = [this](int variant)
    {
        activeLayoutVariant = juce::jlimit(1, 4, variant);
        savePersistedLayoutVariant(activeLayoutVariant);
        resized();
        repaint();
    };
    topBar.setLayoutVariant(activeLayoutVariant);

    chatLog.setMultiLine(true);
    chatLog.setReadOnly(true);
    chatLog.setScrollbarsShown(true);
    chatLog.setCaretVisible(false);
    chatLog.setColour(juce::TextEditor::backgroundColourId, Theme::instance().panelAlt.brighter(0.03f));
    chatLog.setColour(juce::TextEditor::outlineColourId, Theme::instance().panelStroke);
    chatLog.setColour(juce::TextEditor::textColourId, Theme::instance().textSecondary);
    chatLog.applyFontToAllText(Theme::instance().bodyFont().withHeight(15.0f));

    chatPrompt.setMultiLine(true);
    chatPrompt.setScrollbarsShown(true);
    chatPrompt.setReturnKeyStartsNewLine(false);
    chatPrompt.setColour(juce::TextEditor::backgroundColourId, Theme::instance().panelAlt);
    chatPrompt.setColour(juce::TextEditor::outlineColourId, Theme::instance().panelStroke);
    chatPrompt.setColour(juce::TextEditor::focusedOutlineColourId, Theme::instance().cyan);
    chatPrompt.setColour(juce::TextEditor::textColourId, Theme::instance().text);
    chatPrompt.setTextToShowWhenEmpty("Ask AIFR3D... (session history only)",
                                      Theme::instance().text.withAlpha(0.5f));
    chatPrompt.applyFontToAllText(Theme::instance().bodyFont().withHeight(15.0f));

    apiKeyEntry.setMultiLine(false);
    apiKeyEntry.setPasswordCharacter('*');
    apiKeyEntry.setColour(juce::TextEditor::backgroundColourId, Theme::instance().panelAlt);
    apiKeyEntry.setColour(juce::TextEditor::outlineColourId, Theme::instance().panelStroke);
    apiKeyEntry.setColour(juce::TextEditor::focusedOutlineColourId, Theme::instance().cyan);
    apiKeyEntry.setColour(juce::TextEditor::textColourId, Theme::instance().text);
    apiKeyEntry.setTextToShowWhenEmpty("OpenAI API key file: /home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt",
                                       Theme::instance().text.withAlpha(0.5f));
    apiKeyEntry.applyFontToAllText(Theme::instance().bodyFont().withHeight(14.0f));
    if (!dawai::advisory_layer::ApiKeyStore::loadOpenAiKey().empty())
    {
        chatStatus.setText("OpenAI API key loaded from local file. Model locked to GPT-5.2.",
                           juce::dontSendNotification);
    }

    apiKeySaveButton.onClick = [this]
    {
        const auto key = apiKeyEntry.getText().trim().toStdString();
        if (key.empty())
        {
            chatStatus.setText("Paste an OpenAI API key, then press Link API Key.",
                               juce::dontSendNotification);
            return;
        }

        if (dawai::advisory_layer::ApiKeyStore::saveOpenAiKey(key))
        {
            apiKeyEntry.clear();
            chatStatus.setText("OpenAI API key saved locally. Model locked to GPT-5.2.",
                               juce::dontSendNotification);
            return;
        }

        chatStatus.setText("Failed to save OpenAI API key locally.", juce::dontSendNotification);
    };

    apiKeyClearButton.onClick = [this]
    {
        (void)dawai::advisory_layer::ApiKeyStore::clearOpenAiKey();
        apiKeyEntry.clear();
        chatStatus.setText("Local OpenAI API key cleared.", juce::dontSendNotification);
    };
    clearFixListButton.onClick = [this]
    {
        model.fixList.clear();
        model.diagnosticFixList.clear();
        fixList.setModel(model);
        repaint();
    };
    fixListPaneButton.onClick = [this]
    {
        activeInsightPane = InsightPane::FixList;
        applyInsightPaneVisibility();
        resized();
    };
    mixTipsPaneButton.onClick = [this]
    {
        activeInsightPane = InsightPane::MixTips;
        applyInsightPaneVisibility();
        resized();
    };
    fixListPaneButton.setClickingTogglesState(true);
    mixTipsPaneButton.setClickingTogglesState(true);
    const auto modelOptions = loadChatModelOptions();
    int itemId = 1;
    for (const auto& modelName : modelOptions)
    {
        chatModelBox.addItem(modelName, itemId++);
    }
    chatModelBox.setSelectedId(1, juce::dontSendNotification);
    chatModelBox.setTooltip("Model locked to GPT-5.2.");
    chatModelBox.setEnabled(false);
    chatModelBox.setVisible(false);
    pulseModeBox.addItem("Pulse: Loudness", static_cast<int>(ApprovalHalo::PulseMode::Loudness));
    pulseModeBox.addItem("Pulse: Dynamics", static_cast<int>(ApprovalHalo::PulseMode::Dynamics));
    pulseModeBox.addItem("Pulse: Stereo", static_cast<int>(ApprovalHalo::PulseMode::Stereo));
    pulseModeBox.setSelectedId(static_cast<int>(ApprovalHalo::PulseMode::Loudness),
                               juce::dontSendNotification);
    pulseModeBox.setTooltip("Choose which live diagnostic layer the outer pulse ring emphasizes.");
    pulseModeBox.onChange = [this]
    {
        approvalHalo.setPulseMode(static_cast<ApprovalHalo::PulseMode>(
            juce::jlimit(1, 3, pulseModeBox.getSelectedId())));
    };
    approvalHalo.setPulseMode(ApprovalHalo::PulseMode::Loudness);
    chatAskButton.onClick = [this]
    {
        if (!hasLastFrame)
        {
            return;
        }
        if (advisoryInFlight.exchange(true))
        {
            chatStatus.setText("AI is still processing previous request...",
                               juce::dontSendNotification);
            return;
        }
        if (chatPrompt.getText().isNotEmpty())
        {
            appendChatHistoryEntry("You: " + chatPrompt.getText());
        }
        chatStatus.setText("Asking OpenAI GPT-5.2 advisory...", juce::dontSendNotification);
        lastAutoAdvisorySec = lastFrame.tSec;
        lastAutoAdvisoryFlags = lastFrame.flags;
        triggerAdvisory(lastFrame, chatPrompt.getText(), chatAttachments, true);
    };
    chatAttachButton.onClick = [this]
    {
        juce::FileChooser chooser("Attach context file", juce::File(), "*");
        if (chooser.browseForFileToOpen())
        {
            chatAttachments.push_back(
                std::filesystem::path(chooser.getResult().getFullPathName().toStdString()));
            chatStatus.setText("Attached: " + chooser.getResult().getFileName(),
                               juce::dontSendNotification);
        }
    };
    debugToggle.setTooltip("Capture bounded analysis-frame debug log");
    debugToggle.onClick = [this]
    {
        debugLoggingEnabled = debugToggle.getToggleState();
        if (debugLoggingEnabled)
        {
            debugFrameWrite = 0;
            debugFrameCount = 0;
            debugFlushTick = 0;
            chatStatus.setText("Debug logging enabled: writing bounded frame log",
                               juce::dontSendNotification);
            return;
        }

        flushDebugLog(true);
        chatStatus.setText("Debug logging disabled", juce::dontSendNotification);
    };
    if (chatStatus.getText().isEmpty())
    {
        if (dawai::advisory_layer::ApiKeyStore::loadOpenAiKey().empty())
        {
            chatStatus.setText(kMissingKeyFeedback, juce::dontSendNotification);
        }
        else
        {
            chatStatus.setText({}, juce::dontSendNotification);
        }
    }
    chatStatus.setColour(juce::Label::textColourId, Theme::instance().textSecondary);
    chatStatus.setJustificationType(juce::Justification::centredLeft);

    referenceMenuLabel.setText(
        "REFERENCE MENU\nPick a target genre profile and pin it before comparing edits.",
        juce::dontSendNotification);
    referenceMenuLabel.setJustificationType(juce::Justification::topLeft);
    referenceMenuLabel.setColour(juce::Label::textColourId, Theme::instance().textSecondary);

    referenceGenreBox.addItem("Auto (detected)", 1);
    referenceGenreBox.addItem("Pop", 2);
    referenceGenreBox.addItem("EDM", 3);
    referenceGenreBox.addItem("Hip-Hop", 4);
    referenceGenreBox.addItem("Rap", 5);
    referenceGenreBox.addItem("Dubstep", 6);
    referenceGenreBox.addItem("Rock", 7);
    referenceGenreBox.setSelectedId(1, juce::dontSendNotification);
    referenceGenreBox.setTooltip("Reference genre target");
    referenceGenreBox.onChange = [this]
    {
        processor.setPinnedReferenceGenreIndex(
            pinnedGenreIndexFromSelection(referenceGenreBox.getSelectedId()));
    };

    referencePinButton.onClick = [this]
    {
        processor.setPinnedReferenceGenreIndex(
            pinnedGenreIndexFromSelection(referenceGenreBox.getSelectedId()));
        chatStatus.setText("Reference target pinned: " + referenceGenreBox.getText(),
                           juce::dontSendNotification);
    };

    compareMenuLabel.setText(
        "COMPARE MENU\nCapture Mix A, then capture Mix B or compare Mix A against the live buffer.",
        juce::dontSendNotification);
    compareMenuLabel.setJustificationType(juce::Justification::topLeft);
    compareMenuLabel.setColour(juce::Label::textColourId, Theme::instance().textSecondary);
    comparePremixButton.setButtonText("Capture Mix A");
    compareMixButton.setButtonText("Capture Mix B");
    compareRunButton.setButtonText("Use Live B");

    comparePremixButton.onClick = [this]
    {
        if (model.captureCompareSnapshotA("Mix A"))
        {
            compareMenuStatus.setText("Mix A captured from the current live buffer.",
                                      juce::dontSendNotification);
            return;
        }

        compareMenuStatus.setText("Mix A capture needs valid live program material.",
                                  juce::dontSendNotification);
    };
    compareMixButton.onClick = [this]
    {
        if (model.captureCompareSnapshotB("Mix B"))
        {
            compareMenuStatus.setText("Mix B captured from the current live buffer.",
                                      juce::dontSendNotification);
            return;
        }

        compareMenuStatus.setText("Mix B capture needs valid live program material.",
                                  juce::dontSendNotification);
    };
    compareRunButton.onClick = [this]
    {
        model.compareMixB = {};
        if (model.compareMixA.valid)
        {
            compareMenuStatus.setText("Compare mode now tracks Mix A against the live buffer.",
                                      juce::dontSendNotification);
            return;
        }

        compareMenuStatus.setText("Capture Mix A first to compare against the live buffer.",
                                  juce::dontSendNotification);
    };
    compareMenuStatus.setText("Capture Mix A or Mix B from the live analyzer.", juce::dontSendNotification);
    compareMenuStatus.setJustificationType(juce::Justification::centredLeft);
    compareMenuStatus.setColour(juce::Label::textColourId, Theme::instance().textSecondary);

    applyModePaneVisibility();
    applyInsightPaneVisibility();

    model.prepare(kUiRefreshHz);
    model.setPresentationMode(UiPresentationMode::Analyze);
    startTimerHz(kUiRefreshHz);
}

void MainView::appendChatHistoryEntry(const juce::String& entry)
{
    const auto trimmed = entry.trim();
    if (trimmed.isEmpty())
    {
        return;
    }

    chatHistoryEntries.push_back(trimmed);
    if (trimmed.startsWith("AIFR3D"))
    {
        rememberInsightEntry(trimmed);
    }

    juce::String combined;
    for (std::size_t i = 0; i < chatHistoryEntries.size(); ++i)
    {
        combined << chatHistoryEntries[i];
        if (i + 1 < chatHistoryEntries.size())
        {
            combined << "\n\n";
        }
    }
    chatLog.setText(combined, juce::dontSendNotification);
    chatLog.moveCaretToEnd();
}

void MainView::rememberInsightEntry(const juce::String& entry)
{
    const auto trimmed = entry.trim();
    if (trimmed.isEmpty())
    {
        return;
    }

    recentInsightMemory.push_back(trimmed);
    while (recentInsightMemory.size() > 7)
    {
        recentInsightMemory.pop_front();
    }
}

void MainView::paint(juce::Graphics& g)
{
    auto& theme = Theme::instance();
    auto panel = getLocalBounds().toFloat();

    juce::ColourGradient bg(theme.panel.brighter(0.05f), panel.getX(), panel.getY(),
                            theme.panelAlt.darker(0.18f), panel.getX(), panel.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(panel, theme.cornerRadius);
    g.setColour(theme.panelStroke);
    g.drawRoundedRectangle(panel.reduced(1.0f), theme.cornerRadius, 1.0f);

    if (!chatPanelBounds.isEmpty())
    {
        auto chatBounds = chatPanelBounds.toFloat();
        g.setColour(theme.panelAlt.withAlpha(0.82f));
        g.fillRoundedRectangle(chatBounds, theme.cornerRadius * 0.55f);
        g.setColour(theme.panelStroke.brighter(0.3f));
        g.drawRoundedRectangle(chatBounds, theme.cornerRadius * 0.55f, 1.0f);

        auto labelArea = chatPanelBounds;
        labelArea.removeFromTop(18);
        g.setFont(theme.bodyFont().boldened().withHeight(12.0f));
        g.setColour(theme.textSecondary);
        g.drawFittedText("INSIGHT CHAT", labelArea.reduced(8, 0), juce::Justification::centredLeft,
                         1);
    }
}

void MainView::applyModePaneVisibility()
{
    const bool showReference = (activeModePane == ModePane::Reference);
    const bool showCompare = (activeModePane == ModePane::Compare);

    referenceMenuLabel.setVisible(showReference);
    referenceGenreBox.setVisible(showReference);
    referencePinButton.setVisible(showReference);

    compareMenuLabel.setVisible(showCompare);
    comparePremixButton.setVisible(showCompare);
    compareMixButton.setVisible(showCompare);
    compareRunButton.setVisible(showCompare);
    compareMenuStatus.setVisible(showCompare);
}

void MainView::applyInsightPaneVisibility()
{
    const bool showFixList = (activeInsightPane == InsightPane::FixList);
    fixList.setVisible(showFixList);
    mixTipsPanel.setVisible(!showFixList);
    fixListPaneButton.setToggleState(showFixList, juce::dontSendNotification);
    mixTipsPaneButton.setToggleState(!showFixList, juce::dontSendNotification);
}

void MainView::layoutModeMenus(juce::Rectangle<int> area)
{
    if (activeModePane == ModePane::Analyze || area.isEmpty())
    {
        referenceMenuLabel.setBounds({});
        referenceGenreBox.setBounds({});
        referencePinButton.setBounds({});
        compareMenuLabel.setBounds({});
        comparePremixButton.setBounds({});
        compareMixButton.setBounds({});
        compareRunButton.setBounds({});
        compareMenuStatus.setBounds({});
        return;
    }

    auto panel = area.reduced(6);
    panel.removeFromTop(4);

    if (activeModePane == ModePane::Reference)
    {
        referenceMenuLabel.setBounds(panel.removeFromTop(36));
        const int controlsHeight = panel.getWidth() >= 340 ? 28 : 62;
        layoutWrappedControls(panel.removeFromTop(controlsHeight),
                              {{&referenceGenreBox, 220}, {&referencePinButton, 120}});
        return;
    }

    if (activeModePane == ModePane::Compare)
    {
        compareMenuLabel.setBounds(panel.removeFromTop(34));
        const int controlsHeight = panel.getWidth() >= 360 ? 28 : 62;
        layoutWrappedControls(panel.removeFromTop(controlsHeight),
                              {{&comparePremixButton, 120},
                               {&compareMixButton, 112},
                               {&compareRunButton, 112}});
        panel.removeFromTop(4);
        compareMenuStatus.setBounds(panel.removeFromTop(24));
    }
}

void MainView::resized()
{
    auto area = getLocalBounds().reduced(16);
    switch (activeLayoutVariant)
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
    auto top = area.removeFromTop(topBarHeightForWidth(area.getWidth()));
    topBar.setBounds(top);

    auto left = area.removeFromLeft(static_cast<int>(area.getWidth() * 0.58f));
    auto right = area;

    auto leftTop = left.removeFromTop(static_cast<int>(left.getHeight() * 0.68f));
    mixSignatureMeter.setBounds(leftTop.reduced(6));
    candleStory.setBounds(left.reduced(6));

    auto rightTop = right.removeFromTop(static_cast<int>(right.getHeight() * 0.52f));
    approvalHalo.setBounds(
        rightTop.removeFromLeft(static_cast<int>(rightTop.getWidth() * 0.58f)).reduced(6));
    stereoArc.setBounds(rightTop.reduced(6));

    auto rightBottom = right.reduced(6);
    const bool showModeMenu = (activeModePane != ModePane::Analyze);
    auto modeArea = showModeMenu ? rightBottom.removeFromTop(120) : juce::Rectangle<int>{};
    layoutModeMenus(modeArea);

    auto insightTabs = rightBottom.removeFromTop(28);
    fixListPaneButton.setBounds(insightTabs.removeFromLeft(94));
    insightTabs.removeFromLeft(6);
    mixTipsPaneButton.setBounds(insightTabs.removeFromLeft(94));
    rightBottom.removeFromTop(6);

    auto fixesArea = rightBottom.removeFromTop(static_cast<int>(rightBottom.getHeight() * 0.54f));
    fixList.setBounds(fixesArea);
    mixTipsPanel.setBounds(fixesArea);

    auto chatArea = rightBottom;
    chatPanelBounds = chatArea;
    layoutChatPanel(chatArea, apiKeyEntry, apiKeySaveButton, apiKeyClearButton, chatLog,
                    chatPrompt, pulseModeBox, clearFixListButton, chatAttachButton, debugToggle,
                    chatAskButton, chatStatus);
    chatModelBox.setBounds({});
}

void MainView::layoutVariantB(juce::Rectangle<int> area)
{
    auto top = area.removeFromTop(topBarHeightForWidth(area.getWidth()));
    topBar.setBounds(top);

    auto center = area.removeFromTop(static_cast<int>(area.getHeight() * 0.56f));
    auto centerLeft = center.removeFromLeft(static_cast<int>(center.getWidth() * 0.37f));
    auto centerMid = center.removeFromLeft(static_cast<int>(center.getWidth() * 0.52f));
    auto centerRight = center;

    mixSignatureMeter.setBounds(centerLeft.reduced(6));
    approvalHalo.setBounds(centerMid.reduced(6));
    stereoArc.setBounds(centerRight.reduced(6));

    auto lower = area;
    auto lowerLeft = lower.removeFromLeft(static_cast<int>(lower.getWidth() * 0.44f));
    candleStory.setBounds(lowerLeft.reduced(6));

    auto lowerRight = lower.reduced(6);
    const bool showModeMenu = (activeModePane != ModePane::Analyze);
    auto modeArea = showModeMenu ? lowerRight.removeFromTop(120) : juce::Rectangle<int>{};
    layoutModeMenus(modeArea);

    auto insightTabs = lowerRight.removeFromTop(28);
    fixListPaneButton.setBounds(insightTabs.removeFromLeft(94));
    insightTabs.removeFromLeft(6);
    mixTipsPaneButton.setBounds(insightTabs.removeFromLeft(94));
    lowerRight.removeFromTop(6);

    auto chatArea = lowerRight.removeFromBottom(static_cast<int>(lowerRight.getHeight() * 0.58f));
    fixList.setBounds(lowerRight);
    mixTipsPanel.setBounds(lowerRight);

    chatPanelBounds = chatArea;
    layoutChatPanel(chatArea, apiKeyEntry, apiKeySaveButton, apiKeyClearButton, chatLog,
                    chatPrompt, pulseModeBox, clearFixListButton, chatAttachButton, debugToggle,
                    chatAskButton, chatStatus);
    chatModelBox.setBounds({});
}

void MainView::layoutVariantC(juce::Rectangle<int> area)
{
    auto top = area.removeFromTop(topBarHeightForWidth(area.getWidth()));
    topBar.setBounds(top);

    auto leftRail = area.removeFromLeft(static_cast<int>(area.getWidth() * 0.32f));
    auto rightRail = area.removeFromRight(static_cast<int>(area.getWidth() * 0.30f));
    auto middle = area;

    mixSignatureMeter.setBounds(
        leftRail.removeFromTop(static_cast<int>(leftRail.getHeight() * 0.60f)).reduced(6));

    approvalHalo.setBounds(middle.removeFromTop(static_cast<int>(middle.getHeight() * 0.56f)).reduced(6));
    candleStory.setBounds(middle.reduced(6));

    stereoArc.setBounds(
        rightRail.removeFromTop(static_cast<int>(rightRail.getHeight() * 0.48f)).reduced(6));
    const bool showModeMenu = (activeModePane != ModePane::Analyze);
    auto modeArea = showModeMenu ? rightRail.removeFromTop(120) : juce::Rectangle<int>{};
    layoutModeMenus(modeArea);
    auto insightTabs = leftRail.removeFromTop(28);
    fixListPaneButton.setBounds(insightTabs.removeFromLeft(94));
    insightTabs.removeFromLeft(6);
    mixTipsPaneButton.setBounds(insightTabs.removeFromLeft(94));
    leftRail.removeFromTop(6);
    auto chatArea = rightRail.reduced(6);
    fixList.setBounds(leftRail.reduced(6));
    mixTipsPanel.setBounds(leftRail.reduced(6));
    chatPanelBounds = chatArea;
    layoutChatPanel(chatArea, apiKeyEntry, apiKeySaveButton, apiKeyClearButton, chatLog,
                    chatPrompt, pulseModeBox, clearFixListButton, chatAttachButton, debugToggle,
                    chatAskButton, chatStatus);
    chatModelBox.setBounds({});
}

void MainView::layoutVariantD(juce::Rectangle<int> area)
{
    auto top = area.removeFromTop(topBarHeightForWidth(area.getWidth()));
    topBar.setBounds(top);

    auto upper = area.removeFromTop(static_cast<int>(area.getHeight() * 0.48f));
    auto upperLeft = upper.removeFromLeft(static_cast<int>(upper.getWidth() * 0.40f));
    auto upperRight = upper;
    mixSignatureMeter.setBounds(upperLeft.reduced(6));
    stereoArc.setBounds(upperRight.reduced(6));

    auto mid = area.removeFromTop(static_cast<int>(area.getHeight() * 0.40f));
    auto midLeft = mid.removeFromLeft(static_cast<int>(mid.getWidth() * 0.48f));
    candleStory.setBounds(midLeft.reduced(6));
    approvalHalo.setBounds(mid.reduced(6));

    auto bottom = area.reduced(6);
    auto bottomLeft = bottom.removeFromLeft(static_cast<int>(bottom.getWidth() * 0.42f));
    auto insightTabs = bottomLeft.removeFromTop(28);
    fixListPaneButton.setBounds(insightTabs.removeFromLeft(94));
    insightTabs.removeFromLeft(6);
    mixTipsPaneButton.setBounds(insightTabs.removeFromLeft(94));
    bottomLeft.removeFromTop(6);
    fixList.setBounds(bottomLeft);
    mixTipsPanel.setBounds(bottomLeft);

    auto chatArea = bottom;
    const bool showModeMenu = (activeModePane != ModePane::Analyze);
    auto modeArea = showModeMenu ? chatArea.removeFromTop(120) : juce::Rectangle<int>{};
    layoutModeMenus(modeArea);
    chatPanelBounds = chatArea;
    layoutChatPanel(chatArea, apiKeyEntry, apiKeySaveButton, apiKeyClearButton, chatLog,
                    chatPrompt, pulseModeBox, clearFixListButton, chatAttachButton, debugToggle,
                    chatAskButton, chatStatus);
    chatModelBox.setBounds({});
}

void MainView::timerCallback()
{
    AnalysisFrame f;
    bool got = false;
    while (processor.uiFifo.pop(f))
        got = true;

    if (got)
    {
        model.updateFrom(f);
        lastFrame = f;
        hasLastFrame = true;
        captureDebugFrame(f);

        juce::String fixSignature;
        for (const auto& card : model.fixList)
        {
            fixSignature << card.id << ":" << card.impact << "|";
        }
        if (fixSignature != lastFixMemorySignature)
        {
            lastFixMemorySignature = fixSignature;
            if (!model.fixList.empty())
            {
                rememberInsightEntry("FIX: " + model.fixList.front().title + " | " +
                                     model.fixList.front().next);
            }
        }

        const double secondsSinceAuto =
            (lastAutoAdvisorySec < 0.0) ? 9999.0 : (lastFrame.tSec - lastAutoAdvisorySec);
        const bool flagsChanged = (lastFrame.flags != lastAutoAdvisoryFlags);
        const bool periodicRefresh = (lastAutoAdvisorySec < 0.0) || (secondsSinceAuto >= 3.0);
        const bool reactiveRefresh = flagsChanged && (secondsSinceAuto >= 1.0);

        if (advisoryReady(lastFrame) && (periodicRefresh || reactiveRefresh) &&
            !advisoryInFlight.exchange(true))
        {
            lastAutoAdvisorySec = lastFrame.tSec;
            lastAutoAdvisoryFlags = lastFrame.flags;
            chatStatus.setText("Live advisory syncing from realtime buffer...",
                               juce::dontSendNotification);
            triggerAdvisory(lastFrame,
                            "Refresh insight from the current realtime audio buffer and DSP metrics.",
                            {}, false);
        }
    }
    flushDebugLog(false);

    model.advance();

    const bool keepAnimating = advisoryInFlight.load() || uiAnimationActive(model);
    if (!got && !keepAnimating)
    {
        return;
    }

    topBar.setModel(model);
    mixSignatureMeter.setModel(model);
    approvalHalo.setModel(model);
    candleStory.setModel(model);
    stereoArc.setModel(model);
    fixList.setModel(model);
    repaint();
}

void MainView::captureDebugFrame(const AnalysisFrame& frame)
{
    if (!debugLoggingEnabled)
    {
        return;
    }

    debugFrameRing[debugFrameWrite] = frame;
    debugFrameWrite = (debugFrameWrite + 1) % debugFrameRing.size();
    if (debugFrameCount < debugFrameRing.size())
    {
        ++debugFrameCount;
    }
}

void MainView::flushDebugLog(bool force)
{
    if (!debugLoggingEnabled && !force)
    {
        return;
    }

    if (!force)
    {
        if (++debugFlushTick < 30)
        {
            return;
        }
        debugFlushTick = 0;
    }

    std::filesystem::create_directories(debugLogPath.parent_path());
    std::ofstream out(debugLogPath, std::ios::trunc);
    if (!out)
    {
        return;
    }

    out << "t_sec,mix_alignment,signal_clarity,behavior_index,integrated_lufs,short_term_lufs,"
           "true_peak_dbtp,ref_true_peak_dbtp,true_peak_vs_reference_db,crest_factor_db,"
           "stereo_width,stereo_correlation,spectral_balance,transient_density,phase_risk,flags\n";

    if (debugFrameCount == 0)
    {
        return;
    }

    const std::size_t oldest = (debugFrameWrite + debugFrameRing.size() - debugFrameCount) %
                               debugFrameRing.size();

    out << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < debugFrameCount; ++i)
    {
        const auto& frame = debugFrameRing[(oldest + i) % debugFrameRing.size()];
        out << frame.tSec << "," << frame.mixAlignment << "," << frame.signalClarity << ","
            << frame.behaviorIndex << "," << frame.integratedLUFS << "," << frame.shortTermLUFS
            << "," << frame.truePeakDbTP << "," << frame.referenceTruePeakDbTP << ","
            << frame.truePeakVsReferenceDb << "," << frame.crestFactorDb << ","
            << frame.stereoWidth << "," << frame.correlation << "," << frame.spectralBalance
            << "," << frame.transientDensity << "," << frame.phaseRisk << "," << frame.flags
            << "\n";
    }
}

void MainView::triggerAdvisory(const AnalysisFrame& frame, const juce::String& prompt,
                               const std::vector<std::filesystem::path>& attachments, bool appendToChat)
{
    nlohmann::json result;
    result["timestamp_sec"] = frame.tSec;
    result["genre"] = genreFromIndex(frame.genreIndex);
    result["analysis_state_code"] = frame.analysisStateCode;
    result["reference_state_code"] = frame.referenceStateCode;
    result["reference_valid"] = frame.referenceValid != 0;
    result["scored_result_valid"] = frame.scoredResultValid != 0;
    result["live_signal_present"] = frame.liveSignalPresent != 0;
    result["mix_alignment"] =
        juce::jlimit(0.0, 10.0, static_cast<double>(frame.mixAlignment * 10.0f));
    result["mix_signature"] =
        juce::jlimit(0.0, 10.0, static_cast<double>(frame.approval * 10.0f));
    result["signal_clarity"] = frame.signalClarity;
    result["signal_stability"] = frame.signalStability;
    result["behavior_index"] = frame.behaviorIndex;
    result["integrated_lufs"] = frame.integratedLUFS;
    result["short_term_lufs"] = frame.shortTermLUFS;
    result["true_peak_dbtp"] = frame.truePeakDbTP;
    result["true_peak_reference_dbtp"] = frame.referenceTruePeakDbTP;
    result["true_peak_target_dbtp"] = frame.targetTruePeakDbTP;
    result["true_peak_vs_reference_db"] = frame.truePeakVsReferenceDb;
    result["comparison_mode"] = {{"live_reference", "nearest_track"},
                                   {"target_window", "genre_mean_plus_minus_3"}};
    result["target_window_db"] = frame.targetWindowDb;
    result["crest_factor_db"] = frame.crestFactorDb;
    result["transient_density"] = frame.transientDensity;
    result["spectral_balance"] = frame.spectralBalance;
    result["spectral_bands_db"] = {{"sub", frame.subBandDb},
                                     {"low", frame.lowBandDb},
                                     {"low_mid", frame.lowMidBandDb},
                                     {"mid", frame.midBandDb},
                                     {"high_mid", frame.highMidBandDb},
                                     {"high", frame.highBandDb},
                                     {"presence", frame.presenceBandDb},
                                     {"air", frame.airBandDb}};
    result["reference_spectral_bands_db"] = {{"sub", frame.referenceSubBandDb},
                                               {"low", frame.referenceLowBandDb},
                                               {"low_mid", frame.referenceLowMidBandDb},
                                               {"mid", frame.referenceMidBandDb},
                                               {"high_mid", frame.referenceHighMidBandDb},
                                               {"high", frame.referenceHighBandDb},
                                               {"presence", frame.referencePresenceBandDb},
                                               {"air", frame.referenceAirBandDb}};
    result["genre_target_spectral_bands_db"] = {{"sub", frame.targetSubBandDb},
                                                  {"low", frame.targetLowBandDb},
                                                  {"low_mid", frame.targetLowMidBandDb},
                                                  {"mid", frame.targetMidBandDb},
                                                  {"high_mid", frame.targetHighMidBandDb},
                                                  {"high", frame.targetHighBandDb},
                                                  {"presence", frame.targetPresenceBandDb},
                                                  {"air", frame.targetAirBandDb}};
    result["spectral_delta_db"] = {{"sub", frame.subBandDeltaDb},
                                     {"low", frame.lowBandDeltaDb},
                                     {"low_mid", frame.lowMidBandDeltaDb},
                                     {"mid", frame.midBandDeltaDb},
                                     {"high_mid", frame.highMidBandDeltaDb},
                                     {"high", frame.highBandDeltaDb},
                                     {"presence", frame.presenceBandDeltaDb},
                                     {"air", frame.airBandDeltaDb}};
    result["stereo"] = {{"width", frame.stereoWidth},
                          {"left_energy", frame.leftEnergy},
                          {"right_energy", frame.rightEnergy},
                          {"mid_energy", frame.midEnergy},
                          {"side_energy", frame.sideEnergy},
                          {"correlation", frame.correlation},
                          {"low_band_correlation", frame.lowBandCorrelation},
                          {"sub_mono_integrity", frame.subMonoIntegrity},
                          {"phase_risk", frame.phaseRisk},
                          {"low_band_phase_risk", frame.lowBandPhaseRisk},
                          {"center_dominance", frame.centerDominance},
                          {"side_dominance", frame.sideDominance},
                          {"stereo_motion", frame.stereoMotion},
                          {"spatial_spread", frame.spatialSpread},
                          {"reference_width", frame.referenceStereoWidth},
                          {"reference_correlation", frame.referenceCorrelation},
                          {"target_width", frame.targetStereoWidth},
                          {"target_correlation", frame.targetCorrelation}};
    result["flags"] = {{"bits", frame.flags},
                        {"transient_softening", (frame.flags & kFlagTransientSoftening) != 0},
                        {"harshness_burst", (frame.flags & kFlagHarshnessBurst) != 0},
                        {"width_collapse", (frame.flags & kFlagWidthCollapse) != 0},
                        {"clip_risk", (frame.flags & kFlagClipRisk) != 0},
                        {"phase_risk", (frame.flags & kFlagPhaseRisk) != 0}};
    result["plain_english"] = {{"halo_label", model.haloLabel.toStdString()},
                                 {"halo_detail", model.haloDetail.toStdString()},
                                 {"tonal_summary", model.tonalSummary.toStdString()},
                                 {"status", model.statusText.toStdString()}};
    result["overall_rating"] =
        juce::jlimit(0.0, 10.0, static_cast<double>(frame.approval * 10.0f));
    result["signal_clarity_legacy"] = frame.confidence;
    result["confidence"] = frame.confidence;
    const auto liveAnalysisJson = result.dump(2);

    nlohmann::json evidence = nlohmann::json::array();
    evidence.push_back({{"metric", "sub_band_delta_db"}, {"value", frame.subBandDeltaDb}, {"current_db", frame.subBandDb}, {"reference_db", frame.referenceSubBandDb}});
    evidence.push_back({{"metric", "low_band_delta_db"}, {"value", frame.lowBandDeltaDb}, {"current_db", frame.lowBandDb}, {"reference_db", frame.referenceLowBandDb}});
    evidence.push_back({{"metric", "low_mid_band_delta_db"}, {"value", frame.lowMidBandDeltaDb}, {"current_db", frame.lowMidBandDb}, {"reference_db", frame.referenceLowMidBandDb}});
    evidence.push_back({{"metric", "mid_band_delta_db"}, {"value", frame.midBandDeltaDb}, {"current_db", frame.midBandDb}, {"reference_db", frame.referenceMidBandDb}});
    evidence.push_back({{"metric", "high_mid_band_delta_db"}, {"value", frame.highMidBandDeltaDb}, {"current_db", frame.highMidBandDb}, {"reference_db", frame.referenceHighMidBandDb}});
    evidence.push_back({{"metric", "high_band_delta_db"}, {"value", frame.highBandDeltaDb}, {"current_db", frame.highBandDb}, {"reference_db", frame.referenceHighBandDb}});
    evidence.push_back({{"metric", "integrated_lufs"}, {"value", frame.integratedLUFS}});
    evidence.push_back({{"metric", "true_peak_vs_reference_db"}, {"value", frame.truePeakVsReferenceDb}});
    evidence.push_back({{"metric", "genre_target_window_db"}, {"value", frame.targetWindowDb}});
    evidence.push_back({{"metric", "stereo_width"}, {"value", frame.stereoWidth}});
    evidence.push_back({{"metric", "correlation"}, {"value", frame.correlation}});
    evidence.push_back({{"metric", "low_band_correlation"}, {"value", frame.lowBandCorrelation}});
    evidence.push_back({{"metric", "phase_risk"}, {"value", frame.phaseRisk}});
    evidence.push_back({{"metric", "low_band_phase_risk"}, {"value", frame.lowBandPhaseRisk}});
    evidence.push_back({{"metric", "center_dominance"}, {"value", frame.centerDominance}});
    evidence.push_back({{"metric", "stereo_motion"}, {"value", frame.stereoMotion}});
    evidence.push_back({{"metric", "transient_density"}, {"value", frame.transientDensity}});
    if ((frame.flags & kFlagTransientSoftening) != 0)
    {
        evidence.push_back({{"flag", "transient_softening"}, {"active", true}});
    }
    if ((frame.flags & kFlagHarshnessBurst) != 0)
    {
        evidence.push_back({{"flag", "harshness_burst"}, {"active", true}});
    }
    if ((frame.flags & kFlagWidthCollapse) != 0)
    {
        evidence.push_back({{"flag", "width_collapse"}, {"active", true}});
    }
    if ((frame.flags & kFlagClipRisk) != 0)
    {
        evidence.push_back({{"flag", "clip_risk"}, {"active", true}});
    }
    if ((frame.flags & kFlagPhaseRisk) != 0)
    {
        evidence.push_back({{"flag", "phase_risk"}, {"active", true}});
    }
    const auto liveFixJson = evidence.dump(2);

    const auto promptText = prompt.isNotEmpty()
                                ? prompt.toStdString()
                                : std::string("Refresh insight from the live realtime buffer and current DSP metrics.");

    advisory.submit(
        {"analysis",
         {genreFromIndex(frame.genreIndex),
          "translation",
          std::string("preserve punch and emotional impact; contract=") +
              dawai::aifr3d_core::contract::kSystemId +
              "; metrics=" + dawai::aifr3d_core::contract::kMetricMathPack,
          "vst-session",
          promptText,
          std::string(kLockedChatModel),
          attachments,
          liveAnalysisJson,
          liveFixJson},
         [this, appendToChat](std::optional<dawai::advisory_layer::AdvisoryOutput> output)
         {
             advisoryInFlight.store(false);
             if (!output)
             {
                 juce::MessageManager::callAsync(
                     [this, appendToChat]
                     {
                         const auto message = advisoryUnavailableMessage();
                         model.statusText = message;
                         chatStatus.setText(message, juce::dontSendNotification);
                         fixList.setModel(model);
                         repaint();
                         if (appendToChat)
                         {
                             appendChatHistoryEntry("AIFR3D: " + message);
                         }
                     });
                 return;
             }

             juce::MessageManager::callAsync(
                 [this, value = *output, appendToChat]
                 {
                     juce::String advisoryDigest = juce::String(value.summary30s).trim();
                     if (!value.issues.empty())
                     {
                         advisoryDigest << "|";
                         for (const auto& issue : value.issues)
                         {
                             advisoryDigest << juce::String(issue.summary).trim() << ";";
                         }
                     }

                     const bool shouldAppendChat =
                         appendToChat ||
                         (advisoryDigest.isNotEmpty() && advisoryDigest != lastLiveAdvisorySignature);
                     if (shouldAppendChat)
                     {
                         appendChatHistoryEntry((appendToChat ? "AIFR3D: " : "AIFR3D [Live]: ") +
                                                juce::String(value.summary30s));
                         if (!appendToChat)
                         {
                             lastLiveAdvisorySignature = advisoryDigest;
                         }
                     }

                     model.statusText = appendToChat ? "INSIGHT READY" : "LIVE BUFFER LINKED";
                     chatStatus.setText(
                         (appendToChat ? "AI updated (" : "Live advisory synced (") +
                             juce::String(static_cast<int>(value.signalClarity * 100.0)) +
                             "% clarity)",
                         juce::dontSendNotification);
                     fixList.setModel(model);
                     repaint();
                 });
         }});
}
