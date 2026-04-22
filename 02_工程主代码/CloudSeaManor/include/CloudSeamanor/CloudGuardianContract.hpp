#pragma once

// ============================================================================
// 【CloudGuardianContract】云海守护者契约系统
// ============================================================================
// 统一管理6卷契约书的进度、奖励和解锁状态。
//
// 主要职责：
// - 管理6卷契约书（茶园苏醒契、山庄修缮契、灵兽共生契等）
// - 追踪每个云海之约的完成状态
// - 管理卷的解锁和完成逻辑
// - 提供今日推荐和进度文本
// - 支持存档保存/恢复
//
// 与其他系统的关系：
// - 依赖：GameAppText（物品显示名称）
// - 被依赖：GameAppHud（进度显示）、GameAppSave（存档/读档）
//
// 设计原则：
// - 无截止日期、无失败惩罚
// - 错过季节可跨年补完
// - 全部收集永不失效
// - 永久奖励增强游戏体验
//
// 六卷契约结构：
// - 卷1：茶园苏醒契（农业与灵茶）
// - 卷2：山庄修缮契（建筑与装修）
// - 卷3：灵兽共生契（灵兽系统）
// - 卷4：云海羁绊契（社交与角色）
// - 卷5：灵界探索契（探索与采集）
// - 卷6：太初平衡契（主线与终局）
// ============================================================================

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【CloudPactItem】单个"云海之约"目标
// ============================================================================
// 表示一个具体的收集目标，如"收集初雾绿茶×10"。
//
// 字段说明：
// - id：唯一标识符
// - name：显示名称（如"春之芽"）
// - description：详细描述
// - required_item：需要的物品ID
// - required_count：需要数量
// - completed：是否已完成
struct CloudPactItem {
    std::string id;           // 唯一标识
    std::string name;         // 显示名称
    std::string description;    // 描述
    std::string required_item; // 需要的物品ID
    int required_count = 0;   // 需要数量
    bool completed = false;   // 是否已完成

    // 检查是否能用指定物品推进
    bool CanProgressWith(const std::string& item_id) const noexcept {
        return !completed && required_item == item_id;
    }
};

// ============================================================================
// 【ContractVolume】单卷契约书
// ============================================================================
// 表示一卷完整的契约书，包含多个云海之约。
//
// 字段说明：
// - volume_id：卷号（1-6）
// - name：卷名（如"茶园苏醒契"）
// - theme：主题（如"农业与灵茶"）
// - description：描述
// - items：云海之约列表
// - unlocked：是否已解锁
// - completed：是否已完成
// - permanent_bonus_description：永久奖励描述
struct ContractVolume {
    int volume_id = 0;                          // 卷号（1-6）
    std::string name;                           // 卷名
    std::string theme;                          // 主题
    std::string description;                     // 描述
    std::vector<CloudPactItem> items;          // 云海之约列表
    bool unlocked = false;                      // 是否已解锁
    bool completed = false;                     // 是否已完成
    std::string permanent_bonus_description;    // 永久奖励描述

    // 获取已完成项数量
    [[nodiscard]] int CompletedCount() const noexcept {
        int count = 0;
        for (const auto& item : items) {
            if (item.completed) ++count;
        }
        return count;
    }

    // 获取总项数量
    [[nodiscard]] int TotalCount() const noexcept {
        return static_cast<int>(items.size());
    }

    // 获取完成进度比例
    [[nodiscard]] float ProgressRatio() const noexcept {
        if (TotalCount() == 0) return 1.0f;
        return static_cast<float>(CompletedCount()) / static_cast<float>(TotalCount());
    }
};

// ============================================================================
// 【CloudGuardianContract】契约系统领域对象
// ============================================================================
// 管理6卷契约书的完整生命周期。
//
// 设计决策：
// - 使用全局单例访问，方便各系统联动
// - 支持追踪指定卷，显示今日推荐
// - 简化版交付逻辑（直接标记完成）
// - 后续可扩展为跟踪累计数量
//
// 使用示例：
// @code
// auto& contract = GetGlobalContract();
// contract.Initialize();  // 初始化
// bool completed = contract.DeliverItem("TeaLeaf", 1);  // 交付物品
// contract.CheckVolumeUnlocks();  // 检查解锁
// @endcode
class CloudGuardianContract {
public:
    // ========================================================================
    // 【Initialize】初始化契约数据
    // ========================================================================
    // @note 首次运行或重置时调用
    // @note 创建默认的6卷契约数据
    void Initialize();

    // ========================================================================
    // 【DeliverItem】交付物品给契约系统
    // ========================================================================
    // @param item_id 物品ID
    // @param count 物品数量（简化版忽略数量，直接标记完成）
    // @return true 如果任何云海之约因此完成
    // @note 简化版：直接标记完成，后续可扩展为跟踪累计数量
    [[nodiscard]] bool DeliverItem(const std::string& item_id, int count);

    // ========================================================================
    // 【CheckVolumeUnlocks】检查卷的解锁状态
    // ========================================================================
    // @note 检查前序卷是否完成，解锁后续卷
    // @note 检查已解锁卷是否全部完成
    // @calledby GameApp::SleepToNextMorning()
    void CheckVolumeUnlocks();

    // ========================================================================
    // 【SetTrackingVolume】设置追踪的卷
    // ========================================================================
    // @param volume_id 卷号
    // @note 只有已解锁的卷才能被追踪
    void SetTrackingVolume(int volume_id);

    // ========================================================================
    // 【访问器】
    // ========================================================================
    [[nodiscard]] const std::vector<ContractVolume>& Volumes() const noexcept { return volumes_; }
    [[nodiscard]] const ContractVolume* GetVolume(int volume_id) const;
    [[nodiscard]] const ContractVolume* GetTrackingVolume() const;

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] bool IsVolumeUnlocked(int volume_id) const;
    [[nodiscard]] int CompletedVolumeCount() const noexcept;
    [[nodiscard]] int CompletedPactCount() const noexcept;
    [[nodiscard]] int TotalPactCount() const noexcept;

    // ========================================================================
    // 【文本生成】
    // ========================================================================
    [[nodiscard]] std::string ProgressText() const;
    [[nodiscard]] std::string VolumeProgressText(int volume_id) const;
    [[nodiscard]] std::string TodayRecommendation() const;

    // ========================================================================
    // 【存档接口】
    // ========================================================================
    // 保存契约进度
    [[nodiscard]] std::string SaveState() const;

    // 恢复契约进度
    void LoadState(const std::string& state);

    // 重置契约进度
    void Reset();

private:
    // ========================================================================
    // 【InitializeVolumes】初始化6卷契约数据
    // ========================================================================
    [[nodiscard]] std::vector<ContractVolume> InitializeVolumes_() const;

    // ========================================================================
    // 【Member Variables】
    // ========================================================================
    std::vector<ContractVolume> volumes_;     // 契约卷列表
    int tracking_volume_id_ = 1;            // 当前追踪的卷号（默认第1卷）
};

// ============================================================================
// 【GetGlobalContract】获取全局契约单例
// ============================================================================
// @return 全局唯一的契约系统实例
// @note 用于引擎层访问契约系统
CloudGuardianContract& GetGlobalContract();

}  // namespace CloudSeamanor::domain
