#include "TopBar.h"

#include <array>
#include <cmath>

#include "../Plugin/Theme.h"
#include "dawai/aifr3d_core/version.hpp"

namespace
{
juce::String variantLabelFor(int layoutVariant)
{
    switch (layoutVariant)
    {
    case 2:
        return "B";
    case 3:
        return "C";
    case 4:
        return "D";
    default:
        return "A";
    }
}
} // namespace

juce::Image TopBar::tryLoadLogoFromDirectory(const juce::File& dir) const
{
    if (!dir.exists() || !dir.isDirectory())
        return {};

    juce::Array<juce::File> files;
    dir.findChildFiles(files, juce::File::findFiles, false, "*.png;*.jpg;*.jpeg;*.webp");
    if (files.isEmpty())
        return {};

    files.sort();
    return downscaleLogo(juce::ImageFileFormat::loadFrom(files.getReference(0)));
}

juce::Image TopBar::tryLoadLogoFile(const juce::File& file) const
{
    if (!file.existsAsFile())
        return {};
    return downscaleLogo(juce::ImageFileFormat::loadFrom(file));
}

juce::Image TopBar::loadBrandLogo() const
{
    if (const auto envPath = juce::SystemStats::getEnvironmentVariable("AIFR3D_LOGO_PATH", "");
        envPath.isNotEmpty())
    {
        const juce::File envFile(envPath);
        if (envFile.existsAsFile())
            return tryLoadLogoFile(envFile);
        if (auto img = tryLoadLogoFromDirectory(envFile); img.isValid())
            return img;
    }

    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    const std::array<juce::File, 12> logoFiles{
        home.getChildFile(".local/share/icons/hicolor/512x512/apps/dawai.png"),
        home.getChildFile(".local/share/icons/hicolor/256x256/apps/dawai.png"),
        cwd.getChildFile("assets/icons/aifred_logo.png"),
        cwd.getChildFile("assets/icons/aifred_logo.jpg"),
        cwd.getChildFile("../assets/icons/aifred_logo.png"),
        cwd.getChildFile("../assets/icons/aifred_logo.jpg"),
        home.getChildFile("Pictures/North3rnLight3r_Brand_Assets/Aifred_vst_logo.png"),
        home.getChildFile("Pictures/North3rnLight3r_Brand_Assets/Aifred_vst_logo.jpg"),
        home.getChildFile("Pictures/aifred-series_facebook_content/file_00000000b80c720cb4b68df64175a745.png"),
        cwd.getChildFile("assets/branding/aifred_logo.png"),
        cwd.getChildFile("../assets/branding/aifred_logo.png"),
        cwd.getChildFile("assets/branding/aifred_logo.jpg"),
    };
    for (const auto& file : logoFiles)
    {
        if (auto img = tryLoadLogoFile(file); img.isValid())
            return img;
    }

    const std::array<juce::File, 8> logoDirs{
        home.getChildFile("Pictures/North3rnLight3r_Brand_Assets"),
        home.getChildFile("Pictures/aifred-series_facebook_content"),
        cwd.getChildFile("assets/icons"),
        cwd.getChildFile("assets/branding"),
        cwd.getChildFile("assets"),
        cwd.getChildFile("../assets/icons"),
        cwd.getChildFile("../assets/branding"),
        cwd.getChildFile("../assets"),
    };

    for (const auto& dir : logoDirs)
    {
        if (auto img = tryLoadLogoFromDirectory(dir); img.isValid())
            return img;
    }

    return {};
}

juce::Image TopBar::downscaleLogo(const juce::Image& source)
{
    if (!source.isValid())
    {
        return {};
    }
    constexpr int kMaxDim = 256;
    if (source.getWidth() <= kMaxDim && source.getHeight() <= kMaxDim)
    {
        return source;
    }
    return source.rescaled(kMaxDim, kMaxDim, juce::Graphics::highResamplingQuality);
}

TopBar::TopBar()
{
    addAndMakeVisible(compareBtn);
    addAndMakeVisible(refBtn);
    addAndMakeVisible(approvalBtn);
    addAndMakeVisible(demoToggle);
    addAndMakeVisible(layoutLabel);
    addAndMakeVisible(layoutBox);

    brandLogo = loadBrandLogo();

    compareBtn.setClickingTogglesState(true);
    refBtn.setClickingTogglesState(true);
    approvalBtn.setClickingTogglesState(true);
    compareBtn.setRadioGroupId(41);
    refBtn.setRadioGroupId(41);
    approvalBtn.setRadioGroupId(41);
    approvalBtn.setToggleState(true, juce::dontSendNotification);
    updateModeSelection(&approvalBtn);

    compareBtn.onClick = [this] { updateModeSelection(&compareBtn); };
    refBtn.onClick = [this] { updateModeSelection(&refBtn); };
    approvalBtn.onClick = [this] { updateModeSelection(&approvalBtn); };

    layoutLabel.setText("Theme Layout", juce::dontSendNotification);
    layoutLabel.setJustificationType(juce::Justification::centredRight);
    layoutLabel.setColour(juce::Label::textColourId, Theme::instance().textSecondary);
    layoutBox.addItem("A", 1);
    layoutBox.addItem("B", 2);
    layoutBox.addItem("C", 3);
    layoutBox.addItem("D", 4);
    layoutBox.setSelectedId(1, juce::dontSendNotification);
    layoutBox.onChange = [this]
    {
        const int selected = juce::jlimit(1, 4, layoutBox.getSelectedId());
        if (selectedLayoutVariant == selected)
        {
            return;
        }
        selectedLayoutVariant = selected;
        if (onLayoutVariantChanged)
        {
            onLayoutVariantChanged(selectedLayoutVariant);
        }
        repaint();
    };
}

void TopBar::setLayoutVariant(int variant)
{
    const int clamped = juce::jlimit(1, 4, variant);
    selectedLayoutVariant = clamped;
    layoutBox.setSelectedId(clamped, juce::dontSendNotification);
    repaint();
}

void TopBar::updateModeSelection(juce::Button* source)
{
    if (source == nullptr)
        return;

    auto nextMode = Mode::Analyze;
    if (source == &compareBtn)
    {
        activeModeLabel = "COMPARE";
        nextMode = Mode::Compare;
    }
    else if (source == &refBtn)
    {
        activeModeLabel = "REFERENCE";
        nextMode = Mode::Reference;
    }
    else
    {
        activeModeLabel = "ANALYZE";
        nextMode = Mode::Analyze;
    }

    if (activeMode != nextMode)
    {
        activeMode = nextMode;
        if (onModeChanged)
        {
            onModeChanged(activeMode);
        }
    }
    else
    {
        activeMode = nextMode;
    }
}

void TopBar::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    auto panel = getLocalBounds().toFloat();
    const float phase = model ? model->visualPhase : 0.0f;
    const float pulse = model ? model->visualPulse : 0.4f;
    const float shimmer = 0.5f + 0.5f * std::sin(phase);
    juce::ColourGradient panelGradient(t.panel.brighter(0.06f), panel.getX(), panel.getY(),
                                       t.panelAlt.darker(0.16f), panel.getX(), panel.getBottom(),
                                       false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panel, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(panel.reduced(0.8f), t.cornerRadius, 1.0f);

    const int logoX = 12;
    const int logoY = 9;
    const int logoSize = 46;
    const auto logoArea =
        juce::Rectangle<float>((float)logoX, (float)logoY, (float)logoSize, (float)logoSize);
    if (brandLogo.isValid())
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageWithin(brandLogo, logoArea.toNearestInt().getX(), logoArea.toNearestInt().getY(),
                          logoArea.toNearestInt().getWidth(), logoArea.toNearestInt().getHeight(),
                          juce::RectanglePlacement::centred |
                              juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        g.setColour(t.bg.brighter(0.12f));
        g.fillRoundedRectangle(logoArea, 12.0f);
    }

    const float logoGlowPad = 4.0f + 3.0f * pulse;
    g.setColour(t.cyan.withAlpha(0.10f + 0.10f * shimmer));
    g.drawRoundedRectangle(logoArea.expanded(logoGlowPad), 13.0f, 1.1f);

    int controlsLeft = getWidth() - 12;
    for (auto* component : {static_cast<juce::Component*>(&layoutLabel),
                            static_cast<juce::Component*>(&layoutBox),
                            static_cast<juce::Component*>(&compareBtn),
                            static_cast<juce::Component*>(&refBtn),
                            static_cast<juce::Component*>(&approvalBtn),
                            static_cast<juce::Component*>(&demoToggle)})
    {
        if (component != nullptr && !component->getBounds().isEmpty())
        {
            controlsLeft = juce::jmin(controlsLeft, component->getX());
        }
    }

    auto infoArea = juce::Rectangle<int>(66, 8, juce::jmax(190, controlsLeft - 78), getHeight() - 16);
    auto pillArea = infoArea.removeFromRight(juce::jmin(124, juce::jmax(92, infoArea.getWidth() / 4)));
    infoArea.removeFromRight(10);
    auto titleArea = infoArea.removeFromTop(26);
    auto versionArea = infoArea.removeFromTop(16);
    auto statusArea = infoArea.removeFromTop(18);
    auto genreArea = infoArea.removeFromTop(16);

    juce::ColourGradient titleGradient(t.teal, static_cast<float>(titleArea.getX()), 14.0f,
                                       t.gold, static_cast<float>(titleArea.getRight()), 40.0f,
                                       false);
    titleGradient.addColour(0.45, t.cyan);
    titleGradient.addColour(0.78, t.text.withAlpha(0.96f));
    g.setGradientFill(titleGradient);
    auto titleFont = t.titleFont().withHeight(28.0f).boldened();
    g.setFont(titleFont);
    g.drawFittedText("AIFR3D VST 2.2.5 Beta", titleArea,
                     juce::Justification::centredLeft, 1);
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(13.5f));
    g.drawFittedText("Version " + juce::String(dawai::aifr3d_core::kCanonicalVersion) +
                         " | Layout " + variantLabelFor(selectedLayoutVariant),
                     versionArea, juce::Justification::centredLeft, 1);

    if (model != nullptr)
    {
        g.setColour(t.text);
        g.setFont(t.bodyFont().withHeight(14.5f));
        g.drawFittedText("Priority issue: " + model->statusText, statusArea,
                         juce::Justification::centredLeft, 1);

        const int genrePercent = juce::roundToInt(model->detectedGenreStability * 100.0f);
        const auto genreLine = "Genre: " + model->detectedGenre + " (" +
                               juce::String(genrePercent) + "% match clarity)";
        g.setColour(t.textSecondary);
        g.drawFittedText(genreLine, genreArea, juce::Justification::centredLeft, 1);
    }

    auto modePill = pillArea.toFloat().withTrimmedTop(8.0f).withHeight(26.0f);
    g.setColour(t.cyan.withAlpha(0.12f + 0.08f * pulse));
    g.fillRoundedRectangle(modePill, 12.0f);
    g.setColour(t.cyan.withAlpha(0.30f + 0.22f * shimmer));
    g.drawRoundedRectangle(modePill, 12.0f, 1.0f);
    const float sweepX = modePill.getX() + std::fmod(phase * 42.0f, modePill.getWidth());
    juce::ColourGradient sweep(t.cyan.withAlpha(0.0f), sweepX - 22.0f, modePill.getY(),
                               t.cyan.withAlpha(0.20f + 0.10f * pulse), sweepX,
                               modePill.getCentreY(), false);
    sweep.addColour(1.0, t.cyan.withAlpha(0.0f));
    g.setGradientFill(sweep);
    g.fillRoundedRectangle(modePill.reduced(2.0f, 2.0f), 10.0f);
    g.setFont(t.bodyFont().boldened().withHeight(13.5f));
    g.setColour(t.text);
    g.drawFittedText(activeModeLabel, modePill.toNearestInt(), juce::Justification::centred, 1);
}

void TopBar::resized()
{
    auto r = getLocalBounds().reduced(10);
    auto controls = r.removeFromRight(juce::jmin(540, juce::jmax(400, r.getWidth() / 2)));

    const int gap = 6;
    const int demoWidth = 84;
    demoToggle.setBounds(controls.removeFromRight(demoWidth).reduced(4));
    controls.removeFromRight(gap);

    const int layoutBoxWidth = 72;
    const int layoutLabelWidth = juce::jlimit(74, 106, controls.getWidth() / 4);
    auto buttonArea = controls;
    layoutBox.setBounds(buttonArea.removeFromRight(layoutBoxWidth).reduced(2));
    buttonArea.removeFromRight(gap);
    layoutLabel.setBounds(buttonArea.removeFromRight(layoutLabelWidth).reduced(2));
    buttonArea.removeFromRight(gap);

    const int availableForButtons = juce::jmax(240, buttonArea.getWidth());
    const int buttonWidth = juce::jlimit(76, 98, (availableForButtons - gap * 2) / 3);
    compareBtn.setBounds(buttonArea.removeFromRight(buttonWidth).reduced(4));
    buttonArea.removeFromRight(gap);
    refBtn.setBounds(buttonArea.removeFromRight(buttonWidth).reduced(4));
    buttonArea.removeFromRight(gap);
    approvalBtn.setBounds(buttonArea.removeFromRight(buttonWidth + 6).reduced(4));
}
