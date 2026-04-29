#include "CloudSeamanor/domain/BuffSystem.hpp"

#include <algorithm>
#include <sstream>

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

std::string BuffSystem::SaveState() const {
    std::ostringstream out;
    bool first = true;
    for (const auto& [id, buff] : active_buffs_) {
        if (!first) {
            out << ";";
        }
        first = false;
        out << id << ","
            << buff.remaining_seconds << ","
            << buff.stamina_recovery_multiplier << ","
            << buff.stamina_cost_multiplier;
    }
    return out.str();
}

void BuffSystem::LoadState(const std::string& encoded_state) {
    active_buffs_.clear();
    if (encoded_state.empty()) {
        return;
    }
    std::istringstream row_stream(encoded_state);
    std::string row;
    while (std::getline(row_stream, row, ';')) {
        if (row.empty()) {
            continue;
        }
        std::istringstream field_stream(row);
        std::string id;
        std::string remain;
        std::string recover;
        std::string cost;
        if (!std::getline(field_stream, id, ',')) continue;
        if (!std::getline(field_stream, remain, ',')) continue;
        if (!std::getline(field_stream, recover, ',')) continue;
        if (!std::getline(field_stream, cost, ',')) continue;
        try {
            RuntimeBuff buff;
            buff.id = id;
            buff.remaining_seconds = std::stof(remain);
            buff.stamina_recovery_multiplier = std::stof(recover);
            buff.stamina_cost_multiplier = std::stof(cost);
            if (!buff.id.empty() && buff.remaining_seconds > 0.0f) {
                active_buffs_[buff.id] = buff;
            }
        } catch (...) {
        }
    }
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
