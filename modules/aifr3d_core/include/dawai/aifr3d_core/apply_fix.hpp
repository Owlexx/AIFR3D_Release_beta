#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dawai::aifr3d_core
{

enum class SuggestionActionType
{
    AddPlugin,
    SetParam,
    CreateAutomationPoint
};

struct SuggestionAction
{
    SuggestionActionType type = SuggestionActionType::AddPlugin;
    int trackIndex = 0;
    std::string pluginId;
    std::string parameterId;
    double value = 0.0;
    double timelineSeconds = 0.0;
    bool boldMove = false;
};

struct MixActionContext
{
    std::unordered_map<int, std::vector<std::string>> pluginChains;
    std::unordered_map<std::string, double> parameters;
};

class ActionUndoStack
{
  public:
    bool preview(const SuggestionAction& action, MixActionContext context) const;
    bool apply(const SuggestionAction& action, MixActionContext& context, bool userApproved);
    bool undo(MixActionContext& context);

  private:
    static bool perform(const SuggestionAction& action, MixActionContext& context);

  private:
    std::vector<MixActionContext> m_history;
};

class PresetGenerator
{
  public:
    [[nodiscard]] std::vector<SuggestionAction> conservativeChain(int trackIndex) const;
};

} // namespace dawai::aifr3d_core
