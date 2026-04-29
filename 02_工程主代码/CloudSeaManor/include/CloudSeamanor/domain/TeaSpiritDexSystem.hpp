#pragma once

// ============================================================================
// 【TeaSpiritDexSystem】茶灵图鉴系统
// ============================================================================
// 统一管理茶灵图鉴的解锁、状态和奖励。
//
// 主要职责：
// - 管理茶灵图鉴条目状态
// - 判定茶灵解锁条件
// - 管理里程碑奖励
// - 提供图鉴查询接口
//
// 设计原则：
// - 依附核心玩法：收集自然发生在制茶和饮茶流程中
// - 先显性、后隐藏：基础茶灵容易理解，稀有茶灵保留探索空间
// - 收藏可见：解锁后必须在庄园中有实体感
// - 治愈优先：收集失败不惩罚
// ============================================================================

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace CloudSeamanor::domain {

// ============================================================================
// 【TeaSpiritRarity】茶灵稀有度
// ============================================================================
enum class TeaSpiritRarity {
    Common = 0,    // 常规茶灵
    Seasonal = 1,  // 季候茶灵
    Hidden = 2,    // 隐藏茶灵
    Auspicious = 3  // 祥瑞茶灵
};

// ============================================================================
// 【TeaSpiritEntry】茶灵条目
// ============================================================================
struct TeaSpiritEntry {
    std::string tea_spirit_id;
    std::string tea_id;
    TeaSpiritRarity rarity = TeaSpiritRarity::Common;
    std::string name;          // 茶灵名称
    std::string description;   // 图鉴描述
    bool unlocked = false;
    int first_unlock_day = 0;  // 首次解锁天数
    bool display_enabled = true;  // 是否在茶室展示
};

// ============================================================================
// 【TeaSpiritUnlockRule】茶灵解锁规则
// ============================================================================
struct TeaSpiritUnlockRule {
    std::string tea_id;             // 所需灵茶 ID
    std::string tea_spirit_id;      // 对应茶灵 ID
    std::string required_quality;   // 所需品质 (Normal/Fine/Rare/Holy)
    std::string season;             // 季节限制
    std::string time_range;         // 时辰限制
    std::string weather_limit;      // 天气限制
    std::string location_id;        // 场景限制
    std::string extra_condition;     // 额外条件 (eco_balanced/first_brew/festival_day)
};

// ============================================================================
// 【TeaSpiritReward】茶灵奖励配置
// ============================================================================
struct TeaSpiritReward {
    int milestone_count;         // 里程碑数量
    std::string reward_id;      // 奖励 ID
    std::string reward_type;    // 奖励类型 (item/stamina/buff/title)
    std::string reward_value;   // 奖励值
    bool claimed = false;
};

// ============================================================================
// 【TeaSpiritDexSystem】茶灵图鉴系统
// ============================================================================
class TeaSpiritDexSystem {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    TeaSpiritDexSystem();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize();

    // ========================================================================
    // 【数据加载】
    // ========================================================================
    void LoadSpirits(const std::vector<TeaSpiritEntry>& spirits);
    void LoadUnlockRules(const std::vector<TeaSpiritUnlockRule>& rules);
    void LoadRewards(const std::vector<TeaSpiritReward>& rewards);

    // ========================================================================
    // 【解锁判定】
    // ========================================================================
    // 尝试解锁茶灵，返回是否成功
    bool TryUnlock(
        const std::string& tea_id,
        const std::string& quality,
        const std::string& season,
        int time_hour,
        const std::string& weather,
        const std::string& location_id,
        const std::string& extra_condition,
        int current_day
    );

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] int GetUnlockedCount() const;
    [[nodiscard]] int GetTotalCount() const;
    [[nodiscard]] float GetCompletionRatio() const;
    [[nodiscard]] const TeaSpiritEntry* GetSpirit(const std::string& spirit_id) const;
    [[nodiscard]] std::vector<const TeaSpiritEntry*> GetUnlockedSpirits() const;
    [[nodiscard]] std::vector<const TeaSpiritEntry*> GetAllSpirits() const;
    [[nodiscard]] bool IsSpiritUnlocked(const std::string& spirit_id) const;

    // ========================================================================
    // 【奖励】
    // ========================================================================
    [[nodiscard]] std::vector<const TeaSpiritReward*> GetPendingRewards() const;
    void ClaimReward(const std::string& reward_id);
    [[nodiscard]] bool IsMasterRewardClaimed() const { return master_reward_claimed_; }
    void ClaimMasterReward() { master_reward_claimed_ = true; }

    // ========================================================================
    // 【展示控制】
    // ========================================================================
    void SetSpiritDisplay(const std::string& spirit_id, bool enabled);
    [[nodiscard]] std::vector<const TeaSpiritEntry*> GetDisplayableSpirits() const;

    // ========================================================================
    // 【存档序列化】
    // ========================================================================
    [[nodiscard]] std::string Serialize() const;
    void Deserialize(const std::string& data);

private:
    [[nodiscard]] const TeaSpiritUnlockRule* FindMatchingRule_(const std::string& tea_id) const;
    bool CheckRuleConditions_(
        const TeaSpiritUnlockRule& rule,
        const std::string& quality,
        const std::string& season,
        int time_hour,
        const std::string& weather,
        const std::string& location_id,
        const std::string& extra_condition
    ) const;

    std::vector<TeaSpiritEntry> spirits_;
    std::vector<TeaSpiritUnlockRule> unlock_rules_;
    std::vector<TeaSpiritReward> rewards_;
    bool master_reward_claimed_ = false;

    std::unordered_map<std::string, std::size_t> spirit_index_;  // spirit_id -> index
};

// ============================================================================
// 【辅助函数】
// ============================================================================
[[nodiscard]] inline const char* ToString(TeaSpiritRarity r) {
    switch (r) {
        case TeaSpiritRarity::Common: return "Common";
        case TeaSpiritRarity::Seasonal: return "Seasonal";
        case TeaSpiritRarity::Hidden: return "Hidden";
        case TeaSpiritRarity::Auspicious: return "Auspicious";
    }
    return "Unknown";
}

[[nodiscard]] inline const char* ToDisplayName(TeaSpiritRarity r) {
    switch (r) {
        case TeaSpiritRarity::Common: return "常规";
        case TeaSpiritRarity::Seasonal: return "季候";
        case TeaSpiritRarity::Hidden: return "隐藏";
        case TeaSpiritRarity::Auspicious: return "祥瑞";
    }
    return "?";
}

[[nodiscard]] inline TeaSpiritRarity ParseRarity(const std::string& s) {
    if (s == "Seasonal") return TeaSpiritRarity::Seasonal;
    if (s == "Hidden") return TeaSpiritRarity::Hidden;
    if (s == "Auspicious") return TeaSpiritRarity::Auspicious;
    return TeaSpiritRarity::Common;
}

}  // namespace CloudSeamanor::domain
