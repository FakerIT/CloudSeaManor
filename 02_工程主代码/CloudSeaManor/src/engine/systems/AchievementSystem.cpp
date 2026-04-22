#include "CloudSeamanor/engine/systems/AchievementSystem.hpp"

namespace CloudSeamanor::engine {

bool AchievementSystem::UnlockOnce(
    std::unordered_map<std::string, bool>& achievements,
    const std::string& id,
    const std::string& title,
    const std::function<void(const std::string&)>& notify) const {
    auto it = achievements.find(id);
    if (it != achievements.end() && it->second) {
        return false;
    }
    achievements[id] = true;
    if (notify) {
        notify("成就解锁：" + title);
    }
    return true;
}

void AchievementSystem::EvaluateDaily(
    std::unordered_map<std::string, bool>& achievements,
    int decoration_score,
    int gold,
    const SpiritBeast& spirit_beast,
    const std::function<void(const std::string&)>& notify) const {
    if (decoration_score >= 20) {
        UnlockOnce(achievements, "home_designer", "家园设计师", notify);
    }
    if (gold >= 3000) {
        UnlockOnce(achievements, "small_tycoon", "小富豪", notify);
    }
    if (spirit_beast.favor >= 100) {
        UnlockOnce(achievements, "beast_bond_max", "灵兽羁绊满级", notify);
    }
}

} // namespace CloudSeamanor::engine

