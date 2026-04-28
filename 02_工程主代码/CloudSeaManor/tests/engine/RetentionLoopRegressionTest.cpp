#include "../TestFramework.hpp"
#include "CloudSeamanor/GameRuntime.hpp"

#include <filesystem>
#include <string>
#include <vector>

using CloudSeamanor::engine::GameRuntime;
using CloudSeamanor::engine::GameRuntimeCallbacks;

namespace {

struct RuntimeProbe {
    int daily_summary_count = 0;
    int weekly_summary_count = 0;
    std::vector<std::string> notifications;
};

GameRuntimeCallbacks BuildProbeCallbacks(RuntimeProbe& probe) {
    GameRuntimeCallbacks cbs;
    cbs.push_hint = [](const std::string&, float) {};
    cbs.push_notification = [&probe](const std::string& msg) {
        probe.notifications.push_back(msg);
        if (msg.find("日切汇总：") != std::string::npos) {
            ++probe.daily_summary_count;
        }
        if (msg.find("本周经营报告：") != std::string::npos) {
            ++probe.weekly_summary_count;
        }
    };
    cbs.log_info = [](const std::string&) {};
    cbs.play_sfx = [](const std::string&) {};
    cbs.play_bgm = [](const std::string&, bool, float, float) {};
    cbs.update_hud_text = []() {};
    cbs.refresh_window_title = []() {};
    return cbs;
}

void CleanupSlotForRetention_(int slot_index) {
    const auto dir = std::filesystem::path("saves") / ("slot_" + std::to_string(slot_index));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

}  // namespace

TEST_CASE(Retention_day_cycle_summary_is_once_per_day_transition) {
    RuntimeProbe probe;
    GameRuntime runtime;
    runtime.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildProbeCallbacks(probe));

    runtime.WorldState().MutableClock().SetState(1, 360.0f);
    runtime.WorldState().MutableClock().AdvanceDay();
    runtime.OnDayChanged();
    runtime.WorldState().MutableClock().AdvanceDay();
    runtime.OnDayChanged();

    ASSERT_EQ(probe.daily_summary_count, 2);
    return true;
}

TEST_CASE(Retention_midterm_goal_commitment_persists_via_world_progress_save_load) {
    constexpr int kSlot = 2;
    CleanupSlotForRetention_(kSlot);

    RuntimeProbe probe_a;
    GameRuntime runtime_a;
    runtime_a.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildProbeCallbacks(probe_a));

    // Use persistent world progress (main house level) as commitment carrier.
    runtime_a.WorldState().MutableMainHouseRepair().level = 2;
    ASSERT_TRUE(runtime_a.SaveGameToSlot(kSlot));

    RuntimeProbe probe_b;
    GameRuntime runtime_b;
    runtime_b.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildProbeCallbacks(probe_b));
    ASSERT_TRUE(runtime_b.LoadGameFromSlot(kSlot));
    ASSERT_EQ(runtime_b.WorldState().GetMainHouseRepair().level, 2);

    runtime_b.WorldState().MutableClock().AdvanceDay();
    runtime_b.OnDayChanged();
    ASSERT_TRUE(probe_b.daily_summary_count >= 1);

    CleanupSlotForRetention_(kSlot);
    return true;
}

TEST_CASE(Retention_weekly_summary_generated_consistently_on_day7_and_day14) {
    RuntimeProbe probe;
    GameRuntime runtime;
    runtime.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildProbeCallbacks(probe));

    runtime.WorldState().MutableClock().SetState(1, 360.0f);
    for (int i = 0; i < 14; ++i) {
        runtime.WorldState().MutableClock().AdvanceDay();
        runtime.OnDayChanged();
    }

    ASSERT_EQ(probe.weekly_summary_count, 2);
    return true;
}

