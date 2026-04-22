#include "CloudSeamanor/ToolSystem.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::domain {

namespace {

// ============================================================================
// 【ToolEffectTable】工具效果查找表
// ============================================================================
// [ToolType][Tier]
// 锄头：效率倍率 = 翻土速度，范围倍率 = 翻土宽度
// 水壶：效率倍率 = 浇水速度，范围倍率 = 一次浇灌的地块数
// 洒水器：洒水器覆盖数
const ToolEffect kToolEffectTable
    [static_cast<std::size_t>(ToolType::Count)]
    [static_cast<std::size_t>(ToolTier::COUNT)] = {
    // ── 锄头 ─────────────────────────────────────────────────────────────
    {
        // Tier1 普通
        {"hoe_normal", "普通锄头", "普通的锄头", 1.0f, 1.0f, 0.0f, 0},
        // Tier2 铜
        {"hoe_copper", "铜锄", "铜制锄头，效率更高", 1.2f, 1.0f, 0.0f, 0},
        // Tier3 银
        {"hoe_silver", "银锄", "银制锄头，翻土更快", 1.5f, 1.0f, 0.1f, 0},
        // Tier4 金
        {"hoe_gold", "金锄", "金制锄头，翻土又快又省力", 1.8f, 1.0f, 0.2f, 0},
        // Tier5 灵金
        {"hoe_spirit", "灵金锄", "灵金锄头！翻土范围最大、最省力", 2.2f, 1.3f, 0.35f, 0},
    },
    // ── 水壶 ─────────────────────────────────────────────────────────────
    {
        // Tier1 普通
        {"watering_can_normal", "普通水壶", "普通的水壶，一次浇一块", 1.0f, 1.0f, 0.0f, 1},
        // Tier2 铜
        {"watering_can_copper", "铜水壶", "铜制水壶，容量稍大", 1.2f, 1.0f, 0.0f, 1},
        // Tier3 银
        {"watering_can_silver", "银水壶", "银制水壶，一次浇两块", 1.4f, 2.0f, 0.1f, 2},
        // Tier4 金
        {"watering_can_gold", "金水壶", "金制水壶，一次浇三块", 1.6f, 3.0f, 0.2f, 3},
        // Tier5 灵金
        {"watering_can_spirit", "灵金水壶", "灵金水壶！一次浇四块，极省体力", 2.0f, 4.0f, 0.35f, 4},
    },
    // ── 洒水器 ────────────────────────────────────────────────────────────
    {
        // Tier1 普通
        {"sprinkler_normal", "普通洒水器", "每日自动浇一块地", 1.0f, 1.0f, 0.0f, 1},
        // Tier2 铜
        {"sprinkler_copper", "铜洒水器", "每日自动浇两块地", 1.2f, 1.0f, 0.0f, 2},
        // Tier3 银
        {"sprinkler_silver", "银洒水器", "每日自动浇三块地", 1.4f, 1.0f, 0.1f, 3},
        // Tier4 金
        {"sprinkler_gold", "金洒水器", "每日自动浇四块地", 1.6f, 1.0f, 0.2f, 4},
        // Tier5 灵金
        {"sprinkler_spirit", "灵金洒水器", "灵金洒水器！每日自动浇六块地", 2.0f, 1.0f, 0.35f, 6},
    },
};

const char* kToolTypeNames[static_cast<std::size_t>(ToolType::Count)] = {
    "锄头",
    "水壶",
    "洒水器"
};

const char* kTierNames[static_cast<std::size_t>(ToolTier::COUNT)] = {
    "普通",
    "铜",
    "银",
    "金",
    "灵金"
};

}  // namespace

// ============================================================================
// 【GetToolEffectTable】工具效果查找
// ============================================================================
const ToolEffect& GetToolEffectTable(ToolType type, int level) {
    const int tier = std::clamp(level - 1, 0, static_cast<int>(ToolTier::COUNT) - 1);
    return kToolEffectTable[static_cast<std::size_t>(type)][static_cast<std::size_t>(tier)];
}

// ============================================================================
// 【ToolSystem::Initialize】初始化
// ============================================================================
void ToolSystem::Initialize(const ToolUpgradeConfig& config) {
    config_ = config;

    for (std::size_t i = 0; i < static_cast<std::size_t>(ToolType::Count); ++i) {
        const auto type = static_cast<ToolType>(i);
        tool_levels_[type] = ToolLevel{};

        // 填充效果表引用
        tool_effects_[type] = GetToolEffectTable(type, 1);
    }
}

// ============================================================================
// 【ToolSystem::GetLevel】获取等级
// ============================================================================
int ToolSystem::GetLevel(ToolType type) const {
    const auto it = tool_levels_.find(type);
    return (it != tool_levels_.end()) ? it->second.level : 1;
}

// ============================================================================
// 【ToolSystem::GetTier】获取品质阶段
// ============================================================================
ToolTier ToolSystem::GetTier(ToolType type) const {
    const auto it = tool_levels_.find(type);
    return (it != tool_levels_.end()) ? it->second.tier : ToolTier::Tier1;
}

// ============================================================================
// 【ToolSystem::GetExp】获取经验
// ============================================================================
float ToolSystem::GetExp(ToolType type) const {
    const auto it = tool_levels_.find(type);
    return (it != tool_levels_.end()) ? it->second.exp : 0.0f;
}

// ============================================================================
// 【ToolSystem::GetEffect】获取效果
// ============================================================================
ToolEffect ToolSystem::GetEffect(ToolType type) const {
    const int level = GetLevel(type);
    return GetToolEffectTable(type, level);
}

// ============================================================================
// 【ToolSystem::AddExp】增加经验
// ============================================================================
bool ToolSystem::AddExp(ToolType type, float amount) {
    auto& tl = tool_levels_[type];
    tl.exp += amount;

    if (CheckLevelUp_(type)) {
        tl.level = std::min(tl.level + 1, 5);
        tl.exp = 0.0f;
        RecalculateExpThreshold_(type);
        tool_effects_[type] = GetToolEffectTable(type, tl.level);
        return true;  // 升级了
    }
    return false;
}

// ============================================================================
// 【ToolSystem::SetLevel】强制设置等级
// ============================================================================
void ToolSystem::SetLevel(ToolType type, int level) {
    auto& tl = tool_levels_[type];
    tl.level = std::clamp(level, 1, 5);
    tl.tier = static_cast<ToolTier>(std::clamp(level - 1, 0,
                                            static_cast<int>(ToolTier::COUNT) - 1));
    tl.exp = 0.0f;
    RecalculateExpThreshold_(type);
    tool_effects_[type] = GetToolEffectTable(type, tl.level);
}

// ============================================================================
// 【ToolSystem::CheckLevelUp_】检查升级
// ============================================================================
bool ToolSystem::CheckLevelUp_(ToolType type) {
    auto& tl = tool_levels_[type];
    if (tl.level >= 5) return false;
    return tl.exp >= tl.exp_for_next;
}

// ============================================================================
// 【ToolSystem::RecalculateExpThreshold_】重新计算阈值
// ============================================================================
void ToolSystem::RecalculateExpThreshold_(ToolType type) {
    auto& tl = tool_levels_[type];
    switch (tl.level) {
    case 1: tl.exp_for_next = config_.exp_threshold_tier2; break;
    case 2: tl.exp_for_next = config_.exp_threshold_tier3; break;
    case 3: tl.exp_for_next = config_.exp_threshold_tier4; break;
    case 4: tl.exp_for_next = config_.exp_threshold_tier5; break;
    default: tl.exp_for_next = 99999.0f; break;
    }
}

// ============================================================================
// 【GetEfficiency / GetRange / GetCostReduction / GetSprinklerCoverage】
// ============================================================================
float ToolSystem::GetEfficiency(ToolType type) const {
    return GetEffect(type).efficiency_multiplier;
}

float ToolSystem::GetRange(ToolType type) const {
    return GetEffect(type).range_multiplier;
}

float ToolSystem::GetCostReduction(ToolType type) const {
    return GetEffect(type).cost_reduction;
}

int ToolSystem::GetSprinklerCoverage(ToolType type) const {
    if (type != ToolType::Sprinkler) return 0;
    return GetEffect(type).sprinkler_coverage;
}

float ToolSystem::CalcActualStaminaCost(ToolType type, float base_cost) const {
    const float reduction = GetCostReduction(type);
    return std::max(1.0f, base_cost * (1.0f - reduction));
}

// ============================================================================
// 【ToolSystem::ToolTypeName / ToolTierName / FullToolName】
// ============================================================================
const char* ToolSystem::ToolTypeName(ToolType type) const {
    return kToolTypeNames[static_cast<std::size_t>(type)];
}

const char* ToolSystem::ToolTierName(ToolTier tier) const {
    return kTierNames[static_cast<std::size_t>(tier)];
}

std::string ToolSystem::FullToolName(ToolType type) const {
    const auto& effect = GetEffect(type);
    return std::string(ToolTierName(GetTier(type))) + effect.display_name;
}

}  // namespace CloudSeamanor::domain
