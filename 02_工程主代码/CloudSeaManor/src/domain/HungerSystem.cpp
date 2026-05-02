#include "CloudSeamanor/domain/HungerSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::domain {

// ============================================================================
// 【Initialize】
// ============================================================================
void HungerSystem::Initialize(const int current, const int max_value) {
    max_ = std::max(1, max_value);
    current_ = std::clamp(current, 0, max_);
}

// ============================================================================
// 【SetGameClock】
// ============================================================================
void HungerSystem::SetGameClock(const GameClock* clock) {
    clock_ = clock;
}

// ============================================================================
// 【Tick】每帧更新（白天每小时-1点饱食度）
// ============================================================================
void HungerSystem::Tick(const float delta_seconds) {
    accumulated_seconds_ += delta_seconds;

    if (accumulated_seconds_ < kHungerTickInterval) {
        return;
    }

    // 仅在白天（6:00~22:00）消耗饱食度
    if (clock_) {
        const int hour = clock_->Hour();
        if (hour >= 6 && hour < 22) {
            const int minutes_elapsed = static_cast<int>(accumulated_seconds_ / kHungerTickInterval);
            Consume(minutes_elapsed * kHungerPerHour);
        }
    }

    accumulated_seconds_ = 0.0f;
}

// ============================================================================
// 【Consume】
// ============================================================================
void HungerSystem::Consume(const int amount) {
    if (amount <= 0) return;
    current_ = std::max(0, current_ - amount);
}

// ============================================================================
// 【Restore】
// ============================================================================
void HungerSystem::Restore(const int amount) {
    if (amount <= 0) return;
    current_ = std::min(max_, current_ + amount);
}

// ============================================================================
// 【RestoreFromFood】
// ============================================================================
void HungerSystem::RestoreFromFood(const int base_restore, const float quality_multiplier) {
    const int restored = static_cast<int>(base_restore * quality_multiplier);
    Restore(restored);
}

// ============================================================================
// 【RefillForNewDay】起床后重置饱食度
// ============================================================================
void HungerSystem::RefillForNewDay() {
    current_ = 80;
    has_eaten_breakfast_ = false;
}

// ============================================================================
// 【ResetDailyState】
// ============================================================================
void HungerSystem::ResetDailyState() {
    current_ = 80;
    has_eaten_breakfast_ = false;
    accumulated_seconds_ = 0.0f;
}

// ============================================================================
// 【State】
// ============================================================================
HungerState HungerSystem::State() const noexcept {
    if (current_ >= 80) return HungerState::Satiated;
    if (current_ >= 40) return HungerState::Normal;
    if (current_ >= 20) return HungerState::Hungry;
    return HungerState::Starving;
}

// ============================================================================
// 【StaminaRecoveryMultiplier】
// ============================================================================
float HungerSystem::StaminaRecoveryMultiplier() const noexcept {
    switch (State()) {
    case HungerState::Satiated:  return 1.20f;  // 饱足：+20%
    case HungerState::Normal:    return 1.00f;  // 正常：无影响
    case HungerState::Hungry:   return 0.70f;  // 饥饿：-30%
    case HungerState::Starving:  return 0.00f;  // 空腹：停止恢复
    }
    return 1.0f;
}

// ============================================================================
// 【StaminaCostMultiplier】
// ============================================================================
float HungerSystem::StaminaCostMultiplier() const noexcept {
    switch (State()) {
    case HungerState::Satiated:  return 1.00f;  // 无惩罚
    case HungerState::Normal:    return 1.00f;  // 无惩罚
    case HungerState::Hungry:   return 1.15f;  // +15%
    case HungerState::Starving:  return 1.30f;  // +30%
    }
    return 1.0f;
}

// ============================================================================
// 【HungerHint】
// ============================================================================
std::string HungerSystem::HungerHint() const {
    switch (State()) {
    case HungerState::Satiated:
        return "饱足状态，体力恢复速度+20%";
    case HungerState::Normal:
        return "饱食状态良好";
    case HungerState::Hungry:
        return "有些饿了，体力恢复变慢（-30%）";
    case HungerState::Starving:
        return "急需进食！体力恢复停止，消耗+30%";
    }
    return "";
}

// ============================================================================
// 【SaveState】
// ============================================================================
std::string HungerSystem::SaveState() const {
    std::ostringstream oss;
    oss << current_ << "|" << max_ << "|" << (has_eaten_breakfast_ ? 1 : 0);
    return oss.str();
}

// ============================================================================
// 【LoadState】
// ============================================================================
void HungerSystem::LoadState(const std::string& state) {
    if (state.empty()) return;
    std::istringstream iss(state);
    std::string token;
    if (std::getline(iss, token, '|')) {
        try {
            current_ = std::stoi(token);
        } catch (...) {
            current_ = 80;
        }
    }
    if (std::getline(iss, token, '|')) {
        try {
            max_ = std::stoi(token);
        } catch (...) {
            max_ = 100;
        }
    }
    if (std::getline(iss, token, '|')) {
        try {
            has_eaten_breakfast_ = (std::stoi(token) != 0);
        } catch (...) {
            has_eaten_breakfast_ = false;
        }
    }
    ApplyClamp_();
}

// ============================================================================
// 【ApplyClamp_】
// ============================================================================
void HungerSystem::ApplyClamp_() {
    if (current_ < 0) current_ = 0;
    if (current_ > max_) current_ = max_;
    if (max_ < 1) max_ = 1;
}

} // namespace CloudSeamanor::domain
