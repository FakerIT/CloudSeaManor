#include "../TestFramework.hpp"
#include "CloudSeamanor/GameRuntime.hpp"

#include <fstream>
#include <filesystem>

using CloudSeamanor::engine::GameRuntime;
using CloudSeamanor::engine::GameRuntimeCallbacks;

namespace {

GameRuntimeCallbacks BuildNoopCallbacks() {
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

int CountRuleMail(const GameRuntime& runtime, const std::string& rule_id) {
    int count = 0;
    for (const auto& mail : runtime.WorldState().GetMailOrders()) {
        if (mail.source_rule_id == rule_id) {
            ++count;
        }
    }
    return count;
}

void CleanupSlot(int slot_index) {
    const auto dir = std::filesystem::path("saves") / ("slot_" + std::to_string(slot_index));
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return {};
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out << content;
    return true;
}

}  // namespace

TEST_CASE(MailRuleOnce_persists_across_save_and_load) {
    constexpr int kSlot = 3;
    const std::string kRuleId = "test_once_rule";
    CleanupSlot(kSlot);
    const auto rule_table_path = std::filesystem::path("assets/data/MailRuleTable.csv");
    const bool original_exists = std::filesystem::exists(rule_table_path);
    const std::string original_rule_table = ReadTextFile(rule_table_path);
    const std::string test_rule_table =
        "id,trigger_type,trigger_arg,item_id,count,delay_days,hint_text,enabled,sender_template,subject_template,body_template,cooldown_policy\n"
        "test_once_rule,day_in_season_eq,2,TeaPack,1,0,测试规则触发,1,测试发件人,测试标题,测试正文,once\n";
    std::filesystem::create_directories(rule_table_path.parent_path());
    ASSERT_TRUE(WriteTextFile(rule_table_path, test_rule_table));

    GameRuntime runtime_a;
    runtime_a.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildNoopCallbacks());
    runtime_a.WorldState().MutableClock().SetState(1, 360.0f);
    runtime_a.WorldState().MutableClock().AdvanceDay();
    runtime_a.OnDayChanged();
    const int triggered_count_before_save = CountRuleMail(runtime_a, kRuleId);
    ASSERT_TRUE(triggered_count_before_save >= 1);

    ASSERT_TRUE(runtime_a.SaveGameToSlot(kSlot));

    GameRuntime runtime_b;
    runtime_b.Initialize(
        "configs/gameplay.cfg",
        "assets/data/Schedule_Data.csv",
        "assets/data/Gift_Preference.json",
        "assets/data/NPC_Texts.json",
        "assets/maps/prototype_farm.tmx",
        BuildNoopCallbacks());
    ASSERT_TRUE(runtime_b.LoadGameFromSlot(kSlot));

    const int triggered_count_after_load = CountRuleMail(runtime_b, kRuleId);
    ASSERT_EQ(triggered_count_after_load, triggered_count_before_save);

    // 读档后跨天再次日切，once 规则不应重复触发。
    runtime_b.WorldState().MutableClock().AdvanceDay();
    runtime_b.OnDayChanged();
    const int triggered_count_after_next_day = CountRuleMail(runtime_b, kRuleId);
    ASSERT_EQ(triggered_count_after_next_day, triggered_count_after_load);

    if (original_exists) {
        ASSERT_TRUE(WriteTextFile(rule_table_path, original_rule_table));
    } else {
        std::error_code ec;
        std::filesystem::remove(rule_table_path, ec);
    }
    CleanupSlot(kSlot);
    return true;
}

