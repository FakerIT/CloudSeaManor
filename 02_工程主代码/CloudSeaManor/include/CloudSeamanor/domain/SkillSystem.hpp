#pragma once

// ============================================================================
// 【SkillSystem】技能与成长系统
// ============================================================================
// 统一管理玩家的5大技能经验和成长，提供云海倍率、灵兽协助加成。
//
// 主要职责：
// - 管理5大技能（灵农、灵觅、灵钓、灵矿、灵卫）
// - 处理经验获取和等级提升
// - 计算云海倍率加成
// - 提供技能效果加成（被动技能）
//
// 与其他系统的关系：
// - 依赖：CloudSystem（云海倍率）、GameAppText（文本显示）
// - 被依赖：GameAppHud（技能显示）、GameAppSave（存档/读档）
//
// 设计原则（对标SDV）：
// - 1-10级提供配方、效率、收益提升
// - 满级后进入灵气大师节点（类似Mastery）
// - 经验获取受云海浓度、灵茶buff、灵兽协助共同影响
//
// 公式：FinalExp = BaseExp × (1 + α × CloudDensity) × TeaBuff × BeastShare
// ============================================================================

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【SkillType】技能类型枚举
// ============================================================================
// 对标SDV的5大技能，对应云海山庄主题。
enum class SkillType {
    SpiritFarm,    // 灵农：作物、灵茶培育、土壤调律
    SpiritForage,  // 灵觅：采集、灵植识别、云海觅食
    SpiritFish,    // 灵钓：云海垂钓、鱼群相位判断
    SpiritMine,    // 灵矿：挖矿、遗迹采掘、资源精炼
    SpiritGuard,   // 灵卫：可选战斗、防护、灵界生存
};

// ============================================================================
// 【SkillState】技能状态
// ============================================================================
// 存储单个技能的经验和等级信息。
struct SkillState {
    int level = 1;              // 当前等级（1-10）
    float exp = 0.0f;         // 当前经验
    float exp_to_next = 100.0f; // 升级所需经验

    // 重置技能状态
    void Reset() {
        level = 1;
        exp = 0.0f;
        exp_to_next = 100.0f;
    }
};

// ============================================================================
// 【SkillSystem】技能系统领域对象
// ============================================================================
// 管理5大技能的经验获取和等级提升。
//
// 设计决策：
// - 经验公式：FinalExp = BaseExp × 云海倍率 × 灵茶buff × 灵兽协助
// - 1-10级快速反馈，11级+进入灵气大师节点
// - 每级提供被动效果加成
//
// 使用示例：
// @code
// SkillSystem skills;
// skills.AddExp(SkillType::SpiritFarm, 25.0f, cloud_density);
// if (skills.IsLeveledUp()) {
//     skills.ShowLevelUpEffect();
// }
// @endcode
class SkillSystem {
public:
    // ========================================================================
    // 【Initialize】初始化技能系统
    // ========================================================================
    // @note 首次运行时调用，设置所有技能为1级
    void Initialize();

    // ========================================================================
    // 【AddExp】添加经验
    // ========================================================================
    // @param skill 技能类型
    // @param base_exp 基础经验值
    // @param cloud_density 云海密度（0.0-1.0）
    // @param tea_buff 灵茶buff倍率（默认1.0）
    // @param beast_share 灵兽协助倍率（默认1.0）
    // @note 实际获得经验 = BaseExp × 云海倍率 × 灵茶buff × 灵兽协助
    // @return true 如果因此升级
    bool AddExp(
        SkillType skill,
        float base_exp,
        float cloud_density = 0.0f,
        float tea_buff = 1.0f,
        float beast_share = 1.0f
    );

    // ========================================================================
    // 【IsLeveledUp】上次添加经验是否升级了
    // ========================================================================
    // @note 用于触发升级特效
    [[nodiscard]] bool IsLeveledUp() const noexcept { return leveled_up_; }

    // ========================================================================
    // 【GetLastLeveledSkill】获取上次升级的技能
    // ========================================================================
    [[nodiscard]] SkillType GetLastLeveledSkill() const noexcept { return last_leveled_skill_; }

    // ========================================================================
    // 【ClearLevelUpFlag】清除升级标记
    // ========================================================================
    void ClearLevelUpFlag() noexcept { leveled_up_ = false; }

    // ========================================================================
    // 【GetBonus】获取技能效果加成
    // ========================================================================
    // @param skill 技能类型
    // @return 对应的被动效果加成比例
    // @note 根据技能等级返回不同加成
    [[nodiscard]] float GetBonus(SkillType skill) const;

    // ========================================================================
    // 【访问器】
    // ========================================================================
    [[nodiscard]] const SkillState& GetSkill(SkillType skill) const;
    [[nodiscard]] SkillState& GetSkill(SkillType skill);
    [[nodiscard]] int GetLevel(SkillType skill) const noexcept;
    [[nodiscard]] float GetExp(SkillType skill) const noexcept;
    [[nodiscard]] float GetExpToNext(SkillType skill) const noexcept;
    [[nodiscard]] float GetExpRatio(SkillType skill) const noexcept;
    [[nodiscard]] int GetMaxLevel(SkillType skill) const noexcept;
    [[nodiscard]] float GetExpThreshold(SkillType skill, int target_level) const;

    // ========================================================================
    // 【文本接口】
    // ========================================================================
    [[nodiscard]] std::string GetSkillName(SkillType skill) const;
    [[nodiscard]] std::string GetLevelText(SkillType skill) const;
    [[nodiscard]] std::string GetBonusText(SkillType skill) const;

    // ========================================================================
    // 【存档接口】
    // ========================================================================
    [[nodiscard]] std::string SaveState() const;
    void LoadState(const std::string& state);

private:
    // ========================================================================
    // 【ApplyLevelUp】应用升级
    // ========================================================================
    // @param skill 升级的技能
    void ApplyLevelUp_(SkillType skill);

    // ========================================================================
    // 【成员变量】
    // ========================================================================
    std::unordered_map<SkillType, SkillState> skills_;
    bool leveled_up_ = false;
    SkillType last_leveled_skill_ = SkillType::SpiritFarm;
};

// ============================================================================
// 【辅助函数】
// ============================================================================

// 技能类型转字符串
[[nodiscard]] std::string SkillTypeToString(SkillType skill);

// 字符串转技能类型
[[nodiscard]] SkillType StringToSkillType(const std::string& name);

}  // namespace CloudSeamanor::domain
