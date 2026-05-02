#pragma once

// ============================================================================
// 【AchievementRewardLoader.hpp】成就奖励配置加载器
// ============================================================================
// 从 CSV 配置表加载成就奖励数据，提供奖励发放功能。
//
// 性能优化背景：
// - 原 GameRuntime.cpp 中的 grant_achievement_reward lambda 硬编码所有奖励
// - 新方案：配置驱动，只需修改 CSV 文件即可添加/修改奖励
//
// 使用示例：
//   AchievementRewardLoader loader;
//   loader.LoadFromCsv("assets/data/achievement_rewards.csv");
//   
//   // 发放奖励
//   auto grant_fn = loader.MakeGrantFunction(world_state, callbacks);
//   grant_fn("first_crop");  // 发放 first_crop 成就的奖励
// ============================================================================

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/infrastructure/DataRegistry.hpp"

#include <string>
#include <unordered_map>
#include <functional>

namespace CloudSeamanor::engine {

// ============================================================================
// 【AchievementRewardEntry】成就奖励条目
// ============================================================================
struct AchievementRewardEntry {
    std::string achievement_id;
    enum class RewardType { GOLD, ITEM } type;
    std::string item_id;      // ITEM 类型时使用
    int amount = 0;
    std::string hint_text;
    float hint_duration = 1.8f;
};

// ============================================================================
// 【AchievementRewardLoader】成就奖励配置加载器
// ============================================================================
class AchievementRewardLoader {
public:
    // ========================================================================
    // 【LoadFromCsv】从 CSV 文件加载配置
    // ========================================================================
    bool LoadFromCsv(const std::string& path);

    // ========================================================================
    // 【IsLoaded】检查是否已加载
    // ========================================================================
    [[nodiscard]] bool IsLoaded() const noexcept { return !rewards_.empty(); }

    // ========================================================================
    // 【GetReward】获取奖励条目
    // ========================================================================
    [[nodiscard]] const AchievementRewardEntry* GetReward(const std::string& achievement_id) const;

    // ========================================================================
    // 【MakeGrantFunction】创建奖励发放函数
    // ========================================================================
    // 创建一个 std::function<void(const std::string& achievement_id)>，
    // 用于替代原来硬编码的 lambda。
    //
    // @param world_state 世界状态引用（用于修改金币/背包）
    // @param hint_callback 提示回调（用于显示奖励提示）
    // @return 奖励发放函数
    // ========================================================================
    [[nodiscard]] std::function<void(const std::string&)> MakeGrantFunction(
        GameWorldState& world_state,
        const std::function<void(const std::string&, float)>& hint_callback
    ) const;

    // ========================================================================
    // 【GrantReward】直接发放奖励
    // ========================================================================
    void GrantReward(
        const std::string& achievement_id,
        GameWorldState& world_state,
        const std::function<void(const std::string&, float)>& hint_callback
    ) const;

private:
    std::unordered_map<std::string, AchievementRewardEntry> rewards_;
};

}  // namespace CloudSeamanor::engine
