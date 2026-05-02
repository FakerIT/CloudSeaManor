#include "CloudSeamanor/engine/systems/AchievementRewardLoader.hpp"
#include "CloudSeamanor/infrastructure/DataRegistry.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【AchievementRewardLoader::LoadFromCsv】从 CSV 文件加载配置
// ============================================================================
bool AchievementRewardLoader::LoadFromCsv(const std::string& path) {
    rewards_.clear();

    infrastructure::DataRegistry registry;
    if (!registry.LoadCsv(path)) {
        infrastructure::Logger::LogConfigLoadFailure("AchievementRewardLoader: " + path);
        return false;
    }

    for (const auto& row : registry.GetTable("achievement_rewards")) {
        AchievementRewardEntry entry;
        entry.achievement_id = row.GetString("AchievementId");
        const std::string type_str = row.GetString("RewardType");

        if (type_str == "GOLD") {
            entry.type = AchievementRewardEntry::RewardType::GOLD;
            entry.amount = row.GetInt("Amount");
        } else if (type_str == "ITEM") {
            entry.type = AchievementRewardEntry::RewardType::ITEM;
            entry.item_id = row.GetString("ItemId");
            entry.amount = row.GetInt("Amount");
        } else {
            infrastructure::Logger::Warning("AchievementRewardLoader: Unknown reward type '" + type_str + "' for " + entry.achievement_id);
            continue;
        }

        entry.hint_text = row.GetString("HintText");
        entry.hint_duration = row.GetFloat("HintDuration");
        rewards_[entry.achievement_id] = entry;
    }

    infrastructure::Logger::Info("AchievementRewardLoader: Loaded " + std::to_string(rewards_.size()) + " achievement rewards");
    return true;
}

// ============================================================================
// 【AchievementRewardLoader::GetReward】获取奖励条目
// ============================================================================
const AchievementRewardEntry* AchievementRewardLoader::GetReward(const std::string& achievement_id) const {
    const auto it = rewards_.find(achievement_id);
    if (it != rewards_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// 【AchievementRewardLoader::GrantReward】直接发放奖励
// ============================================================================
void AchievementRewardLoader::GrantReward(
    const std::string& achievement_id,
    GameWorldState& world_state,
    const std::function<void(const std::string&, float)>& hint_callback
) const {
    const auto* entry = GetReward(achievement_id);
    if (!entry) {
        return;  // 未找到对应奖励，静默忽略
    }

    switch (entry->type) {
        case AchievementRewardEntry::RewardType::GOLD:
            world_state.MutableGold() += entry->amount;
            break;
        case AchievementRewardEntry::RewardType::ITEM:
            world_state.MutableInventory().AddItem(entry->item_id, entry->amount);
            break;
    }

    hint_callback(entry->hint_text, entry->hint_duration);
}

// ============================================================================
// 【AchievementRewardLoader::MakeGrantFunction】创建奖励发放函数
// ============================================================================
std::function<void(const std::string&)> AchievementRewardLoader::MakeGrantFunction(
    GameWorldState& world_state,
    const std::function<void(const std::string&, float)>& hint_callback
) const {
    return [this, &world_state, hint_callback](const std::string& achievement_id) {
        GrantReward(achievement_id, world_state, hint_callback);
    };
}

}  // namespace CloudSeamanor::engine
