#include "MixTipsPanel.h"

#include "../Plugin/Theme.h"

namespace
{
juce::String topicText(MixTipsPanel::Topic topic)
{
    switch (topic)
    {
    case MixTipsPanel::Topic::IntegratedLoudness:
        return
            "Integrated Loudness\n"
            "What it means:\n"
            "Perceived loudness across the full track in LUFS. Streaming services turn loud masters down, so preserving punch matters more than chasing raw level.\n\n"
            "Colour key:\n"
            "🔵 Under target  |  🟦 Good  |  🟢 Professional target  |  🟡 Risk zone  |  🔴 Problem\n\n"
            "Why it matters:\n"
            "- Prevents loud-but-flat masters.\n"
            "- Keeps loudness aligned to platform targets.\n"
            "- Stops limiter drive from killing transient impact.\n\n"
            "How to fix loud mixes:\n"
            "- Reduce limiter threshold or output gain.\n"
            "- Check bus compression before pushing the master.\n"
            "- Keep true peak below the safety ceiling.\n\n"
            "How to fix quiet mixes:\n"
            "- Add measured makeup gain with a limiter.\n"
            "- Compare LUFS and RMS to a real reference.\n"
            "- Keep 3-6 dB headroom before final limiting.\n\n"
            "Reference examples:\n"
            "- Blinding Lights — loud modern pop with transient punch.\n"
            "- Random Access Memories — modern level with preserved dynamics.\n";

    case MixTipsPanel::Topic::DynamicRange:
        return
            "Dynamic Range\n"
            "What it means:\n"
            "Difference between loud peaks and average level. Lower values mean heavier compression and less contrast between sections.\n\n"
            "Colour key:\n"
            "🔵 Extra open  |  🟦 Healthy  |  🟢 Balanced  |  🟡 Compressed  |  🔴 Brick-walled\n\n"
            "Why it matters:\n"
            "- Low DR sounds flat and fatiguing.\n"
            "- Healthy DR keeps emotion, punch, and impact.\n"
            "- Streaming normalization rewards dynamics now.\n\n"
            "How to improve it:\n"
            "- Ease limiter pressure on the master bus.\n"
            "- Use parallel compression instead of crushing every bus.\n"
            "- Let verses stay narrower and choruses hit harder.\n"
            "- Automate level instead of compressing everything equally.\n\n"
            "Useful targets:\n"
            "- Classical: around DR 14.\n"
            "- Jazz: around DR 12.\n"
            "- Rock: around DR 8.\n"
            "- Pop: around DR 7.\n"
            "- EDM: around DR 5.\n\n"
            "Reference examples:\n"
            "- Kind of Blue — natural jazz dynamics.\n"
            "- Thriller — balanced punch.\n"
            "- Harlem Shake — intentionally heavy compression.\n";

    case MixTipsPanel::Topic::StereoWidth:
        return
            "Stereo Width\n"
            "What it means:\n"
            "How wide the mix feels and how safely it folds to mono. Width should add space without phase collapse or low-end weakness.\n\n"
            "Colour key:\n"
            "🔵 Too narrow  |  🟦 Slightly under  |  🟢 Stable width  |  🟡 Over-wide risk  |  🔴 Phase problem\n\n"
            "Width strategy:\n"
            "- Keep the low end mono below 120 Hz.\n"
            "- Center lead vocal, kick, snare, and bass.\n"
            "- Push pads, delays, reverbs, and background layers wider.\n"
            "- Keep guitars, keys, and support parts medium-wide.\n"
            "- Watch correlation: safe is above 0, ideal is around +0.5 or better.\n\n"
            "Common mistakes:\n"
            "- Widening every track.\n"
            "- Overusing Haas delay and creating comb filtering.\n"
            "- Skipping mono checks.\n"
            "- Letting bass live in stereo.\n\n"
            "Advanced moves:\n"
            "- Use mid-side EQ: keep lows in Mid, open highs in Side.\n"
            "- Widen groups, not every channel.\n"
            "- Automate width so choruses open up.\n\n"
            "Reference examples:\n"
            "- Blinding Lights — wide but mono-safe.\n"
            "- bad guy — headphone-wide production.\n"
            "- Sgt. Pepper (original stereo) — intentionally narrow vintage spread.\n";

    case MixTipsPanel::Topic::FrequencyBalance:
    default:
        return
            "Frequency Balance\n"
            "What it means:\n"
            "Energy distribution from sub through air. This is where mud, harshness, missing bass, thinness, and dark mixes show up first.\n\n"
            "Colour key:\n"
            "🔵 Under target  |  🟦 Slightly under  |  🟢 On target  |  🟡 Over target  |  🔴 Dominant problem\n\n"
            "Band guide:\n"
            "- Sub 20-60 Hz: weight and physical impact.\n"
            "- Bass 60-250 Hz: warmth and power.\n"
            "- Low-mid 250-500 Hz: body or mud.\n"
            "- Mid 500-2 kHz: vocal body and definition.\n"
            "- High-mid 2-6 kHz: presence or harshness.\n"
            "- Presence 6-12 kHz: sparkle or sibilance.\n"
            "- Air 12-20 kHz: space and sheen.\n\n"
            "Fix muddy mixes:\n"
            "- Cut 250-500 Hz on pads, guitars, and dense support parts.\n"
            "- High-pass non-bass elements at 80-100 Hz.\n"
            "- Check kick and bass together so they complement each other.\n\n"
            "Fix harsh mixes:\n"
            "- De-ess vocals around 5-8 kHz.\n"
            "- Use multiband control around 2-4 kHz.\n"
            "- Check cymbals and distortion for aggressive upper mids.\n\n"
            "Fix thin mixes:\n"
            "- Restore controlled weight around 80-120 Hz.\n"
            "- Add warmth around 200-300 Hz where needed.\n"
            "- Use saturation for density before boosting more top end.\n\n"
            "Reference examples:\n"
            "- bad guy — deep sub with controlled mids.\n"
            "- Blinding Lights — balanced low end and bright clarity.\n"
            "- Get Lucky — brighter disco-inspired top end.\n";
    }
}
} // namespace

MixTipsPanel::MixTipsPanel()
{
    const auto bind = [this](juce::TextButton& button, Topic topic)
    {
        addAndMakeVisible(button);
        button.onClick = [this, topic] { selectTopic(topic); };
    };

    bind(frequencyButton, Topic::FrequencyBalance);
    bind(loudnessButton, Topic::IntegratedLoudness);
    bind(dynamicRangeButton, Topic::DynamicRange);
    bind(stereoWidthButton, Topic::StereoWidth);

    addAndMakeVisible(content);
    content.setMultiLine(true);
    content.setReadOnly(true);
    content.setScrollbarsShown(true);
    content.setCaretVisible(false);
    content.setPopupMenuEnabled(false);
    content.setColour(juce::TextEditor::backgroundColourId, Theme::instance().panelAlt.brighter(0.02f));
    content.setColour(juce::TextEditor::outlineColourId, Theme::instance().panelStroke);
    content.setColour(juce::TextEditor::textColourId, Theme::instance().textSecondary);
    content.applyFontToAllText(Theme::instance().bodyFont().withHeight(14.0f));

    refreshContent();
}

void MixTipsPanel::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    auto area = getLocalBounds().toFloat().reduced(10.0f);

    juce::ColourGradient panelGradient(t.panel.brighter(0.04f), area.getX(), area.getY(),
                                       t.panelAlt.darker(0.12f), area.getX(), area.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(area, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(area, t.cornerRadius, 1.0f);

    auto title = area.toNearestInt().removeFromTop(26);
    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(15.0f));
    g.drawText("MIX TIPS", title, juce::Justification::left);

    auto legend = area.toNearestInt().removeFromBottom(20).reduced(12, 0);
    const std::array<std::pair<juce::Colour, juce::String>, 5> entries{{
        {juce::Colour(0xff3b82f6), "Under"},
        {t.cyan, "Good"},
        {juce::Colour(0xff39ff88), "Target"},
        {t.gold, "Risk"},
        {t.redWarn, "Problem"},
    }};

    int x = legend.getX();
    for (const auto& entry : entries)
    {
        g.setColour(entry.first);
        g.fillEllipse(static_cast<float>(x), static_cast<float>(legend.getY() + 4), 8.0f, 8.0f);
        x += 12;
        g.setColour(t.textSecondary);
        g.setFont(t.monoFont().withHeight(11.0f));
        g.drawText(entry.second, x, legend.getY(), 58, legend.getHeight(), juce::Justification::left);
        x += 62;
    }
}

void MixTipsPanel::resized()
{
    auto area = getLocalBounds().reduced(12, 34);
    auto tabs = area.removeFromTop(28);
    const int gap = 6;
    const int tabWidth = (tabs.getWidth() - gap * 3) / 4;
    frequencyButton.setBounds(tabs.removeFromLeft(tabWidth));
    tabs.removeFromLeft(gap);
    loudnessButton.setBounds(tabs.removeFromLeft(tabWidth));
    tabs.removeFromLeft(gap);
    dynamicRangeButton.setBounds(tabs.removeFromLeft(tabWidth));
    tabs.removeFromLeft(gap);
    stereoWidthButton.setBounds(tabs.removeFromLeft(tabWidth));
    area.removeFromTop(8);
    area.removeFromBottom(18);
    content.setBounds(area);
}

void MixTipsPanel::selectTopic(Topic topic)
{
    activeTopic = topic;
    refreshContent();
}

void MixTipsPanel::updateButtonStates()
{
    const auto setState = [this](juce::TextButton& button, Topic topic)
    {
        button.setToggleState(activeTopic == topic, juce::dontSendNotification);
    };

    frequencyButton.setClickingTogglesState(true);
    loudnessButton.setClickingTogglesState(true);
    dynamicRangeButton.setClickingTogglesState(true);
    stereoWidthButton.setClickingTogglesState(true);
    setState(frequencyButton, Topic::FrequencyBalance);
    setState(loudnessButton, Topic::IntegratedLoudness);
    setState(dynamicRangeButton, Topic::DynamicRange);
    setState(stereoWidthButton, Topic::StereoWidth);
}

void MixTipsPanel::refreshContent()
{
    updateButtonStates();
    content.setText(topicText(activeTopic), juce::dontSendNotification);
    content.moveCaretToTop(false);
}
