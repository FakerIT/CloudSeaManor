#pragma once

// ============================================================================
// 【NpcDeliverySystem】NPC 每日委托系统
// ============================================================================
// Responsibilities:
// - 从 CSV 加载委托池（NpcDeliveryTable.csv）
// - 每日 6:00 生成 1-3 个 NPC 委托并注入 RuntimeQuest
// - 监测完成条件（背包数量满足目标）并标记为 Completed（不直接发奖）
// - 与 NPC 对话时结算奖励（金币 + 好感），并将任务标记为 Claimed
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【QuestDelivery】委托定义（由表驱动）
// ============================================================================
struct QuestDelivery {
    std::string npc_id;
    std::string quest_id;
    std::string title;
    std::string description;
    std::string item_id;
    int item_count = 0;
    int reward_gold = 0;
    int reward_favor = 0;
};

class NpcDeliverySystem {
public:
    using HintCallback = std::function<void(const std::string&, float)>;

    /**
     * @brief 从 CSV 加载委托池
     * @param path CSV 路径（建议 assets/data/NpcDeliveryTable.csv）
     */
    bool LoadFromCsv(const std::string& path);

    /**
     * @brief 每帧更新（轻量），在 6:00 触发当日刷新，并更新完成状态
     */
    void Update(GameWorldState& world_state, int current_day, int current_hour);

    /**
     * @brief 绑定提示回调
     */
    void SetHintCallback(HintCallback cb) { hint_callback_ = std::move(cb); }

    /**
     * @brief 与指定 NPC 对话时尝试领取已完成的委托奖励
     * @return true 表示成功领取过至少一个奖励
     */
    bool TryClaimRewards(
        GameWorldState& world_state,
        const std::string& npc_id
    );

private:
    void RefreshDaily_(
        std::vector<RuntimeQuest>& quests,
        int day
    );
    void EvaluateCompletion_(
        std::vector<RuntimeQuest>& quests,
        const CloudSeamanor::domain::Inventory& inventory
    );

    [[nodiscard]] static std::optional<QuestDelivery> ParseQuestId_(const std::string& quest_id);
    [[nodiscard]] static std::string BuildQuestId_(
        const std::string& npc_id,
        const std::string& item_id,
        int item_count,
        int reward_gold,
        int reward_favor
    );

    std::vector<QuestDelivery> pool_;
    HintCallback hint_callback_;
    int last_refresh_day_ = -1;
};

} // namespace CloudSeamanor::engine

