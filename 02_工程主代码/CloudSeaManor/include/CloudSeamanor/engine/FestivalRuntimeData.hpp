#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【FestivalRuntimeData】节日玩法运行时数据（跨日 buff / 当日领奖去重）
// ============================================================================
// 与 FestivalSystem.participated（剧情/成就用“是否参与过”）解耦：
// - mvp_auto_reward_last_day_by_festival_：按游戏日去重“日切自动礼包”
// - 其它 int 字段：记录 buff 生效的游戏日或截止日（含端点）
// ============================================================================
struct FestivalRuntimeData {
    std::unordered_map<std::string, int> mvp_auto_reward_last_day_by_festival;

    int qingming_double_spirit_gather_day = -1;
    int flower_bloom_visual_day = -1;
    int mid_autumn_regen_bonus_until_day = -1;
    int double_ninth_chrysanthemum_day = -1;
    int harvest_festival_sell_bonus_day = -1;
    int tea_culture_contest_day = -1;
    int winter_solstice_polar_anchor_day = -1;

    void Reset();

    void ClearDayScopedVisuals_(int new_game_day);
    void OnBeginNewDay(int new_game_day);

    void AppendSaveLines(std::vector<std::string>& lines) const;
    // @return true if the line was recognized and consumed
    bool TryConsumeSaveLine(const std::string& tag, const std::vector<std::string>& fields);
};

}  // namespace CloudSeamanor::engine
