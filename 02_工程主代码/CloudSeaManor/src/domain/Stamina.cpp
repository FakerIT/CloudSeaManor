#include "CloudSeamanor/domain/Stamina.hpp"

#include <algorithm>

namespace CloudSeamanor::domain {

// ============================================================================
// 【Initialize】从配置初始化体力系统
// ============================================================================
void StaminaSystem::Initialize(
    float current,
    float max_value,
    int protection_days,
    float protection_modifier
) {
    // 设置保护期参数
    protection_days_ = protection_days;
    protection_modifier_ = protection_modifier;

    // 设置上限和当前值
    SetMax(max_value);
    SetCurrent(current);
}

// ============================================================================
// 【SetGameDay】设置当前游戏天数
// ============================================================================
void StaminaSystem::SetGameDay(int day) {
    // 天数必须为正数
    if (day < 1) {
        return;
    }
    game_day_ = day;
}

// ============================================================================
// 【Consume】消耗体力（应用新手保护倍率）
// ============================================================================
void StaminaSystem::Consume(float amount) {
    if (amount <= 0.0f) {
        return;
    }

    // 新手保护期消耗减半
    float actual_amount = amount * GetConsumptionModifier();
    ConsumeRaw(actual_amount);
}

// ============================================================================
// 【ConsumeRaw】原始消耗（不受新手保护影响）
// ============================================================================
void StaminaSystem::ConsumeRaw(float amount) {
    if (amount <= 0.0f) {
        return;
    }

    current_ = std::max(0.0f, current_ - amount);
}

// ============================================================================
// 【Recover】恢复体力
// ============================================================================
void StaminaSystem::Recover(float amount) {
    if (amount <= 0.0f) {
        return;
    }

    current_ = std::min(max_, current_ + amount);
}

// ============================================================================
// 【RecoverPerSecond】按时间恢复体力
// ============================================================================
void StaminaSystem::RecoverPerSecond(float amount_per_second, float delta_time) {
    if (amount_per_second <= 0.0f || delta_time <= 0.0f) {
        return;
    }

    float recovered = amount_per_second * delta_time;
    Recover(recovered);
}

// ============================================================================
// 【Refill】完全恢复体力到上限
// ============================================================================
void StaminaSystem::Refill() {
    current_ = max_;
}

// ============================================================================
// 【Update】每帧更新
// ============================================================================
void StaminaSystem::Update(float delta_time, float recover_per_second) {
    RecoverPerSecond(recover_per_second, delta_time);
}

// ============================================================================
// 【SetMax】设置体力上限
// ============================================================================
void StaminaSystem::SetMax(float max_value) {
    if (max_value <= 0.0f) {
        return;
    }

    max_ = max_value;

    // 确保当前值不超过新上限
    ApplyClamp_();
}

// ============================================================================
// 【SetCurrent】设置当前体力
// ============================================================================
void StaminaSystem::SetCurrent(float current_value) {
    current_ = std::clamp(current_value, 0.0f, max_);
}

// ============================================================================
// 【IsInProtectionPeriod】是否处于新手保护期
// ============================================================================
bool StaminaSystem::IsInProtectionPeriod() const noexcept {
    return game_day_ <= protection_days_;
}

// ============================================================================
// 【CanAfford】是否能负担指定体力消耗
// ============================================================================
bool StaminaSystem::CanAfford(float amount) const noexcept {
    if (amount <= 0.0f) {
        return true;
    }

    // 在保护期时，消耗减半，所以检查时也应该考虑这个因素
    // 但为了简化，保护期判断仍以实际消耗为准
    return current_ >= (amount * GetConsumptionModifier());
}

// ============================================================================
// 【GetConsumptionModifier】获取当前消耗倍率
// ============================================================================
float StaminaSystem::GetConsumptionModifier() const noexcept {
    return IsInProtectionPeriod() ? protection_modifier_ : 1.0f;
}

// ============================================================================
// 【ApplyClamp_】内部：将当前体力限制在合法范围内
// ============================================================================
void StaminaSystem::ApplyClamp_() {
    if (current_ < 0.0f) {
        current_ = 0.0f;
    } else if (current_ > max_) {
        current_ = max_;
    }
}

}  // namespace CloudSeamanor::domain
