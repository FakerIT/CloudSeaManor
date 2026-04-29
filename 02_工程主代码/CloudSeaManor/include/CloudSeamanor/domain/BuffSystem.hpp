#pragma once

#include <string>
#include <unordered_map>

namespace CloudSeamanor::domain {

struct RuntimeBuff {
    std::string id;
    float remaining_seconds = 0.0f;
    float stamina_recovery_multiplier = 1.0f;
    float stamina_cost_multiplier = 1.0f;
};

class BuffSystem {
public:
    void ApplyBuff(const RuntimeBuff& buff);
    void Tick(float delta_seconds);
    void Clear();
    [[nodiscard]] std::string SaveState() const;
    void LoadState(const std::string& encoded_state);

    [[nodiscard]] float StaminaRecoveryMultiplier() const noexcept;
    [[nodiscard]] float StaminaCostMultiplier() const noexcept;

private:
    std::unordered_map<std::string, RuntimeBuff> active_buffs_;
};

}  // namespace CloudSeamanor::domain
