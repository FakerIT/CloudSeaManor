#include "CloudSeamanor/engine/systems/AchievementSystem.hpp"

namespace CloudSeamanor::engine {

bool AchievementSystem::UnlockOnce(
    std::unordered_map<std::string, bool>& achievements,
    const std::string& id,
    const std::string& title,
    const std::function<void(const std::string&)>& notify,
    const std::function<void(const std::string& achievement_id)>& grant_reward) const {
    auto it = achievements.find(id);
    if (it != achievements.end() && it->second) {
        return false;
    }
    achievements[id] = true;
    if (grant_reward) {
        grant_reward(id);
    }
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
    const std::function<void(const std::string&)>& notify,
    const std::function<void(const std::string& achievement_id)>& grant_reward) const {
    if (decoration_score >= 20) {
        UnlockOnce(achievements, "home_designer", "家园设计师", notify, grant_reward);
    }
    if (gold >= 3000) {
        UnlockOnce(achievements, "small_tycoon", "小富豪", notify, grant_reward);
    }
    if (spirit_beast.favor >= 100) {
        UnlockOnce(achievements, "beast_bond_max", "灵兽羁绊满级", notify, grant_reward);
    }
}

void AchievementSystem::HandleEvent(
    std::unordered_map<std::string, bool>& achievements,
    const Event& event,
    const std::function<void(const std::string&)>& notify,
    const std::function<void(const std::string& achievement_id)>& grant_reward) const {
    if (event.type == "harvest") {
        const int count = event.GetAs<int>("count", 0);
        if (count >= 1) {
            UnlockOnce(achievements, "first_crop", "初次收获", notify, grant_reward);
        }
        if (count >= 100) {
            UnlockOnce(achievements, "ten_crops", "小有规模", notify, grant_reward);
        }
    } else if (event.type == "gift") {
        const int favor = event.GetAs<int>("favor_delta", 0);
        if (favor >= 15) {
            UnlockOnce(achievements, "gift_expert", "送礼达人", notify, grant_reward);
        }
    } else if (event.type == "build") {
        const int level = event.GetAs<int>("house_level", 1);
        if (level >= 3) {
            UnlockOnce(achievements, "master_builder", "大宅建成", notify, grant_reward);
        }
    } else if (event.type == "beast_bond") {
        const int favor = event.GetAs<int>("favor", 0);
        if (favor >= 1) {
            UnlockOnce(achievements, "beast_bond", "初次结缘", notify, grant_reward);
        }
        if (favor >= 100) {
            UnlockOnce(achievements, "beast_bond_max", "灵兽羁绊满级", notify, grant_reward);
        }
    } else if (event.type == "beast_type_collected") {
        const bool lively = event.GetAs<int>("lively", 0) > 0;
        const bool lazy = event.GetAs<int>("lazy", 0) > 0;
        const bool curious = event.GetAs<int>("curious", 0) > 0;
        if (lively && lazy && curious) {
            UnlockOnce(achievements, "beast_all_types", "全类型灵兽结缘", notify, grant_reward);
        }
    }
}

} // namespace CloudSeamanor::engine

