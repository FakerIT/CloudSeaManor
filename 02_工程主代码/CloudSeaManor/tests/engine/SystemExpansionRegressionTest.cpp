#include "../TestFramework.hpp"
#include "CloudSeamanor/engine/GameRuntime.hpp"
#include "CloudSeamanor/infrastructure/DataRegistry.hpp"

#include <filesystem>

using CloudSeamanor::engine::GameRuntime;
using CloudSeamanor::engine::GameRuntimeCallbacks;
using CloudSeamanor::engine::DiaryEntryState;
using CloudSeamanor::engine::PlacedObject;

namespace {

GameRuntimeCallbacks BuildSilentCallbacks_() {
    GameRuntimeCallbacks cbs;
    cbs.push_hint = [](const std::string&, float) {};
    cbs.push_notification = [](const std::string&) {};
    cbs.log_info = [](const std::string&) {};
    cbs.play_sfx = [](const std::string&) {};
    cbs.play_bgm = [](const std::string&, bool, float, float) {};
    cbs.update_hud_text = []() {};
    cbs.refresh_window_title = []() {};
    return cbs;
}

void CleanupSlot_(int slot_index) {
    const auto dir = std::filesystem::path("saves") / ("slot_" + std::to_string(slot_index));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

}  // namespace

TEST_CASE(SystemExpansion_diary_unlocks_from_runtime_conditions) {
    GameRuntime runtime;
    runtime.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildSilentCallbacks_());

    runtime.WorldState().MutableInventory().AddItem("TeaPack", 1);
    runtime.WorldState().MutableClock().AdvanceDay();
    runtime.OnDayChanged();

    const auto& entries = runtime.WorldState().GetDiaryEntries();
    ASSERT_TRUE(!entries.empty());
    ASSERT_EQ(entries.front().entry_id, "harvest_first");
    return true;
}

TEST_CASE(SystemExpansion_skill_branch_enters_pending_choice_at_level_10_on_day_change) {
    GameRuntime runtime;
    runtime.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildSilentCallbacks_());

    runtime.Systems().GetSkills().GetSkill(CloudSeamanor::domain::SkillType::SpiritFarm).level = 10;
    runtime.WorldState().MutableClock().AdvanceDay();
    runtime.OnDayChanged();

    const auto& pending = runtime.WorldState().GetPendingSkillBranches();
    ASSERT_TRUE(!pending.empty());
    ASSERT_EQ(pending.front(), "灵农");
    return true;
}

TEST_CASE(SystemExpansion_save_load_roundtrip_preserves_new_expansion_fields) {
    constexpr int kSlot = 4;
    CleanupSlot_(kSlot);

    GameRuntime runtime_a;
    runtime_a.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildSilentCallbacks_());

    runtime_a.WorldState().MutableDiaryEntries().push_back(DiaryEntryState{"favor_bond", 6, true});
    runtime_a.WorldState().MutableSkillBranches()["灵农"] = "abundance";
    runtime_a.WorldState().MutablePlacedObjects().push_back(
        PlacedObject{"tea_room_decor", 2, 3, 90, "tea_room", "test"});
    runtime_a.WorldState().MutableRecipeUnlocks()["cook_tea_egg"] = true;
    runtime_a.WorldState().MutablePurifyReturnDays() = 2;
    runtime_a.WorldState().MutablePurifyReturnSpirits() = 3;
    runtime_a.WorldState().MutableFishingAttempts() = 5;
    runtime_a.WorldState().MutableLastFishCatch() = "cloud_koi";
    ASSERT_TRUE(runtime_a.SaveGameToSlot(kSlot));

    GameRuntime runtime_b;
    runtime_b.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildSilentCallbacks_());
    ASSERT_TRUE(runtime_b.LoadGameFromSlot(kSlot));

    ASSERT_EQ(runtime_b.WorldState().GetDiaryEntries().size(), static_cast<std::size_t>(1));
    ASSERT_EQ(runtime_b.WorldState().GetDiaryEntries().front().entry_id, "favor_bond");
    ASSERT_EQ(runtime_b.WorldState().GetSkillBranches().at("灵农"), "abundance");
    ASSERT_EQ(runtime_b.WorldState().GetPlacedObjects().size(), static_cast<std::size_t>(1));
    ASSERT_EQ(runtime_b.WorldState().GetFishingAttempts(), 5);
    ASSERT_EQ(runtime_b.WorldState().GetLastFishCatch(), "cloud_koi");
    ASSERT_EQ(runtime_b.WorldState().GetPurifyReturnDays(), 2);

    CleanupSlot_(kSlot);
    return true;
}

TEST_CASE(SystemExpansion_data_registry_registers_paths_in_order) {
    CloudSeamanor::infrastructure::DataRegistry registry;
    registry.RegisterPath("recipes", "assets/data/recipes");
    registry.RegisterPath("fishing", "assets/data/fishing");
    registry.RegisterPath("skills", "assets/data/skills");

    ASSERT_EQ(registry.Entries().size(), static_cast<std::size_t>(3));
    ASSERT_EQ(registry.Entries()[0].category, "recipes");
    ASSERT_EQ(registry.Entries()[1].path, "assets/data/fishing");
    ASSERT_EQ(registry.Entries()[2].category, "skills");
    return true;
}
