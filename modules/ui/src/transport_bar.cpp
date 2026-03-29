#include "dawai/ui/transport_bar.hpp"

#include "dawai/ui/theme/theme.hpp"

namespace dawai::ui
{

TransportBar::TransportBar()
{
    addAndMakeVisible(m_playButton);
    addAndMakeVisible(m_stopButton);
    addAndMakeVisible(m_pauseButton);
    addAndMakeVisible(m_loadButton);
    addAndMakeVisible(m_timeLabel);

    m_timeLabel.setText("00:00.000", juce::dontSendNotification);

    m_playButton.onClick = [this]
    {
        if (onPlay)
            onPlay();
    };
    m_stopButton.onClick = [this]
    {
        if (onStop)
            onStop();
    };
    m_pauseButton.onClick = [this]
    {
        if (onPause)
            onPause();
    };
    m_loadButton.onClick = [this]
    {
        if (onLoadTrack1)
            onLoadTrack1();
    };

    m_timeLabel.setColour(juce::Label::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_timeLabel.setFont(dawai::ui::theme::bodyFont().boldened().withHeight(13.0f));
}

void TransportBar::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat();
    juce::ColourGradient bg(dawai::ui::theme::Palette::panelAlt().brighter(0.02f), panel.getX(),
                            panel.getY(), dawai::ui::theme::Palette::panelAlt().darker(0.12f),
                            panel.getX(), panel.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(panel, 7.0f);
    g.setColour(dawai::ui::theme::Palette::stroke());
    g.drawRoundedRectangle(panel.reduced(1.0f), 7.0f, 1.0f);
}

void TransportBar::resized()
{
    auto area = getLocalBounds().reduced(6);
    m_playButton.setBounds(area.removeFromLeft(80));
    area.removeFromLeft(6);
    m_stopButton.setBounds(area.removeFromLeft(80));
    area.removeFromLeft(6);
    m_pauseButton.setBounds(area.removeFromLeft(80));
    area.removeFromLeft(10);
    m_loadButton.setBounds(area.removeFromLeft(220));
    area.removeFromLeft(10);
    m_timeLabel.setBounds(area.removeFromLeft(120));
}

void TransportBar::setTimeSeconds(double seconds)
{
    const int totalMs = static_cast<int>(seconds * 1000.0);
    const int mins = totalMs / 60000;
    const int sec = (totalMs / 1000) % 60;
    const int ms = totalMs % 1000;

    m_timeLabel.setText(juce::String::formatted("%02d:%02d.%03d", mins, sec, ms),
                        juce::dontSendNotification);
}

} // namespace dawai::ui
