#include "DiagnosticEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
std::string formatDb(float value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << value;
    return stream.str();
}

bool hasReferenceDelta(float value)
{
    return std::isfinite(value);
}

bool referenceEvaluationAvailable(const MixMetrics& metrics)
{
    return metrics.referenceStateCode == kReferenceStateReady && metrics.referenceValid;
}

bool scoredReferenceAvailable(const MixMetrics& metrics)
{
    return referenceEvaluationAvailable(metrics) && metrics.scoredResultValid;
}

float liveMeasurementConfidence(const MixMetrics& metrics)
{
    const float windowConfidence =
        metrics.analysisStateCode == kAnalysisStateValidScored ? 1.0f
        : metrics.analysisStateCode == kAnalysisStateActiveLive ? 0.72f
                                                                : 0.38f;
    return std::clamp(0.25f + 0.35f * windowConfidence + 0.20f * metrics.dynamics.transientDensity +
                          0.20f * metrics.stereo.subMonoIntegrity,
                      0.0f, 1.0f);
}

std::string liveToneLabel(const MixMetrics& metrics)
{
    const float mud = metrics.tonal.lowMidBandDb - metrics.tonal.midBandDb;
    const float harsh = metrics.tonal.highMidBandDb - metrics.tonal.midBandDb;
    const float bassGap = metrics.tonal.midBandDb - metrics.tonal.subBandDb;
    const float thinness =
        (metrics.tonal.midBandDb - metrics.tonal.lowBandDb) +
        (metrics.tonal.highMidBandDb - metrics.tonal.lowMidBandDb);

    if (mud > 3.0f)
    {
        return "Low-mids building";
    }
    if (harsh > 3.0f)
    {
        return "High-mids forward";
    }
    if (bassGap > 8.0f)
    {
        return "Low end light";
    }
    if (thinness > 8.0f)
    {
        return "Tone thinning";
    }
    return "Live tonal measurement";
}

std::string toneLabelForLargestDelta(const MixMetrics& metrics)
{
    if (!scoredReferenceAvailable(metrics))
    {
        return liveToneLabel(metrics);
    }

    const std::array<std::pair<const char*, float>, 6> entries{{
        {"Sub", metrics.referenceDeltas.subBandDb},
        {"Low-End", metrics.referenceDeltas.lowBandDb},
        {"Low-Mids", metrics.referenceDeltas.lowMidBandDb},
        {"Midrange", metrics.referenceDeltas.midBandDb},
        {"High-Mids", metrics.referenceDeltas.highMidBandDb},
        {"High-End", metrics.referenceDeltas.highBandDb},
    }};

    auto best = entries.front();
    for (const auto& entry : entries)
    {
        if (std::abs(entry.second) > std::abs(best.second))
        {
            best = entry;
        }
    }

    if (std::abs(best.second) < 1.2f)
    {
        return "Tonal balance near target";
    }
    return std::string(best.first) + (best.second > 0.0f ? " heavy" : " light");
}

std::string tonalSummaryText(const MixMetrics& metrics)
{
    if (scoredReferenceAvailable(metrics))
    {
        return "Sub " + formatDb(metrics.referenceDeltas.subBandDb) + " dB | Low " +
               formatDb(metrics.referenceDeltas.lowBandDb) + " dB | Low-mid " +
               formatDb(metrics.referenceDeltas.lowMidBandDb) + " dB | Mid " +
               formatDb(metrics.referenceDeltas.midBandDb) + " dB | High-mid " +
               formatDb(metrics.referenceDeltas.highMidBandDb) + " dB | High " +
               formatDb(metrics.referenceDeltas.highBandDb) + " dB vs reference";
    }

    if (!metrics.liveSignalPresent)
    {
        return "Waiting for live program material.";
    }

    return "Live bands | Sub " + formatDb(metrics.tonal.subBandDb) + " dB | Low " +
           formatDb(metrics.tonal.lowBandDb) + " dB | Low-mid " +
           formatDb(metrics.tonal.lowMidBandDb) + " dB | Mid " +
           formatDb(metrics.tonal.midBandDb) + " dB | High-mid " +
           formatDb(metrics.tonal.highMidBandDb) + " dB | High " +
           formatDb(metrics.tonal.highBandDb) + " dB";
}

bool hasIssueId(const AifredReport& report, const std::string& id)
{
    return std::any_of(report.issues.begin(), report.issues.end(), [&](const auto& issue)
                       { return issue.id == id; });
}

void addIssue(AifredReport& report, std::string id, std::string category, std::string title,
              std::string explanation, std::string suggestedAction, std::string impact)
{
    report.issues.push_back({std::move(id), std::move(category), std::move(title),
                             std::move(explanation), std::move(suggestedAction),
                             std::move(impact), report.confidence});
}

int impactRank(const std::string& impact)
{
    const auto lowered = impact;
    if (lowered == "high")
    {
        return 0;
    }
    if (lowered == "medium")
    {
        return 1;
    }
    return 2;
}

int issuePriority(const std::string& id)
{
    if (id == "reference_blocked")
    {
        return 0;
    }
    if (id == "reference_incomplete")
    {
        return 1;
    }
    if (id == "low_end_phase")
    {
        return 2;
    }
    if (id == "stereo_phase")
    {
        return 3;
    }
    if (id == "loudness_clip" || id == "loudness_hot")
    {
        return 4;
    }
    if (id == "stereo_collapse")
    {
        return 5;
    }
    if (id == "dynamic_flattening" || id == "dynamic_range_low")
    {
        return 6;
    }
    if (id == "mud_lowmid" || id == "harsh_highs" || id == "missing_bass" ||
        id == "tinny_balance")
    {
        return 7;
    }
    if (id == "reference_drift")
    {
        return 8;
    }
    if (id == "translation_good")
    {
        return 99;
    }
    return 50;
}

const char* analysisStateText(std::uint8_t stateCode)
{
    switch (stateCode)
    {
    case kAnalysisStateIdle:
        return "idle";
    case kAnalysisStateBufferWarming:
        return "buffer-warming";
    case kAnalysisStateActiveLive:
        return "active-live";
    case kAnalysisStateValidScored:
        return "valid-scored";
    case kAnalysisStateInvalidReference:
        return "invalid-reference";
    case kAnalysisStateReferenceBlocked:
        return "reference-blocked";
    default:
        return "idle";
    }
}
} // namespace

AifredReport DiagnosticEngine::interpret(const MixMetrics& metrics)
{
    AifredReport report;
    report.overallAlignment = metrics.scoring.mixAlignment;
    report.confidence =
        scoredReferenceAvailable(metrics) ? metrics.scoring.signalClarity
                                          : liveMeasurementConfidence(metrics);
    report.flags = metrics.flags;
    report.tonalSummary = tonalSummaryText(metrics);
    report.haloLabel = toneLabelForLargestDelta(metrics);
    report.haloDetail = report.tonalSummary;
    report.summaryStatus = "Listening...";
    report.analysisState = analysisStateText(metrics.analysisStateCode);
    report.trendStatus = scoredReferenceAvailable(metrics)
                             ? (metrics.scoring.mixAlignment < 0.45f ? "drifting"
                                : metrics.scoring.mixAlignment < 0.65f ? "adjusting"
                                                                       : "stable")
                             : "live-only";

    if (metrics.analysisStateCode == kAnalysisStateIdle)
    {
        report.haloLabel = "No Signal";
        report.haloDetail = "Play program material to start analysis.";
        report.summaryStatus = "No valid program material";
        report.tonalSummary = "Analyzer idle until signal arrives.";
        return report;
    }

    if (metrics.analysisStateCode == kAnalysisStateBufferWarming)
    {
        report.haloLabel = "Buffer Warming";
        report.haloDetail = "Signal detected. Gathering enough audio for a stable read.";
        report.summaryStatus = "Analysis window warming";
        report.tonalSummary = "Waiting for enough program material before scoring.";
        return report;
    }

    if (metrics.analysisStateCode == kAnalysisStateActiveLive)
    {
        report.haloLabel = "Live Analysis";
        report.haloDetail = "Raw tone, stereo, and loudness are updating before full scoring locks.";
        report.summaryStatus = "Live metrics updating";
        report.tonalSummary = "Live analyzer active. Measurements are running before the stable window locks.";
        return report;
    }

    const bool referenceReady = referenceEvaluationAvailable(metrics);
    if (!referenceReady)
    {
        report.haloLabel = "Live Measurement";
        report.haloDetail = "Meters are measuring the realtime buffer. Reference evaluation is not active.";
        report.summaryStatus = metrics.referenceStateCode == kReferenceStateBlocked
                                   ? "Reference source blocked. Live measurement mode active."
                                   : "Reference data unavailable. Live measurement mode active.";
        if (metrics.referenceStateCode == kReferenceStateBlocked)
        {
            addIssue(report, "reference_blocked", "Reference", "Reference source blocked",
                     "The canonical reference source failed validation, so target scoring is paused while live meters stay active.",
                     "Remove contaminated references and reload the curated canonical manifest.",
                     "medium");
        }
        else
        {
            addIssue(report, "reference_incomplete", "Reference", "Reference data unavailable",
                     "Target corridors are not available, so the plugin has switched to live measurement mode.",
                     "Restore the canonical reference pool to re-enable corridor colors and scoring.",
                     "medium");
        }
    }

    if ((metrics.flags & kFlagPhaseRisk) != 0)
    {
        report.haloLabel = "Stereo Too Unstable";
        report.haloDetail = "Correlation and mono integrity are dropping below target.";
        report.summaryStatus = "Phase risk detected";
        addIssue(report, "stereo_phase", "Width/Depth", "Stereo field is unstable",
                 "Phase risk and correlation drift show mono compatibility is weakening.",
                 "Reduce widening and tighten low-end stereo spread until correlation recovers.",
                 "high");
    }
    if (metrics.stereo.lowBandPhaseRisk > 0.20f || metrics.stereo.lowBandCorrelation < 0.15f)
    {
        addIssue(report, "low_end_phase", "Low End", "Low end mono integrity is at risk",
                 "Sub-band correlation is dropping and the low-frequency stereo field may cancel in mono.",
                 "Collapse sub energy toward center and narrow bass widening below 120 Hz.",
                 metrics.stereo.lowBandPhaseRisk > 0.34f ? "high" : "medium");
    }
    else if ((metrics.flags & kFlagClipRisk) != 0)
    {
        report.haloLabel = "Headroom Too Tight";
        report.haloDetail = "True peak is pushing past the safe mastering window.";
        report.summaryStatus = "Clip / true-peak risk";
        addIssue(report, "loudness_clip", "Loudness", "Headroom is too tight",
                 "True peak is too close to 0 dBTP for the current loudness target.",
                 "Back off limiter drive or output gain until true peak returns below -1 dBTP.",
                 "high");
    }
    else if ((metrics.flags & kFlagWidthCollapse) != 0)
    {
        report.haloLabel = "Stereo Too Narrow";
        report.haloDetail = "Side energy is falling away from the live reference target.";
        report.summaryStatus = "Stereo width under target";
        addIssue(report, "stereo_collapse", "Width/Depth", "Stereo image collapsed",
                 "Side energy is too low and the width corridor is under target.",
                 "Rebuild width with panning, ambience, or stereo information above the low end.",
                 "high");
    }
    else if ((metrics.flags & kFlagTransientSoftening) != 0)
    {
        report.haloLabel = "Transient Punch Down";
        report.haloDetail = "Crest factor and transient density are both slipping.";
        report.summaryStatus = "Transient impact softening";
        addIssue(report, "dynamic_flattening", "Dynamics", "Dynamics are flattening",
                 "Crest factor and transient density show the mix is being over-controlled.",
                 "Ease limiting or bus compression so peaks and attack can recover.",
                 "high");
    }
    else if (scoredReferenceAvailable(metrics) && metrics.scoring.mixAlignment < 0.45f)
    {
        report.haloLabel = "Off Target";
        report.haloDetail = "The live mix is drifting away from the selected reference pool.";
        report.summaryStatus = "Needs alignment to selected reference pool";
    }
    else if (scoredReferenceAvailable(metrics) && metrics.scoring.mixAlignment < 0.65f)
    {
        report.haloLabel = "Slightly Off Target";
        report.summaryStatus = "Working toward translation target";
    }

    switch (metrics.frequencyBalance.statusCode)
    {
    case kFrequencyBalanceStatusMuddy:
        addIssue(report, "mud_lowmid", "Low-Mids", "Mud is building up",
                 "Low-mid energy is above the target balance and can mask clarity and separation.",
                 "Trim 250-500 Hz on crowded buses or competing instruments.",
                 metrics.frequencyBalance.severity >= 2 ? "high" : "medium");
        break;
    case kFrequencyBalanceStatusHarsh:
    case kFrequencyBalanceStatusSibilant:
        addIssue(report, "harsh_highs", "High End", "Harshness is above target",
                 "High-mid or presence energy is exceeding the reference balance corridor.",
                 "Reduce aggressive 2-6 kHz or 6-12 kHz emphasis until the mix relaxes.",
                 "high");
        break;
    case kFrequencyBalanceStatusBassThin:
    case kFrequencyBalanceStatusSubLacking:
        addIssue(report, "missing_bass", "Low End", "Low end is under target",
                 "Sub and bass energy are trailing the target balance and the mix may feel small.",
                 "Recover controlled weight in the 20-250 Hz region with kick and bass balance.",
                 "high");
        break;
    case kFrequencyBalanceStatusLowMidHollow:
    case kFrequencyBalanceStatusMidScooped:
        addIssue(report, "tinny_balance", "Tonal Balance", "Tone is getting thin",
                 "Body energy is under target and the center of the mix is hollowing out.",
                 "Restore low-mid and mid body before adding more top-end brightness.",
                 "medium");
        break;
    default:
        break;
    }

    if (!hasIssueId(report, "mud_lowmid") && hasReferenceDelta(metrics.referenceDeltas.lowMidBandDb) &&
        metrics.referenceDeltas.lowMidBandDb > 2.5f)
    {
        addIssue(report, "mud_lowmid", "Low-Mids", "Mud is building up",
                 "Low-mid energy is above the reference target and can mask clarity.",
                 "Trim 200-800 Hz build-up on the bus or competing instruments.",
                 metrics.referenceDeltas.lowMidBandDb > 4.0f ? "high" : "medium");
    }

    if (!hasIssueId(report, "harsh_highs") &&
        ((hasReferenceDelta(metrics.referenceDeltas.highMidBandDb) &&
          metrics.referenceDeltas.highMidBandDb > 3.0f) ||
         (hasReferenceDelta(metrics.referenceDeltas.highBandDb) &&
          metrics.referenceDeltas.highBandDb > 4.0f)))
    {
        addIssue(report, "harsh_highs", "High End", "Harshness is above target",
                 "Upper-mid or high-frequency energy is exceeding the reference corridor.",
                 "Reduce aggressive 2-6 kHz or top-end boost until the tone relaxes.",
                 "high");
    }

    if (!hasIssueId(report, "missing_bass") &&
        ((hasReferenceDelta(metrics.referenceDeltas.subBandDb) &&
          metrics.referenceDeltas.subBandDb < -3.0f) ||
         (hasReferenceDelta(metrics.referenceDeltas.lowBandDb) &&
          metrics.referenceDeltas.lowBandDb < -2.5f)))
    {
        addIssue(report, "missing_bass", "Low End", "Low end is under target",
                 "Sub and bass energy are trailing the reference and the mix may feel small.",
                 "Recover controlled weight in the 20-200 Hz region with kick/bass balance.",
                 "high");
    }

    if (!hasIssueId(report, "tinny_balance") &&
        hasReferenceDelta(metrics.referenceDeltas.lowBandDb) &&
        hasReferenceDelta(metrics.referenceDeltas.lowMidBandDb) &&
        hasReferenceDelta(metrics.referenceDeltas.highMidBandDb) &&
        metrics.referenceDeltas.lowBandDb < -1.5f && metrics.referenceDeltas.lowMidBandDb < -1.0f &&
        metrics.referenceDeltas.highMidBandDb > 1.5f)
    {
        addIssue(report, "tinny_balance", "Tonal Balance", "Tone is getting thin",
                 "The body of the mix is under target while upper presence stays elevated.",
                 "Restore low and low-mid body before adding more top-end.",
                 "medium");
    }

    if (metrics.loudness.statusCode == kIntegratedLoudnessStatusCriticallyLoud ||
        metrics.loudness.statusCode == kIntegratedLoudnessStatusTooLoud ||
        metrics.loudness.statusCode == kIntegratedLoudnessStatusSlightlyLoud ||
        metrics.loudness.truePeakDbTP > -1.0f)
    {
        addIssue(report, "loudness_hot", "Loudness", "Loudness is running hot",
                 "Integrated LUFS and peak ceiling are both pushing beyond the safer target zone.",
                 "Lower limiter drive or output until LUFS and true peak return to target.",
                 "high");
    }
    else if (metrics.loudness.statusCode == kIntegratedLoudnessStatusTooQuiet ||
             metrics.loudness.statusCode == kIntegratedLoudnessStatusCriticallyQuiet)
    {
        addIssue(report, "loudness_low", "Loudness", "Loudness is under target",
                 "Integrated LUFS is sitting well below the target corridor for the active release context.",
                 "Add measured makeup gain after restoring punch and headroom control.",
                 "medium");
    }

    if (metrics.dynamics.statusCode == kDynamicRangeStatusCompressed ||
        metrics.dynamics.statusCode == kDynamicRangeStatusHeavilyCompressed ||
        metrics.dynamics.statusCode == kDynamicRangeStatusBrickWalled)
    {
        addIssue(report, "dynamic_range_low", "Dynamics", "Dynamic range is collapsing",
                 "The loudest moments are too close to the average level, which flattens contrast and impact.",
                 "Reduce limiter pressure or bus compression so peaks and section contrast can recover.",
                 metrics.dynamics.statusCode == kDynamicRangeStatusBrickWalled ? "high" : "medium");
    }

    if (scoredReferenceAvailable(metrics) && metrics.scoring.mixAlignment < 0.55f)
    {
        addIssue(report, "reference_drift", "Reference", "Reference deviation is visible",
                 "The overall alignment score shows meaningful drift from the active reference target.",
                 "Prioritize the largest tonal or stereo delta before fine-detail moves.",
                 metrics.scoring.mixAlignment < 0.45f ? "high" : "medium");
    }

    if (report.issues.empty())
    {
        addIssue(report, "translation_good", "Translation",
                 scoredReferenceAvailable(metrics) ? "Mix is translating well"
                                                   : "Live measurement looks stable",
                 scoredReferenceAvailable(metrics)
                     ? "Core loudness, tone, stereo, and dynamics metrics are within the working corridor."
                     : "Realtime loudness, stereo, tone, and dynamics measurements are stable with no dominant warning.",
                 scoredReferenceAvailable(metrics)
                     ? "Make smaller moves only and preserve the current balance."
                     : "Continue monitoring the live meters or restore the reference pool for target evaluation.",
                 "medium");
    }

    std::stable_sort(report.issues.begin(), report.issues.end(),
                     [](const AifredIssue& lhs, const AifredIssue& rhs)
                     {
                         const int lhsImpact = impactRank(lhs.impact);
                         const int rhsImpact = impactRank(rhs.impact);
                         if (lhsImpact != rhsImpact)
                         {
                             return lhsImpact < rhsImpact;
                         }

                         const int lhsPriority = issuePriority(lhs.id);
                         const int rhsPriority = issuePriority(rhs.id);
                         if (lhsPriority != rhsPriority)
                         {
                             return lhsPriority < rhsPriority;
                         }

                         if (std::abs(lhs.confidence - rhs.confidence) > 1.0e-4f)
                         {
                             return lhs.confidence > rhs.confidence;
                         }

                         return lhs.title < rhs.title;
                     });

    if (!report.issues.empty())
    {
        report.haloLabel = report.issues.front().title;
        report.haloDetail = report.issues.front().explanation;
        report.confidence = report.issues.front().confidence;
        if (report.issues.front().impact == "high")
        {
            report.summaryStatus = "Priority issue: " + report.issues.front().title;
        }
    }

    return report;
}
