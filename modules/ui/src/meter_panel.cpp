#include "dawai/ui/meter_panel.hpp"

#include "dawai/ui/theme/theme.hpp"

#include <array>
#include <cmath>

namespace dawai::ui
{

namespace
{
juce::Colour blueUnder()
{
    return juce::Colour(0xff3b82f6);
}

juce::Colour greenTarget()
{
    return juce::Colour(0xff39ff88);
}

juce::Colour comparisonColour(float delta)
{
    if (std::abs(delta) <= 0.25f)
    {
        return greenTarget();
    }
    if (delta < -1.0f)
    {
        return blueUnder();
    }
    if (delta < -0.25f)
    {
        return dawai::ui::theme::Palette::accent();
    }
    if (delta > 1.0f)
    {
        return dawai::ui::theme::Palette::danger();
    }
    return dawai::ui::theme::Palette::accentWarm();
}

juce::String titleFor(MeterPanel::Mode mode)
{
    switch (mode)
    {
    case MeterPanel::Mode::Compare:
        return "A/B Metering";
    case MeterPanel::Mode::Reference:
        return "Live vs Reference Metering";
    case MeterPanel::Mode::Analyze:
    default:
        return "Live Mix Metering";
    }
}

juce::String helperFor(MeterPanel::Mode mode)
{
    switch (mode)
    {
    case MeterPanel::Mode::Compare:
        return "Current live buffer is compared against captured Mix A / Mix B snapshots.";
    case MeterPanel::Mode::Reference:
        return "Purple reference cues show the target corridor only in Reference mode.";
    case MeterPanel::Mode::Analyze:
    default:
        return "All primary readouts follow the current live buffer only.";
    }
}

bool hasProgramMaterial(const dawai::metering::MeterSnapshot& snapshot)
{
    return snapshot.loudness.shortTermLufs > -60.0 || snapshot.dynamics.transientDensity > 0.01 ||
           snapshot.loudness.truePeakDbtp > -50.0;
}

void drawMetricLine(juce::Graphics& g, juce::Rectangle<int> row, const juce::String& text)
{
    g.drawText(text, row, juce::Justification::left);
}

} // namespace

void MeterPanel::setMode(Mode mode)
{
    m_mode = mode;
    repaint();
}

void MeterPanel::setSnapshot(const dawai::metering::MeterSnapshot& snapshot)
{
    m_snapshot = snapshot;
    repaint();
}

void MeterPanel::captureCompareMixA()
{
    m_compareMixA = m_snapshot;
    repaint();
}

void MeterPanel::captureCompareMixB()
{
    m_compareMixB = m_snapshot;
    repaint();
}

void MeterPanel::clearCompareMixB()
{
    m_compareMixB.reset();
    repaint();
}

void MeterPanel::setReferenceTruePeakDbtp(double value)
{
    m_referenceTruePeakDbtp = value;
    repaint();
}

void MeterPanel::setAnimationPhase(float phase, float pulse)
{
    m_animationPhase = phase;
    m_animationPulse = juce::jlimit(0.0F, 1.0F, pulse);
    repaint();
}

void MeterPanel::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat();
    const bool liveProgramMaterial = hasProgramMaterial(m_snapshot);
    const float shimmer = 0.5f + 0.5f * std::sin(m_animationPhase);
    juce::ColourGradient bg(dawai::ui::theme::Palette::panel().brighter(0.04f), panel.getX(),
                            panel.getY(), dawai::ui::theme::Palette::panelAlt().darker(0.15f),
                            panel.getX(), panel.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(panel, 7.0f);
    g.setColour(dawai::ui::theme::Palette::stroke());
    g.drawRoundedRectangle(panel.reduced(1.0f), 7.0f, 1.0f);
    g.setColour(dawai::ui::theme::Palette::accent().withAlpha(0.04f + 0.08f * m_animationPulse));
    g.drawRoundedRectangle(panel.reduced(2.0f), 7.0f, 1.0f);

    auto bounds = getLocalBounds().reduced(8);
    g.setColour(dawai::ui::theme::Palette::textPrimary());
    g.setFont(dawai::ui::theme::bodyFont().boldened().withHeight(16.0f));
    g.drawText(titleFor(m_mode), bounds.removeFromTop(20), juce::Justification::left);
    g.setColour(dawai::ui::theme::Palette::textSecondary());
    g.setFont(dawai::ui::theme::bodyFont().withHeight(12.5f));
    g.drawText(helperFor(m_mode), bounds.removeFromTop(16), juce::Justification::left);

    auto spectrumArea = bounds.removeFromTop(getHeight() / 2 - 22);
    g.setColour(dawai::ui::theme::Palette::accentMuted().withAlpha(0.18F));
    g.fillRoundedRectangle(spectrumArea.toFloat(), 6.0f);

    const std::array<float, 7> dbMarks{3.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -32.0f};
    const auto mapY = [&](double db)
    {
        const float norm = juce::jmap(static_cast<float>(db), -32.0F, 3.0F, 1.0F, 0.0F);
        return spectrumArea.getY() + norm * spectrumArea.getHeight();
    };

    g.setColour(dawai::ui::theme::Palette::textSecondary().withAlpha(0.18f));
    for (float mark : dbMarks)
    {
        const float y = mapY(mark);
        g.drawLine(static_cast<float>(spectrumArea.getX()), y,
                   static_cast<float>(spectrumArea.getRight()), y, 1.0f);
    }

    const auto& bins = m_snapshot.spectrum.averagedBinsDb;
    if (bins.size() > 1)
    {
        juce::Path path;
        const float xStep = spectrumArea.getWidth() / static_cast<float>(bins.size() - 1);
        path.startNewSubPath(spectrumArea.getX(),
                             mapY(juce::jlimit(-32.0, 3.0, bins.front())));
        for (std::size_t i = 1; i < bins.size(); ++i)
        {
            path.lineTo(spectrumArea.getX() + static_cast<float>(i) * xStep,
                        mapY(juce::jlimit(-32.0, 3.0, bins[i])));
        }

        if (m_mode == Mode::Reference)
        {
            g.setColour(dawai::ui::theme::Palette::accentPurple().withAlpha(0.14f));
            g.strokePath(path, juce::PathStrokeType(4.5F));
        }
        g.setColour(dawai::ui::theme::Palette::accent());
        g.strokePath(path, juce::PathStrokeType(2.0F));
    }

    if (m_mode == Mode::Compare && m_compareMixA.has_value())
    {
        const auto x = spectrumArea.getX() + 14;
        const auto y = spectrumArea.getY() + 12;
        g.setColour(dawai::ui::theme::Palette::accentPurple());
        g.fillEllipse(static_cast<float>(x), static_cast<float>(y), 8.0f, 8.0f);
        g.setColour(dawai::ui::theme::Palette::textSecondary());
        g.drawText("Mix A captured", x + 14, y - 2, 110, 12, juce::Justification::left);
        if (m_compareMixB.has_value())
        {
            g.setColour(dawai::ui::theme::Palette::accent());
            g.fillEllipse(static_cast<float>(x), static_cast<float>(y + 14), 8.0f, 8.0f);
            g.drawText("Mix B captured", x + 14, y + 12, 110, 12, juce::Justification::left);
        }
    }

    g.setColour(dawai::ui::theme::Palette::textPrimary());
    g.setFont(dawai::ui::theme::bodyFont().withHeight(14.0f));

    if (!liveProgramMaterial)
    {
        g.setColour(dawai::ui::theme::Palette::textSecondary());
        g.drawFittedText(
            "No valid program material. Meter rows stay withheld until the live buffer contains enough audio to measure honestly.",
            bounds.reduced(2), juce::Justification::topLeft, 4);
        return;
    }

    if (m_mode == Mode::Compare)
    {
        const auto* mixA = m_compareMixA ? &(*m_compareMixA) : nullptr;
        const auto* mixB = m_compareMixB ? &(*m_compareMixB) : &m_snapshot;
        const double lufsDelta = (mixA != nullptr) ? (mixB->loudness.integratedLufs - mixA->loudness.integratedLufs) : 0.0;
        const double peakDelta = (mixA != nullptr) ? (mixB->loudness.truePeakDbtp - mixA->loudness.truePeakDbtp) : 0.0;
        const double widthDelta = (mixA != nullptr) ? (mixB->stereo.width - mixA->stereo.width) : 0.0;
        const double corrDelta = (mixA != nullptr) ? (mixB->stereo.correlation - mixA->stereo.correlation) : 0.0;
        const double crestDelta = (mixA != nullptr) ? (mixB->dynamics.crestFactorDb - mixA->dynamics.crestFactorDb) : 0.0;
        const auto deltaColour = comparisonColour(static_cast<float>(lufsDelta));
        g.setColour(deltaColour);
        drawMetricLine(g, bounds.removeFromTop(18),
                       mixA != nullptr ? "Mix B/live minus Mix A" : "Capture Mix A for A/B deltas");
        g.setColour(dawai::ui::theme::Palette::textPrimary());
        drawMetricLine(g, bounds.removeFromTop(18),
                       "LUFS Δ: " + juce::String(lufsDelta, 2) + " | Peak Δ: " +
                           juce::String(peakDelta, 2) + " dBTP");
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Width Δ: " + juce::String(widthDelta, 2) + " | Corr Δ: " +
                           juce::String(corrDelta, 2));
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Crest Δ: " + juce::String(crestDelta, 2) + " dB");
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Mix A capture is purple. Current/live or Mix B is cyan.");
    }
    else
    {
        drawMetricLine(g, bounds.removeFromTop(18),
                       "LUFS Int: " + juce::String(m_snapshot.loudness.integratedLufs, 2));
        drawMetricLine(g, bounds.removeFromTop(18),
                       "LUFS ST: " + juce::String(m_snapshot.loudness.shortTermLufs, 2));
        drawMetricLine(g, bounds.removeFromTop(18),
                       "True Peak: " + juce::String(m_snapshot.loudness.truePeakDbtp, 2) +
                           " dBTP");
        if (m_mode == Mode::Reference && m_referenceTruePeakDbtp > -80.0)
        {
            const double delta = m_snapshot.loudness.truePeakDbtp - m_referenceTruePeakDbtp;
            drawMetricLine(g, bounds.removeFromTop(18),
                           "Reference peak: " + juce::String(m_referenceTruePeakDbtp, 2) +
                               " dBTP | Delta: " + juce::String(delta, 2) + " dB");
        }
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Stereo Width: " + juce::String(m_snapshot.stereo.width, 2) +
                           " | Correlation: " + juce::String(m_snapshot.stereo.correlation, 2));
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Peak dBFS: " + juce::String(m_snapshot.dynamics.peakDbfs, 2) +
                           " | Crest: " + juce::String(m_snapshot.dynamics.crestFactorDb, 2) +
                           " dB");
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Transient density: " +
                           juce::String(m_snapshot.dynamics.transientDensity, 2) +
                           " | Spectral tilt: " +
                           juce::String(m_snapshot.spectrum.spectralTiltDb, 2) + " dB");
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Low: " + juce::String(m_snapshot.spectrum.lowBandDb, 1) +
                           " dB | Low-mid: " + juce::String(m_snapshot.spectrum.lowMidBandDb, 1) +
                           " dB");
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Presence: " + juce::String(m_snapshot.spectrum.presenceBandDb, 1) +
                           " dB | Air: " + juce::String(m_snapshot.spectrum.airBandDb, 1) + " dB");
        drawMetricLine(g, bounds.removeFromTop(18),
                       "Mid energy: " + juce::String(m_snapshot.stereo.midEnergy, 2) +
                           " | Side energy: " + juce::String(m_snapshot.stereo.sideEnergy, 2));
    }

    g.setColour(dawai::ui::theme::Palette::textSecondary());
    g.setFont(dawai::ui::theme::monoFont().withHeight(12.0f));
    drawMetricLine(g, bounds.removeFromTop(16),
                   m_mode == Mode::Reference
                       ? "Reference mode only: target corridor is active."
                       : (m_mode == Mode::Compare
                              ? "Compare mode avoids reference-pool logic and stays A/B."
                              : "Analyze mode uses live-buffer metrics only."));
}

} // namespace dawai::ui
