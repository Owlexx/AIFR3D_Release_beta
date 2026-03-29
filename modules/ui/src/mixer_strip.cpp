#include "dawai/ui/mixer_strip.hpp"

#include "dawai/ui/theme/theme.hpp"

namespace dawai::ui
{

MixerStrip::MixerStrip(int trackIndex) : m_trackIndex(trackIndex)
{
    addAndMakeVisible(m_name);
    addAndMakeVisible(m_mute);
    addAndMakeVisible(m_solo);
    addAndMakeVisible(m_fader);
    addAndMakeVisible(m_pan);
    for (auto& load : m_insertLoad)
    {
        addAndMakeVisible(load);
    }
    for (auto& insert : m_insertBypass)
    {
        insert.setClickingTogglesState(true);
        addAndMakeVisible(insert);
    }

    m_name.setText("Track " + juce::String(trackIndex + 1), juce::dontSendNotification);
    m_name.setJustificationType(juce::Justification::centred);
    m_name.setColour(juce::Label::textColourId, dawai::ui::theme::Palette::textSecondary());

    m_fader.setSliderStyle(juce::Slider::LinearVertical);
    m_fader.setRange(-60.0, 6.0, 0.1);
    m_fader.setValue(0.0);

    m_pan.setSliderStyle(juce::Slider::LinearHorizontal);
    m_pan.setRange(-1.0, 1.0, 0.01);
    m_pan.setValue(0.0);

    m_mute.onClick = [this]
    {
        if (onMute)
            onMute(m_trackIndex, m_mute.getToggleState());
    };
    m_solo.onClick = [this]
    {
        if (onSolo)
            onSolo(m_trackIndex, m_solo.getToggleState());
    };
    m_fader.onValueChange = [this]
    {
        if (onFader)
            onFader(m_trackIndex, static_cast<float>(m_fader.getValue()));
    };
    m_pan.onValueChange = [this]
    {
        if (onPan)
            onPan(m_trackIndex, static_cast<float>(m_pan.getValue()));
    };

    for (std::size_t i = 0; i < m_insertBypass.size(); ++i)
    {
        m_insertLoad[i].onClick = [this, i]
        {
            if (onInsertLoad)
            {
                onInsertLoad(m_trackIndex, static_cast<int>(i));
            }
        };
        m_insertBypass[i].onClick = [this, i]
        {
            if (onInsertBypass)
            {
                onInsertBypass(m_trackIndex, static_cast<int>(i),
                               m_insertBypass[i].getToggleState());
            }
        };
    }
}

void MixerStrip::resized()
{
    auto area = getLocalBounds().reduced(4);
    m_name.setBounds(area.removeFromTop(24));

    auto buttons = area.removeFromTop(24);
    m_mute.setBounds(buttons.removeFromLeft(28));
    buttons.removeFromLeft(4);
    m_solo.setBounds(buttons.removeFromLeft(28));

    area.removeFromTop(8);
    auto insertLoadRow = area.removeFromTop(22);
    for (auto& load : m_insertLoad)
    {
        load.setBounds(insertLoadRow.removeFromLeft(24));
        insertLoadRow.removeFromLeft(2);
    }
    area.removeFromTop(2);
    auto insertBypassRow = area.removeFromTop(22);
    for (auto& insert : m_insertBypass)
    {
        insert.setBounds(insertBypassRow.removeFromLeft(24));
        insertBypassRow.removeFromLeft(2);
    }

    area.removeFromTop(6);
    m_pan.setBounds(area.removeFromBottom(30));
    area.removeFromBottom(6);
    m_fader.setBounds(area);
}

void MixerStrip::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat();
    juce::ColourGradient bg(dawai::ui::theme::Palette::panel().brighter(0.03f), panel.getX(),
                            panel.getY(), dawai::ui::theme::Palette::panelAlt().darker(0.12f),
                            panel.getX(), panel.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(panel, 7.0F);
    g.setColour(dawai::ui::theme::Palette::stroke());
    g.drawRoundedRectangle(panel.reduced(1.0f), 7.0F, 1.0F);
}

void MixerStrip::setTrackName(const juce::String& name)
{
    m_name.setText(name, juce::dontSendNotification);
}

} // namespace dawai::ui
