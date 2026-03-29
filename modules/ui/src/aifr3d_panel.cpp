#include "dawai/ui/aifr3d_panel.hpp"

#include "dawai/ui/theme/theme.hpp"

#include <cmath>

namespace dawai::ui
{

Aifr3dPanel::Aifr3dPanel()
{
    addAndMakeVisible(m_trueButton);
    addAndMakeVisible(m_compareButton);
    addAndMakeVisible(m_referenceButton);
    addAndMakeVisible(m_pickPremixButton);
    addAndMakeVisible(m_pickMixedButton);
    addAndMakeVisible(m_analyzeRendersButton);
    addAndMakeVisible(m_compareStatus);
    addAndMakeVisible(m_meterPanel);
    addAndMakeVisible(m_insightPanel);

    auto setModeFrom = [this](Mode mode) { setMode(mode); };
    m_trueButton.onClick = [setModeFrom] { setModeFrom(Mode::True); };
    m_compareButton.onClick = [setModeFrom] { setModeFrom(Mode::Compare); };
    m_referenceButton.onClick = [setModeFrom] { setModeFrom(Mode::Reference); };

    m_trueButton.setClickingTogglesState(true);
    m_compareButton.setClickingTogglesState(true);
    m_referenceButton.setClickingTogglesState(true);
    m_trueButton.setRadioGroupId(32);
    m_compareButton.setRadioGroupId(32);
    m_referenceButton.setRadioGroupId(32);
    m_trueButton.setToggleState(true, juce::dontSendNotification);

    m_compareStatus.setText("Capture Mix A, then capture Mix B or compare against live.",
                            juce::dontSendNotification);
    m_compareStatus.setJustificationType(juce::Justification::centredLeft);
    m_compareStatus.setColour(juce::Label::textColourId,
                              dawai::ui::theme::Palette::textSecondary());

    m_pickPremixButton.onClick = [this]
    {
        m_meterPanel.captureCompareMixA();
        m_compareStatus.setText("Mix A captured from the current live buffer.",
                                juce::dontSendNotification);
    };
    m_pickMixedButton.onClick = [this]
    {
        m_meterPanel.captureCompareMixB();
        m_compareStatus.setText("Mix B captured from the current live buffer.",
                                juce::dontSendNotification);
    };
    m_analyzeRendersButton.onClick = [this]
    {
        m_meterPanel.clearCompareMixB();
        m_compareStatus.setText("Compare mode now tracks Mix A against the live buffer.",
                                juce::dontSendNotification);
    };

    m_pickPremixButton.setButtonText("Capture Mix A");
    m_pickMixedButton.setButtonText("Capture Mix B");
    m_analyzeRendersButton.setButtonText("Use Live B");
}

void Aifr3dPanel::setMode(Mode mode)
{
    m_mode = mode;
    m_trueButton.setToggleState(mode == Mode::True, juce::dontSendNotification);
    m_compareButton.setToggleState(mode == Mode::Compare, juce::dontSendNotification);
    m_referenceButton.setToggleState(mode == Mode::Reference, juce::dontSendNotification);
    m_meterPanel.setMode(mode == Mode::Compare     ? MeterPanel::Mode::Compare
                         : mode == Mode::Reference ? MeterPanel::Mode::Reference
                                                   : MeterPanel::Mode::Analyze);
    repaint();
}

void Aifr3dPanel::setMeterSnapshot(const dawai::metering::MeterSnapshot& snapshot)
{
    m_meterPanel.setSnapshot(snapshot);
}

void Aifr3dPanel::setReferenceTruePeakDbtp(double value)
{
    m_meterPanel.setReferenceTruePeakDbtp(value);
}

void Aifr3dPanel::setInsight(const dawai::advisory_layer::AdvisoryOutput& output)
{
    m_insightPanel.setOutput(output);
}

void Aifr3dPanel::appendInsightStatus(const juce::String& message)
{
    m_insightPanel.appendStatusMessage(message);
}

void Aifr3dPanel::setAnimationPhase(float phase, float pulse)
{
    m_animationPhase = phase;
    m_animationPulse = juce::jlimit(0.0F, 1.0F, pulse);
    m_meterPanel.setAnimationPhase(m_animationPhase, m_animationPulse);
    m_insightPanel.setAnimationPhase(m_animationPhase, m_animationPulse);
    repaint();
}

void Aifr3dPanel::resized()
{
    auto area = getLocalBounds().reduced(6);
    auto modeRow = area.removeFromTop(28);
    m_trueButton.setBounds(modeRow.removeFromLeft(92));
    modeRow.removeFromLeft(6);
    m_compareButton.setBounds(modeRow.removeFromLeft(92));
    modeRow.removeFromLeft(6);
    m_referenceButton.setBounds(modeRow.removeFromLeft(110));

    area.removeFromTop(6);
    auto split = area;
    m_meterPanel.setBounds(split.removeFromTop(area.proportionOfHeight(0.55F)));
    split.removeFromTop(6);
    auto compareRow = split.removeFromTop(28);
    m_pickPremixButton.setBounds(compareRow.removeFromLeft(110));
    compareRow.removeFromLeft(4);
    m_pickMixedButton.setBounds(compareRow.removeFromLeft(110));
    compareRow.removeFromLeft(4);
    m_analyzeRendersButton.setBounds(compareRow.removeFromLeft(90));
    compareRow.removeFromLeft(4);
    m_compareStatus.setBounds(compareRow);

    split.removeFromTop(4);
    m_insightPanel.setBounds(split);

    const bool showCompareTools = (m_mode == Mode::Compare);
    m_pickPremixButton.setVisible(showCompareTools);
    m_pickMixedButton.setVisible(showCompareTools);
    m_analyzeRendersButton.setVisible(showCompareTools);
    m_compareStatus.setVisible(showCompareTools);
}

void Aifr3dPanel::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat();
    const float shimmer = 0.5f + 0.5f * std::sin(m_animationPhase);
    juce::ColourGradient bg(dawai::ui::theme::Palette::panelAlt().brighter(0.05f), panel.getX(),
                            panel.getY(), dawai::ui::theme::Palette::panelAlt().darker(0.16f),
                            panel.getX(), panel.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(panel, 8.0F);
    g.setColour(dawai::ui::theme::Palette::stroke());
    g.drawRoundedRectangle(panel.reduced(1.0f), 8.0F, 1.0F);
    g.setColour(dawai::ui::theme::Palette::accent().withAlpha(0.05f + 0.08f * m_animationPulse));
    g.drawRoundedRectangle(panel.reduced(2.0f), 8.0F, 1.0F);

    g.setColour(dawai::ui::theme::Palette::textSecondary());
    juce::String modeLabel = "TRUE";
    if (m_mode == Mode::Compare)
        modeLabel = "COMPARE";
    if (m_mode == Mode::Reference)
        modeLabel = "REFERENCE";

    g.setFont(dawai::ui::theme::bodyFont().withHeight(12.0f).boldened());
    g.drawText("AIFR3D | Mode: " + modeLabel + "   <>",
               getLocalBounds().removeFromTop(20).reduced(8, 0), juce::Justification::right);

    const auto headerLine = juce::Rectangle<float>(panel.getX() + 8.0F, panel.getY() + 18.0F,
                                                   panel.getWidth() - 16.0F, 1.0F);
    juce::ColourGradient scan(
        dawai::ui::theme::Palette::accent().withAlpha(0.0f), headerLine.getX(), headerLine.getY(),
        dawai::ui::theme::Palette::accent().withAlpha(0.18f + 0.12f * shimmer),
        headerLine.getX() +
            std::fmod(m_animationPhase * 90.0f, juce::jmax(32.0f, headerLine.getWidth())),
        headerLine.getY(), false);
    scan.addColour(1.0, dawai::ui::theme::Palette::accent().withAlpha(0.0f));
    g.setGradientFill(scan);
    g.fillRect(headerLine);
}

} // namespace dawai::ui
