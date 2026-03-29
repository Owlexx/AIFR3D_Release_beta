#include "dawai/ui/timeline_component.hpp"

#include "dawai/ui/theme/theme.hpp"

#include <algorithm>
#include <utility>

namespace dawai::ui
{

void TimelineComponent::setSession(dawai::audio_engine::SessionState state)
{
    m_session = std::move(state);
    repaint();
}

void TimelineComponent::setPlayheadSeconds(double seconds)
{
    m_playheadSeconds = seconds;
    repaint();
}

void TimelineComponent::paint(juce::Graphics& g)
{
    g.fillAll(dawai::ui::theme::Palette::panelAlt());

    auto bounds = getLocalBounds().toFloat();
    if (m_session.tracks.empty())
    {
        return;
    }

    const float rowHeight = bounds.getHeight() / static_cast<float>(m_session.tracks.size());
    for (std::size_t i = 0; i < m_session.tracks.size(); ++i)
    {
        const auto row = bounds.removeFromTop(rowHeight);
        g.setColour(dawai::ui::theme::Palette::textSecondary().withAlpha(0.2F));
        g.drawRect(row.toNearestInt(), 1);

        for (const auto& clip : m_session.tracks[i].clips)
        {
            const float x = static_cast<float>(clip.startSeconds) * 40.0F;
            const float w = std::max(20.0F, static_cast<float>(clip.lengthSeconds) * 40.0F);
            juce::Rectangle<float> region(row.getX() + x, row.getY() + 4.0F, w,
                                          row.getHeight() - 8.0F);
            g.setColour(dawai::ui::theme::Palette::accentMuted().withAlpha(0.6F));
            g.fillRoundedRectangle(region, 4.0F);
        }
    }

    const float playheadX = static_cast<float>(m_playheadSeconds) * 40.0F;
    g.setColour(dawai::ui::theme::Palette::accent());
    g.drawLine(playheadX, 0.0F, playheadX, static_cast<float>(getHeight()), 1.5F);
}

} // namespace dawai::ui
