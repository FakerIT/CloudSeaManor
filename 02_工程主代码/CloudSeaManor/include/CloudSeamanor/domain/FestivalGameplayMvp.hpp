#pragma once

#include <string>
#include <utility>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【FestivalAutoRewardSpec】节日日切自动奖励与当日 Buff 描述（领域层）
// ============================================================================
struct FestivalAutoRewardSpec {
    int gold = 0;
    int stamina = 0;
    std::vector<std::pair<std::string, int>> grant_items;
    std::vector<std::string> hint_lines;
    int favor_all_npcs = 0; // >0 时对每位 NPC 增加好感（由 engine 做上限裁剪）

    bool set_qingming_double_spirit = false;
    bool set_flower_bloom_visual = false;
    int mid_autumn_regen_bonus_until_day = -1; // inclusive
    bool set_double_ninth_chrysanthemum = false;
    bool set_harvest_sell_bonus = false;
    bool set_tea_culture_contest = false;
    bool set_winter_polar_night = false;
};

// ============================================================================
// 【BuildFestivalAutoRewardSpec】根据节日 id 生成“当日首次”自动礼包规则
// ============================================================================
// @param festival_id 来自 FestivalSystem / CSV
// @param game_day 当前游戏日（GameClock::Day）
// @param year 当前游戏年（GameClock::Year）
// ============================================================================
[[nodiscard]] FestivalAutoRewardSpec BuildFestivalAutoRewardSpec(
    const std::string& festival_id,
    int game_day,
    int year);

}  // namespace CloudSeamanor::domain
