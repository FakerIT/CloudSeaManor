#pragma once
// ============================================================================
// 【RelationshipSystem】告白 / 婚礼 / 婚后关系系统（领域层）
// ============================================================================
// Responsibilities:
// - 管理关系终局状态机（告白→订婚→婚礼→婚后）
// - 提供触发条件判定与状态迁移 API（不依赖 engine / IO）
// - 输出可用于 UI 的简要状态文本
// ============================================================================

#include "CloudSeamanor/domain/BuffSystem.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/FestivalSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"

#include <string>

namespace CloudSeamanor::domain {

enum class RelationshipStage : unsigned char {
    Single = 0,
    Close = 1,
    ConfessionAvailable = 2,
    Engaged = 3,
    WeddingScheduled = 4,
    Married = 5,
};

struct RelationshipState {
    RelationshipStage stage = RelationshipStage::Single;
    std::string target_npc_id;  // 仅支持单一关系终局（MVP）

    // 冷却与里程碑（按绝对天数计）
    int confession_cooldown_until_day = 0;  // <= day 表示可尝试
    int engaged_day = 0;
    int wedding_day = 0;
    int married_since_day = 0;
    bool wedding_ceremony_done = false;
    int post_wedding_event_cursor = 0;
};

struct ConfessionRules {
    int favor_threshold = 1000;
    int heart_threshold = 10;
    int start_hour = 18;
    int end_hour = 22;  // [start, end)
    int cooldown_days_on_fail = 3;
    int favor_penalty_on_fail = 30;
    std::string token_item_id = "JadeRing";  // MVP：用已有物品作为“告白信物”
};

struct WeddingRules {
    int min_schedule_days_ahead = 2;
    int gold_cost = 200;
    std::string gift_item_id = "TeaPack";
    int gift_item_count = 1;
};

enum class ConfessionResult : unsigned char {
    Accepted = 0,
    Rejected = 1,
    NotAllowed = 2
};

class RelationshipSystem {
public:
    RelationshipSystem() = default;

    [[nodiscard]] const ConfessionRules& Confession() const noexcept { return confession_; }
    [[nodiscard]] const WeddingRules& Wedding() const noexcept { return wedding_; }

    [[nodiscard]] bool CanOfferConfession(
        const RelationshipState& state,
        int current_day,
        int current_hour,
        CloudState weather,
        int npc_favor,
        int npc_heart_level,
        bool has_token_item) const noexcept;

    [[nodiscard]] ConfessionResult TryConfess(
        RelationshipState& state,
        int current_day,
        int npc_favor,
        int npc_heart_level) const noexcept;

    // 预约婚礼：根据节日冲突自动顺延。
    // 返回 0 表示失败；否则返回最终 wedding_day（绝对天数）。
    [[nodiscard]] int TryScheduleWedding(
        RelationshipState& state,
        int current_day,
        int player_gold,
        int tea_pack_count,
        const FestivalSystem& festivals) const;

    // 每日推进：处理婚礼日结算、每日婚后增益等。
    // 返回 true 表示状态发生重要变化（如结婚）。
    [[nodiscard]] bool OnBeginNewDay(RelationshipState& state, int new_day) const noexcept;

    void ApplyDailyMarriageBuff(const RelationshipState& state, BuffSystem& buffs) const;

    [[nodiscard]] static std::string StageText(RelationshipStage stage);

private:
    ConfessionRules confession_{};
    WeddingRules wedding_{};
};

}  // namespace CloudSeamanor::domain

