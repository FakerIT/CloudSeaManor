#include "CloudSeamanor/BuffSystem.hpp"

#include <algorithm>

namespace CloudSeamanor::domain {

void BuffSystem::ApplyBuff(const RuntimeBuff& buff) {
    if (buff.id.empty() || buff.remaining_seconds <= 0.0f) {
        return;
    }
    active_buffs_[buff.id] = buff;
}

void BuffSystem::Tick(float delta_seconds) {
    if (delta_seconds <= 0.0f) {
        return;
    }
    for (auto it = active_buffs_.begin(); it != active_buffs_.end();) {
        it->second.remaining_seconds -= delta_seconds;
        if (it->second.remaining_seconds <= 0.0f) {
            it = active_buffs_.erase(it);
        } else {
            ++it;
        }
    }
}

void BuffSystem::Clear() {
    active_buffs_.clear();
}

float BuffSystem::StaminaRecoveryMultiplier() const noexcept {
    float result = 1.0f;
    for (const auto& [_, buff] : active_buffs_) {
        result *= std::max(0.0f, buff.stamina_recovery_multiplier);
    }
    return result;
}

float BuffSystem::StaminaCostMultiplier() const noexcept {
    float result = 1.0f;
    for (const auto& [_, buff] : active_buffs_) {
        result *= std::max(0.0f, buff.stamina_cost_multiplier);
    }
    return result;
}

}  // namespace CloudSeamanor::domain
