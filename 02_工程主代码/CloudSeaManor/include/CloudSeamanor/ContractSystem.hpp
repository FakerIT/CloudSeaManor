#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace CloudSeamanor::domain {

// ============================================================================
// 【ContractBundle】单个Bundle条目
// ============================================================================
struct ContractBundle {
    std::string id;
    std::string contract_id;
    std::string item_id;
    int item_count = 1;
    std::string item_name_zh;
    std::string description_zh;
};

// ============================================================================
// 【ContractReward】契约奖励
// ============================================================================
struct ContractReward {
    std::string id;
    std::string contract_id;
    std::string reward_type;    // item|buff|title
    std::string reward_id;
    int reward_count = 1;
    std::string description_zh;
    std::string bonus_effect;
};

// ============================================================================
// 【TeaSpirit】茶灵数据
// ============================================================================
struct TeaSpirit {
    std::string id;
    std::string contract_id;
    std::string name_zh;
    std::string name_en;
    std::string description_zh;
    std::string visual_color;    // 十六进制颜色代码
    std::string unlock_animation;
    std::string habitat_zh;
};

// ============================================================================
// 【ContractData】契约定义
// ============================================================================
struct ContractData {
    std::string id;
    std::string name_zh;
    std::string name_en;
    std::string description_zh;
    std::string required_item;   // 解锁所需的道具
    std::string unlock_condition;
    std::string reward_item_id;
    int reward_count = 1;
    std::string prerequisite_id; // 前置契约

    std::vector<ContractBundle> bundles;
    std::vector<ContractReward> rewards;
    TeaSpirit spirit;
};

// ============================================================================
// 【ContractProgress】契约进度
// ============================================================================
struct ContractProgress {
    std::string contract_id;
    int completion_count = 0;
    int total_required = 0;
    bool is_completed = false;
    bool is_rewarded = false;
    int completed_day = 0;  // 完成时的天数

    float GetProgress() const {
        if (total_required == 0) return 0.0f;
        return static_cast<float>(completion_count) / total_required;
    }
};

// ============================================================================
// 【ContractItemProgress】单个物品进度
// ============================================================================
struct BundleItemProgress {
    std::string bundle_id;
    std::string item_id;
    int required = 0;
    int collected = 0;

    bool IsComplete() const { return collected >= required; }
    float GetProgress() const {
        if (required == 0) return 0.0f;
        return std::min(1.0f, static_cast<float>(collected) / required);
    }
};

// ============================================================================
// 【ContractCompleteInfo】契约完成信息
// ============================================================================
struct ContractCompleteInfo {
    std::string contract_id;
    std::string contract_name;
    std::string spirit_name;
    std::string reward_description;
    std::vector<std::string> bonus_effects;
};

// ============================================================================
// 【ContractSystem】契约系统
// ============================================================================
class ContractSystem {
public:
    // ========================================================================
    // 单例
    // ========================================================================
    static ContractSystem& Instance();

    // ========================================================================
    // 初始化
    // ========================================================================
    bool Initialize();
    void LoadContractData(const std::string& csv_path);
    void LoadBundleData(const std::string& csv_path);
    void LoadRewardData(const std::string& csv_path);
    void LoadSpiritData(const std::string& csv_path);

    // ========================================================================
    // 查询接口
    // ========================================================================
    const ContractData* GetContract(const std::string& contract_id) const;
    std::vector<const ContractData*> GetAllContracts() const;
    std::vector<const ContractData*> GetAvailableContracts() const;
    std::vector<const ContractData*> GetCompletedContracts() const;

    bool IsContractUnlocked(const std::string& contract_id) const;
    bool IsContractCompleted(const std::string& contract_id) const;
    bool IsContractRewarded(const std::string& contract_id) const;

    const ContractProgress* GetProgress(const std::string& contract_id) const;
    std::vector<const BundleItemProgress*> GetBundleProgress(const std::string& contract_id) const;

    // ========================================================================
    // 物品收集（当玩家获得契约所需物品时调用）
    // ========================================================================
    void CollectItem(const std::string& item_id, int count = 1);

    // ========================================================================
    // 契约操作
    // ========================================================================
    bool TryCompleteContract(const std::string& contract_id);
    bool TryCompleteContract(const std::string& contract_id, int current_day);
    bool TryClaimReward(const std::string& contract_id);

    // ========================================================================
    // 统计
    // ========================================================================
    int GetCompletedCount() const;
    int GetTotalContractCount() const;
    float GetOverallProgress() const;

    // ========================================================================
    // 存档
    // ========================================================================
    void LoadProgress(const std::map<std::string, ContractProgress>& progress,
                     const std::map<std::string, BundleItemProgress>& bundle_progress);
    std::map<std::string, ContractProgress> GetProgress() const { return progress_; }
    std::map<std::string, BundleItemProgress> GetBundleProgress() const { return bundle_progress_; }

    // ========================================================================
    // 回调
    // ========================================================================
    using OnContractCompleteCallback = std::function<void(const ContractCompleteInfo&)>;
    using OnRewardClaimedCallback = std::function<void(const std::string& contract_id)>;
    using OnBundleItemCollectedCallback = std::function<void(const std::string& item_id, int count)>;
    using OnRewardGrantCallback = std::function<void(const std::string& reward_type, const std::string& reward_id, int count)>;

    void SetOnContractComplete(OnContractCompleteCallback callback) { on_contract_complete_ = callback; }
    void SetOnRewardClaimed(OnRewardClaimedCallback callback) { on_reward_claimed_ = callback; }
    void SetOnBundleItemCollected(OnBundleItemCollectedCallback callback) { on_bundle_item_collected_ = callback; }
    void SetOnRewardGrant(OnRewardGrantCallback callback) { on_reward_grant_ = callback; }

private:
    ContractSystem() = default;
    ~ContractSystem() = default;
    ContractSystem(const ContractSystem&) = delete;
    ContractSystem& operator=(const ContractSystem&) = delete;

    // 数据存储
    std::map<std::string, ContractData> contracts_;
    std::map<std::string, ContractBundle> bundles_;
    std::map<std::string, ContractReward> rewards_;
    std::map<std::string, TeaSpirit> spirits_;

    // 玩家进度
    std::map<std::string, ContractProgress> progress_;
    std::map<std::string, BundleItemProgress> bundle_progress_;

    // 回调
    OnContractCompleteCallback on_contract_complete_;
    OnRewardClaimedCallback on_reward_claimed_;
    OnBundleItemCollectedCallback on_bundle_item_collected_;
    OnRewardGrantCallback on_reward_grant_;

    // 辅助方法
    void InitializeProgress();
    void UpdateContractProgress(const std::string& contract_id);
    bool CheckPrerequisite(const std::string& contract_id) const;
    void TriggerContractComplete(const std::string& contract_id);
    void GrantReward(const std::string& contract_id);
};

}  // namespace CloudSeamanor::domain
