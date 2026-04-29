#include "CloudSeamanor/domain/ManorEcologySystem.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace CloudSeamanor::domain {

ManorEcologySystem::ManorEcologySystem() {
    // 初始化为平稳中性生态
    state_.aura_level = 50;
    state_.spirit_beast_activity = 50;
    state_.element_weights = {20, 20, 20, 20, 20};  // 五元素均匀分布
    state_.balance_score = 100;
    state_.balanced_days_streak = 0;
    state_.dominant_element_index = -1;
    state_.current_stage = EcologyStage::Stable;
    state_.balance_state = BalanceState::NearBalanced;
    state_.total_events_triggered = 0;
}

void ManorEcologySystem::OnDayChanged(int current_day) {
    // 1. 结算每日基础变化
    // 灵气浓度每日自然衰减（庄园需要维护）
    if (state_.aura_level > 30) {
        state_.aura_level -= 1;
    }

    // 2. 元素权重每日向平衡方向回归一点
    const int target_weight = 20;  // 中性值
    for (int i = 0; i < kManorElementCount; ++i) {
        if (state_.element_weights[i] > target_weight) {
            state_.element_weights[i] -= 1;
        } else if (state_.element_weights[i] < target_weight) {
            state_.element_weights[i] += 1;
        }
    }

    // 3. 重新计算派生状态
    RecalculateDerivedState_();

    // 4. 更新平衡天数
    UpdateBalanceDays_();

    // 5. 检查并触发祥瑞事件
    // （祥瑞事件在 CheckAuspiciousEvent 中处理）
}

void ManorEcologySystem::ApplyDelta(const EcologyDeltaEvent& event) {
    // 灵气浓度变化
    state_.aura_level = std::clamp(
        state_.aura_level + event.aura_delta,
        kMinAura, kMaxAura);

    // 元素权重变化
    const int idx = static_cast<int>(event.primary_element);
    if (idx >= 0 && idx < kManorElementCount) {
        state_.element_weights[idx] = std::clamp(
            state_.element_weights[idx] + event.element_delta,
            kElementWeightMin, kElementWeightMax);
    }

    // 重新计算派生状态
    RecalculateDerivedState_();
}

void ManorEcologySystem::ApplyDelta(ManorElement element, int aura_delta, int element_delta) {
    EcologyDeltaEvent event;
    event.primary_element = element;
    event.aura_delta = aura_delta;
    event.element_delta = element_delta;
    ApplyDelta(event);
}

void ManorEcologySystem::LoadPlantingEffects(const std::vector<PlantingEffect>& effects) {
    planting_effects_ = effects;
}

void ManorEcologySystem::LoadDecorationEffects(const std::vector<DecorationEffect>& effects) {
    decoration_effects_ = effects;
}

void ManorEcologySystem::LoadAuspiciousEvents(const std::vector<AuspiciousEvent>& events) {
    auspicious_events_ = events;
    event_cooldowns_.clear();
    for (const auto& e : events) {
        EcologyEventRecord rec;
        rec.event_id = e.id;
        rec.last_trigger_day = 0;
        rec.cooldown_remaining_days = 0;
        event_cooldowns_.push_back(rec);
    }
}

void ManorEcologySystem::LoadNpcComments(const std::vector<NpcEcologyComment>& comments) {
    npc_comments_ = comments;
}

const PlantingEffect* ManorEcologySystem::GetPlantingEffect(const std::string& tea_id) const {
    for (const auto& e : planting_effects_) {
        if (e.tea_id == tea_id) {
            return &e;
        }
    }
    return nullptr;
}

const DecorationEffect* ManorEcologySystem::GetDecorationEffect(const std::string& decor_id) const {
    for (const auto& e : decoration_effects_) {
        if (e.decor_id == decor_id) {
            return &e;
        }
    }
    return nullptr;
}

EcologyStage ManorEcologySystem::GetAuraStage() const {
    return state_.current_stage;
}

BalanceState ManorEcologySystem::GetBalanceState() const {
    return state_.balance_state;
}

std::string ManorEcologySystem::GetStageDescription() const {
    return ToDisplayName(state_.current_stage);
}

std::string ManorEcologySystem::GetBalanceDescription() const {
    if (state_.dominant_element_index >= 0 && state_.dominant_element_index < kManorElementCount) {
        const char* element_name = ToDisplayName(
            static_cast<ManorElement>(state_.dominant_element_index));
        return std::string(element_name) + "意略盛";
    }
    return ToDisplayName(state_.balance_state);
}

std::string ManorEcologySystem::GetEcologySummary() const {
    std::ostringstream oss;
    oss << "庄园灵气" << ToDisplayName(state_.current_stage);
    oss << "，" << GetBalanceDescription();
    if (state_.balanced_days_streak >= 3) {
        oss << "。五气相和，似有祥瑞将至。";
    }
    return oss.str();
}

ManorElement ManorEcologySystem::GetDominantElement() const {
    if (state_.dominant_element_index >= 0 && state_.dominant_element_index < kManorElementCount) {
        return static_cast<ManorElement>(state_.dominant_element_index);
    }
    return ManorElement::Cloud;
}

float ManorEcologySystem::GetQualityBonus(ManorElement tea_element) const {
    const int idx = static_cast<int>(tea_element);
    if (idx < 0 || idx >= kManorElementCount) {
        return 1.0f;  // 未知元素，无加成
    }

    // 基础品质系数
    float bonus = 1.0f;

    // 灵气充盈以上有基础加成
    if (state_.current_stage == EcologyStage::Abundant) {
        bonus += 0.05f;
    } else if (state_.current_stage == EcologyStage::Luminous) {
        bonus += 0.10f;
    }

    // 主导元素匹配加成
    if (state_.dominant_element_index == idx) {
        bonus += 0.03f;
    }

    // 完美平衡对所有元素都有小幅加成
    if (state_.balance_state == BalanceState::PerfectlyBalanced) {
        bonus += 0.02f;
    }

    return bonus;
}

std::string ManorEcologySystem::CheckAuspiciousEvent(int current_day) {
    for (size_t i = 0; i < auspicious_events_.size(); ++i) {
        const auto& evt = auspicious_events_[i];
        auto& cooldown = event_cooldowns_[i];

        // 检查冷却
        if (cooldown.cooldown_remaining_days > 0) {
            cooldown.cooldown_remaining_days--;
            continue;
        }

        // 检查灵气阶段
        if (state_.current_stage < evt.min_aura_stage) {
            continue;
        }

        // 检查平衡状态
        bool balance_ok = false;
        switch (evt.required_balance) {
            case BalanceState::PerfectlyBalanced:
                balance_ok = (state_.balance_state == BalanceState::PerfectlyBalanced);
                break;
            case BalanceState::NearBalanced:
                balance_ok = (state_.balance_state == BalanceState::NearBalanced
                           || state_.balance_state == BalanceState::PerfectlyBalanced);
                break;
            case BalanceState::SingleElementDominant:
                balance_ok = (state_.dominant_element_index >= 0);
                if (balance_ok && evt.min_dominant_element_weight > 0) {
                    balance_ok = (state_.element_weights[state_.dominant_element_index]
                               >= evt.min_dominant_element_weight);
                }
                break;
            default:
                balance_ok = true;
        }
        if (!balance_ok) {
            continue;
        }

        // 检查平衡天数要求
        if (evt.required_balance_days > 0) {
            if (state_.balanced_days_streak < evt.required_balance_days) {
                continue;
            }
        }

        // 触发成功
        cooldown.last_trigger_day = current_day;
        cooldown.cooldown_remaining_days = evt.cooldown_days;
        state_.total_events_triggered++;

        return evt.id;
    }
    return {};
}

std::vector<std::string> ManorEcologySystem::GetNpcComments(int min_heart_level) const {
    std::vector<std::string> result;
    for (const auto& comment : npc_comments_) {
        if (comment.min_heart_level > min_heart_level) {
            continue;
        }
        if (comment.ecology_stage != state_.current_stage
            && comment.ecology_stage != EcologyStage::Stable) {  // Stable 可以匹配所有
            continue;
        }
        if (comment.dominant_element != GetDominantElement()
            && comment.dominant_element != ManorElement::Cloud) {  // Cloud 可以作为默认值
            continue;
        }
        result.push_back(comment.text);
    }
    return result;
}

std::string ManorEcologySystem::Serialize() const {
    std::ostringstream oss;
    oss << "aura=" << state_.aura_level;
    oss << ",activity=" << state_.spirit_beast_activity;
    for (int i = 0; i < kManorElementCount; ++i) {
        oss << ",e" << i << "=" << state_.element_weights[i];
    }
    oss << ",balance=" << state_.balance_score;
    oss << ",balanced_days=" << state_.balanced_days_streak;
    oss << ",events=" << state_.total_events_triggered;
    return oss.str();
}

void ManorEcologySystem::Deserialize(const std::string& data) {
    if (data.empty()) {
        return;  // 旧存档没有数据，保持默认状态
    }

    // 简单解析 key=value,key=value 格式
    std::istringstream iss(data);
    std::string token;
    while (std::getline(iss, token, ',')) {
        auto pos = token.find('=');
        if (pos == std::string::npos) continue;

        std::string key = token.substr(0, pos);
        std::string val = token.substr(pos + 1);

        if (key == "aura") state_.aura_level = std::stoi(val);
        else if (key == "activity") state_.spirit_beast_activity = std::stoi(val);
        else if (key == "balance") state_.balance_score = std::stoi(val);
        else if (key == "balanced_days") state_.balanced_days_streak = std::stoi(val);
        else if (key == "events") state_.total_events_triggered = std::stoi(val);
        else if (key.size() == 2 && key[0] == 'e') {
            int idx = key[1] - '0';
            if (idx >= 0 && idx < kManorElementCount) {
                state_.element_weights[idx] = std::stoi(val);
            }
        }
    }

    RecalculateDerivedState_();
}

void ManorEcologySystem::RecalculateDerivedState_() {
    UpdateBalanceScore_();
    state_.dominant_element_index = CalculateDominantElement_();
    state_.balance_state = CalculateBalanceState_();
    state_.current_stage = CalculateAuraStage_();
}

void ManorEcologySystem::UpdateBalanceScore_() {
    // 计算平衡分数：元素权重越接近均匀，平衡分数越高
    const int target = 20;  // 中性值
    int total_diff = 0;
    for (int i = 0; i < kManorElementCount; ++i) {
        total_diff += std::abs(state_.element_weights[i] - target);
    }
    // 最大差值情况下 diff = 400，转换为 0-100 分数
    state_.balance_score = std::max(0, 100 - total_diff / 4);
}

void ManorEcologySystem::UpdateBalanceDays_() {
    if (state_.balance_state == BalanceState::NearBalanced
        || state_.balance_state == BalanceState::PerfectlyBalanced) {
        state_.balanced_days_streak++;
    } else {
        state_.balanced_days_streak = 0;
    }
}

int ManorEcologySystem::CalculateDominantElement_() const {
    int max_weight = -1;
    int max_idx = -1;
    for (int i = 0; i < kManorElementCount; ++i) {
        // 只有当元素权重显著高于平均值时才认为是主导
        if (state_.element_weights[i] > max_weight && state_.element_weights[i] > 25) {
            max_weight = state_.element_weights[i];
            max_idx = i;
        }
    }
    return max_idx;
}

BalanceState ManorEcologySystem::CalculateBalanceState_() const {
    // 计算权重离散度
    int max_diff = 0;
    int min_weight = 100;
    for (int i = 0; i < kManorElementCount; ++i) {
        max_diff = std::max(max_diff, std::abs(state_.element_weights[i] - 20));
        min_weight = std::min(min_weight, state_.element_weights[i]);
    }

    if (max_diff <= kPerfectBalanceThreshold) {
        return BalanceState::PerfectlyBalanced;
    }
    if (max_diff <= kNearBalanceThreshold) {
        return BalanceState::NearBalanced;
    }
    if (min_weight < 10) {
        return BalanceState::SingleElementDominant;
    }
    return BalanceState::DualElementResonant;
}

EcologyStage ManorEcologySystem::CalculateAuraStage_() const {
    if (state_.aura_level <= 25) {
        return EcologyStage::Withered;
    }
    if (state_.aura_level <= 60) {
        return EcologyStage::Stable;
    }
    if (state_.aura_level <= 85) {
        return EcologyStage::Abundant;
    }
    return EcologyStage::Luminous;
}

}  // namespace CloudSeamanor::domain
