#include "dawai/aifr3d_core/apply_fix.hpp"

namespace dawai::aifr3d_core
{

bool ActionUndoStack::perform(const SuggestionAction& action, MixActionContext& context)
{
    switch (action.type)
    {
    case SuggestionActionType::AddPlugin:
        context.pluginChains[action.trackIndex].push_back(action.pluginId);
        return true;
    case SuggestionActionType::SetParam:
        context.parameters[action.pluginId + ":" + action.parameterId] = action.value;
        return true;
    case SuggestionActionType::CreateAutomationPoint:
        context
            .parameters[action.pluginId + ":automation:" + std::to_string(action.timelineSeconds)] =
            action.value;
        return true;
    }
    return false;
}

bool ActionUndoStack::preview(const SuggestionAction& action, MixActionContext context) const
{
    return perform(action, context);
}

bool ActionUndoStack::apply(const SuggestionAction& action, MixActionContext& context,
                            bool userApproved)
{
    if (!userApproved)
    {
        return false;
    }

    m_history.push_back(context);
    return perform(action, context);
}

bool ActionUndoStack::undo(MixActionContext& context)
{
    if (m_history.empty())
    {
        return false;
    }

    context = m_history.back();
    m_history.pop_back();
    return true;
}

std::vector<SuggestionAction> PresetGenerator::conservativeChain(int trackIndex) const
{
    return {
        {SuggestionActionType::AddPlugin, trackIndex, "eq.basic", "", 0.0, 0.0, false},
        {SuggestionActionType::AddPlugin, trackIndex, "mbcomp.basic", "", 0.0, 0.0, false},
        {SuggestionActionType::AddPlugin, trackIndex, "saturator.soft", "", 0.0, 0.0, true},
        {SuggestionActionType::SetParam, trackIndex, "eq.basic", "low_cut_hz", 30.0, 0.0, false},
        {SuggestionActionType::SetParam, trackIndex, "mbcomp.basic", "ratio", 1.8, 0.0, false},
        {SuggestionActionType::SetParam, trackIndex, "saturator.soft", "drive", 0.15, 0.0, true},
    };
}

} // namespace dawai::aifr3d_core
