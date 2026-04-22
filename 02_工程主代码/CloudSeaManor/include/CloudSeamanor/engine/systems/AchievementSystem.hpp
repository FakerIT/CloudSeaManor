#pragma once

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/EventBus.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace CloudSeamanor::engine {

class AchievementSystem {
public:
    bool UnlockOnce(
        std::unordered_map<std::string, bool>& achievements,
        const std::string& id,
        const std::string& title,
        const std::function<void(const std::string&)>& notify,
        const std::function<void(const std::string& achievement_id)>& grant_reward = nullptr) const;

    void EvaluateDaily(
        std::unordered_map<std::string, bool>& achievements,
        int decoration_score,
        int gold,
        const SpiritBeast& spirit_beast,
        const std::function<void(const std::string&)>& notify,
        const std::function<void(const std::string& achievement_id)>& grant_reward = nullptr) const;

    void HandleEvent(
        std::unordered_map<std::string, bool>& achievements,
        const Event& event,
        const std::function<void(const std::string&)>& notify,
        const std::function<void(const std::string& achievement_id)>& grant_reward = nullptr) const;
};

} // namespace CloudSeamanor::engine

