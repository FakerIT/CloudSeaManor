#include "../TestFramework.hpp"
#include "CloudSeamanor/engine/GameRuntime.hpp"

#include <string>

using CloudSeamanor::engine::GameRuntime;
using CloudSeamanor::engine::GameRuntimeCallbacks;

namespace {

GameRuntimeCallbacks BuildCountingCallbacks(int& daily_summary_count) {
    GameRuntimeCallbacks cbs;
    cbs.push_hint = [](const std::string&, float) {};
    cbs.push_notification = [&daily_summary_count](const std::string& msg) {
        if (msg.find("日切汇总：") != std::string::npos) {
            ++daily_summary_count;
        }
    };
    cbs.log_info = [](const std::string&) {};
    cbs.play_sfx = [](const std::string&) {};
    cbs.play_bgm = [](const std::string&, bool, float, float) {};
    cbs.update_hud_text = []() {};
    cbs.refresh_window_title = []() {};
    return cbs;
}

}  // namespace

TEST_CASE(SleepToNextMorning_advances_day_and_triggers_daily_settlement_once) {
    int daily_summary_count = 0;
    GameRuntime runtime;
    runtime.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildCountingCallbacks(daily_summary_count));

    runtime.WorldState().MutableClock().SetState(5, 22.0f * 60.0f);
    const int day_before = runtime.WorldState().GetClock().Day();

    (void)runtime.SleepToNextMorning();

    const int day_after = runtime.WorldState().GetClock().Day();
    ASSERT_EQ(day_after, day_before + 1);
    ASSERT_EQ(daily_summary_count, 1);
    return true;
}

