#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/GameConstants.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::domain {

namespace {
namespace SK = CloudSeamanor::GameConstants::Skill;
}

// ============================================================================
// 【Initialize】初始化技能系统
// ============================================================================
void SkillSystem::Initialize() {
    skills_.clear();
    skills_[SkillType::SpiritFarm] = SkillState();
    skills_[SkillType::SpiritForage] = SkillState();
    skills_[SkillType::SpiritFish] = SkillState();
    skills_[SkillType::SpiritMine] = SkillState();
    skills_[SkillType::SpiritGuard] = SkillState();
    leveled_up_ = false;
}

// ============================================================================
// 【AddExp】添加经验
// ============================================================================
bool SkillSystem::AddExp(
    SkillType skill,
    float base_exp,
    float cloud_density,
    float tea_buff,
    float beast_share
) {
    // 计算云海倍率：+50% ~ +120%
    const float cloud_mul = 1.0f + std::clamp(cloud_density, 0.0f, 1.0f) * SK::CloudMultiplierCap;

    // 计算最终经验
    const float final_exp = base_exp * cloud_mul * tea_buff * beast_share;

    // 获取或创建技能状态
    SkillState& state = skills_[skill];

    // 添加经验
    state.exp += final_exp;

    // 检查升级
    bool leveled = false;
    while (state.exp >= state.exp_to_next && state.level < SK::MaxLevel * 2) {
        state.exp -= state.exp_to_next;
        state.level += 1;
        state.exp_to_next *= SK::ExpGrowthRate;
        ApplyLevelUp_(skill);
        leveled = true;
    }

    return leveled;
}

// ============================================================================
// 【ApplyLevelUp_】应用升级
// ============================================================================
void SkillSystem::ApplyLevelUp_(SkillType skill) {
    leveled_up_ = true;
    last_leveled_skill_ = skill;
}

// ============================================================================
// 【GetBonus】获取技能效果加成
// ============================================================================
float SkillSystem::GetBonus(SkillType skill) const {
    const auto it = skills_.find(skill);
    if (it == skills_.end()) return 0.0f;

    const int level = it->second.level;

    // 基础加成：每级+5%
    float bonus = static_cast<float>(level - 1) * SK::PerLevelBonus;

    if (level >= SK::MaxLevel) {
        bonus += SK::MaxLevelExtraBonus;
    }

    return bonus;
}

// ============================================================================
// 【GetSkill】获取技能状态
// ============================================================================
const SkillState& SkillSystem::GetSkill(SkillType skill) const {
    static SkillState empty;
    const auto it = skills_.find(skill);
    return (it != skills_.end()) ? it->second : empty;
}

SkillState& SkillSystem::GetSkill(SkillType skill) {
    return skills_[skill];
}

// ============================================================================
// 【GetLevel】获取等级
// ============================================================================
int SkillSystem::GetLevel(SkillType skill) const noexcept {
    const auto it = skills_.find(skill);
    return (it != skills_.end()) ? it->second.level : 1;
}

// ============================================================================
// 【GetExp】获取当前经验
// ============================================================================
float SkillSystem::GetExp(SkillType skill) const noexcept {
    const auto it = skills_.find(skill);
    return (it != skills_.end()) ? it->second.exp : 0.0f;
}

// ============================================================================
// 【GetExpToNext】获取升级所需经验
// ============================================================================
float SkillSystem::GetExpToNext(SkillType skill) const noexcept {
    const auto it = skills_.find(skill);
    return (it != skills_.end()) ? it->second.exp_to_next : SK::InitialExpToNext;
}

// ============================================================================
// 【GetExpRatio】获取经验进度比例
// ============================================================================
float SkillSystem::GetExpRatio(SkillType skill) const noexcept {
    const auto it = skills_.find(skill);
    if (it == skills_.end()) return 0.0f;
    if (it->second.exp_to_next <= 0.0f) return 0.0f;
    return std::min(1.0f, it->second.exp / it->second.exp_to_next);
}

// ============================================================================
// 【GetMaxLevel】获取技能最大等级
// ============================================================================
int SkillSystem::GetMaxLevel(SkillType skill) const noexcept {
    (void)skill;
    return SK::MaxLevel;
}

// ============================================================================
// 【GetExpThreshold】获取指定等级的累计经验阈值
// ============================================================================
float SkillSystem::GetExpThreshold(SkillType skill, int target_level) const {
    if (target_level <= 1) return 0.0f;
    float total = 0.0f;
    float exp_to_next = SK::InitialExpToNext;
    for (int i = 1; i < target_level; ++i) {
        total += exp_to_next;
        exp_to_next *= SK::ExpGrowthRate;
    }
    (void)skill;  // 未使用的参数
    return total;
}

// ============================================================================
// 【GetSkillName】获取技能名称
// ============================================================================
std::string SkillSystem::GetSkillName(SkillType skill) const {
    return SkillTypeToString(skill);
}

// ============================================================================
// 【GetLevelText】获取等级文本
// ============================================================================
std::string SkillSystem::GetLevelText(SkillType skill) const {
    std::ostringstream oss;
    oss << SkillTypeToString(skill) << " Lv." << GetLevel(skill);
    return oss.str();
}

// ============================================================================
// 【GetBonusText】获取加成文本
// ============================================================================
std::string SkillSystem::GetBonusText(SkillType skill) const {
    std::ostringstream oss;
    oss << GetSkillName(skill) << "加成：+" << static_cast<int>(GetBonus(skill) * 100) << "%";
    return oss.str();
}

// ============================================================================
// 【SaveState】保存状态
// ============================================================================
std::string SkillSystem::SaveState() const {
    std::ostringstream oss;
    for (const auto& [skill, state] : skills_) {
        oss << SkillTypeToString(skill) << ":"
             << state.level << ","
             << state.exp << ","
             << state.exp_to_next << "|";
    }
    return oss.str();
}

// ============================================================================
// 【LoadState】加载状态
// ============================================================================
void SkillSystem::LoadState(const std::string& state) {
    if (state.empty()) {
        Initialize();
        return;
    }
    skills_.clear();
    std::istringstream iss(state);
    std::string segment;
    while (std::getline(iss, segment, '|')) {
        if (segment.empty()) continue;
        std::istringstream ss(segment);
        std::string skill_name;
        if (!std::getline(ss, skill_name, ':')) continue;
        SkillType skill = StringToSkillType(skill_name);
        SkillState s;
        std::string rest;
        if (std::getline(ss, rest)) {
            std::replace(rest.begin(), rest.end(), ',', ' ');
            std::istringstream rs(rest);
            rs >> s.level >> s.exp >> s.exp_to_next;
        }
        skills_[skill] = s;
    }
    leveled_up_ = false;
}

// ============================================================================
// 【SkillTypeToString】技能类型转字符串
// ============================================================================
std::string SkillTypeToString(SkillType skill) {
    switch (skill) {
    case SkillType::SpiritFarm:   return "灵农";
    case SkillType::SpiritForage: return "灵觅";
    case SkillType::SpiritFish:   return "灵钓";
    case SkillType::SpiritMine:   return "灵矿";
    case SkillType::SpiritGuard:  return "灵卫";
    }
    return "未知";
}

// ============================================================================
// 【StringToSkillType】字符串转技能类型
// ============================================================================
SkillType StringToSkillType(const std::string& name) {
    if (name == "灵农") return SkillType::SpiritFarm;
    if (name == "灵觅") return SkillType::SpiritForage;
    if (name == "灵钓") return SkillType::SpiritFish;
    if (name == "灵矿") return SkillType::SpiritMine;
    if (name == "灵卫") return SkillType::SpiritGuard;
    return SkillType::SpiritFarm;
}

}  // namespace CloudSeamanor::domain
