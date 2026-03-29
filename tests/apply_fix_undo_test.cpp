#include "dawai/aifr3d_core/apply_fix.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Apply fix preview/apply/undo works", "[apply-fix]")
{
    dawai::aifr3d_core::MixActionContext context;
    dawai::aifr3d_core::ActionUndoStack undo;

    dawai::aifr3d_core::SuggestionAction addPlugin;
    addPlugin.type = dawai::aifr3d_core::SuggestionActionType::AddPlugin;
    addPlugin.trackIndex = 0;
    addPlugin.pluginId = "eq.basic";

    REQUIRE(undo.preview(addPlugin, context));
    REQUIRE(undo.apply(addPlugin, context, true));
    REQUIRE(context.pluginChains[0].size() == 1);

    REQUIRE(undo.undo(context));
    REQUIRE(context.pluginChains[0].empty());
}

TEST_CASE("Preset generation creates conservative chain", "[apply-fix]")
{
    dawai::aifr3d_core::PresetGenerator generator;
    const auto actions = generator.conservativeChain(0);

    REQUIRE(actions.size() >= 3);
    REQUIRE(actions[0].type == dawai::aifr3d_core::SuggestionActionType::AddPlugin);
}
