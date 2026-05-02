#pragma once

// ============================================================================
// 【BuffEffectTable】灵茶Buff效果数据驱动层
// ============================================================================
// 负责从 CSV 加载灵茶Buff效果定义表，提供Buff效果信息查询。
//
// 主要职责：
// - 从 `assets/data/BuffEffectTable.csv` 加载Buff效果定义
// - 按Buff ID查询效果定义
// - 提供Buff效果对游戏参数的影响计算
//
// 设计原则：
// - 所有Buff效果参数由数据驱动
// - 支持多种效果组合（体力、交易、好感、灵气等）
// - 效果可视化（颜色、图标等）
// ============================================================================

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【BuffEffectDefinition】Buff效果定义（单行 CSV 数据）
// ============================================================================
struct BuffEffectDefinition {
    std::string id;                          // Buff效果唯一标识
    std::string name;                          // 中文名称
    int duration_seconds = 0;                 // 持续时间（秒）
    std::string description;                   // 效果描述
    float stamina_recovery_multiplier = 1.0f; // 体力恢复倍率
    float stamina_cost_multiplier = 1.0f;     // 体力消耗倍率
    float work_efficiency_bonus = 0.0f;      // 工作效率加成
    float trade_price_bonus = 0.0f;           // 交易价格加成
    float favor_gain_bonus = 0.0f;            // 好感度获取加成
    float spirit_rate_bonus = 0.0f;           // 灵气获取加成
    float craft_success_bonus = 0.0f;         // 制作成功率加成
    float insight_trigger_bonus = 0.0f;       // 灵感触发加成
    float skill_mastery_bonus = 0.0f;         // 技能熟练度加成
    float defense_bonus = 0.0f;               // 防御力加成
    float move_speed_bonus = 0.0f;            // 移动速度加成
    std::string color_hex;                     // 显示颜色（十六进制）
};

// ============================================================================
// 【ActiveBuff】运行时激活的Buff
// ============================================================================
struct ActiveBuff {
    std::string buff_effect_id;       // Buff效果ID
    float remaining_seconds = 0.0f;  // 剩余时间
    float stamina_recovery_mult = 1.0f;
    float stamina_cost_mult = 1.0f;
    float work_efficiency = 0.0f;
    float trade_price = 0.0f;
    float favor_gain = 0.0f;
    float spirit_rate = 0.0f;
    float craft_success = 0.0f;
    float insight_trigger = 0.0f;
    float skill_mastery = 0.0f;
    float defense = 0.0f;
    float move_speed = 0.0f;
};

// ============================================================================
// 【BuffEffectTable】Buff效果数据表管理器
// ============================================================================
class BuffEffectTable {
public:
    // ========================================================================
    // 【LoadFromFile】从 CSV 文件加载Buff效果表
    // ========================================================================
    // @param file_path CSV 文件路径
    // @return true 表示加载成功
    bool LoadFromFile(const std::string& file_path);

    // ========================================================================
    // 【Get】按 ID 获取Buff效果定义
    // ========================================================================
    // @param id Buff效果标识
    // @return Buff效果定义指针，未找到返回 nullptr
    [[nodiscard]] const BuffEffectDefinition* Get(const std::string& id) const;

    // ========================================================================
    // 【Exists】检查Buff效果是否存在
    // ========================================================================
    [[nodiscard]] bool Exists(const std::string& id) const;

    // ========================================================================
    // 【All】获取所有Buff效果定义
    // ========================================================================
    [[nodiscard]] const std::vector<BuffEffectDefinition>& All() const noexcept { return buffs_; }

    // ========================================================================
    // 【GetByCategory】按效果类型获取Buff列表
    // ========================================================================
    // @param category 效果类型（stamina/trade/favor/spirit/craft/mastery）
    [[nodiscard]] std::vector<const BuffEffectDefinition*> GetByCategory(const std::string& category) const;

private:
    std::vector<BuffEffectDefinition> buffs_;
    std::unordered_map<std::string, std::size_t> index_;
};

// ============================================================================
// 【GetGlobalBuffEffectTable】获取全局Buff效果表单例
// ============================================================================
[[nodiscard]] BuffEffectTable& GetGlobalBuffEffectTable();

} // namespace CloudSeamanor::domain
