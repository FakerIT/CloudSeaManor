#include "CloudSeamanor/RelationshipSystem.hpp"

#include <algorithm>

namespace CloudSeamanor::domain {

namespace {

bool IsWeatherAllowedForConfession_(CloudState s) {
    switch (s) {
    case CloudState::Clear:
    case CloudState::Mist:
        return true;
    case CloudState::DenseCloud:
    case CloudState::Tide:
        return false;
    }
    return false;
}

int DayInYearFromAbsolute_(int day) {
    constexpr int kDaysPerYear = 28 * 4;
    return ((std::max(1, day) - 1) % kDaysPerYear) + 1;
}

int SeasonIndexFromAbsolute_(int day) {
    const int day_in_year = DayInYearFromAbsolute_(day);
    return (day_in_year - 1) / 28;
}

int DayInSeasonFromAbsolute_(int day) {
    const int day_in_year = DayInYearFromAbsolute_(day);
    return ((day_in_year - 1) % 28) + 1;
}

bool IsFestivalOnAbsoluteDay_(const FestivalSystem& festivals, int absolute_day) {
    const int season_index = SeasonIndexFromAbsolute_(absolute_day);
    const int day_in_season = DayInSeasonFromAbsolute_(absolute_day);
    for (const auto& fest : festivals.GetAllFestivals()) {
        if (fest.season == season_index && fest.day == day_in_season) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool RelationshipSystem::CanOfferConfession(
    const RelationshipState& state,
    int current_day,
    int current_hour,
    CloudState weather,
    int npc_favor,
    int npc_heart_level,
    bool has_token_item) const noexcept {
    if (!state.target_npc_id.empty() && state.stage == RelationshipStage::Married) {
        return false;
    }
    if (current_day < state.confession_cooldown_until_day) {
        return false;
    }
    if (current_hour < confession_.start_hour || current_hour >= confession_.end_hour) {
        return false;
    }
    if (!IsWeatherAllowedForConfession_(weather)) {
        return false;
    }
    if (npc_favor < confession_.favor_threshold) {
        return false;
    }
    if (npc_heart_level < confession_.heart_threshold) {
        return false;
    }
    if (!has_token_item) {
        return false;
    }
    if (!state.target_npc_id.empty() && state.target_npc_id != "ANY") {
        // 已有目标（订婚/婚礼进行中），不对其他 NPC 开放告白。
        if (state.stage == RelationshipStage::Engaged
            || state.stage == RelationshipStage::WeddingScheduled) {
            return false;
        }
    }
    return true;
}

ConfessionResult RelationshipSystem::TryConfess(
    RelationshipState& state,
    int current_day,
    int npc_favor,
    int npc_heart_level) const noexcept {
    if (current_day < state.confession_cooldown_until_day) {
        return ConfessionResult::NotAllowed;
    }
    if (npc_favor < confession_.favor_threshold || npc_heart_level < confession_.heart_threshold) {
        return ConfessionResult::NotAllowed;
    }
    // MVP：条件满足即成功（后续可加入随机、性格、剧情旗标等）
    state.stage = RelationshipStage::Engaged;
    state.engaged_day = std::max(1, current_day);
    return ConfessionResult::Accepted;
}

int RelationshipSystem::TryScheduleWedding(
    RelationshipState& state,
    int current_day,
    int player_gold,
    int tea_pack_count,
    const FestivalSystem& festivals) const {
    if (state.stage != RelationshipStage::Engaged) {
        return 0;
    }
    if (player_gold < wedding_.gold_cost) {
        return 0;
    }
    if (tea_pack_count < wedding_.gift_item_count) {
        return 0;
    }
    int desired = std::max(1, current_day) + std::max(0, wedding_.min_schedule_days_ahead);
    // 节日冲突自动顺延：最多顺延 14 天，避免死循环（数据异常时兜底）。
    for (int i = 0; i < 14; ++i) {
        if (!IsFestivalOnAbsoluteDay_(festivals, desired)) {
            break;
        }
        ++desired;
    }
    state.stage = RelationshipStage::WeddingScheduled;
    state.wedding_day = desired;
    return desired;
}

bool RelationshipSystem::OnBeginNewDay(RelationshipState& state, int new_day) const noexcept {
    if (state.stage == RelationshipStage::WeddingScheduled
        && state.wedding_day > 0
        && new_day >= state.wedding_day) {
        state.stage = RelationshipStage::Married;
        state.married_since_day = std::max(1, new_day);
        return true;
    }
    return false;
}

void RelationshipSystem::ApplyDailyMarriageBuff(const RelationshipState& state, BuffSystem& buffs) const {
    if (state.stage != RelationshipStage::Married) {
        return;
    }
    RuntimeBuff buff;
    buff.id = "marriage_daily_bonus";
    // 以每日刷新实现“长期效果”，避免永久 buff 的存档耦合。
    buff.remaining_seconds = 60.0f * 60.0f * 24.0f;
    buff.stamina_recovery_multiplier = 1.05f;
    buff.stamina_cost_multiplier = 0.95f;
    buffs.ApplyBuff(buff);
}

std::string RelationshipSystem::StageText(RelationshipStage stage) {
    switch (stage) {
    case RelationshipStage::Single: return "单身";
    case RelationshipStage::Close: return "亲密";
    case RelationshipStage::ConfessionAvailable: return "可告白";
    case RelationshipStage::Engaged: return "订婚";
    case RelationshipStage::WeddingScheduled: return "婚礼已预约";
    case RelationshipStage::Married: return "已婚";
    }
    return "未知";
}

}  // namespace CloudSeamanor::domain

