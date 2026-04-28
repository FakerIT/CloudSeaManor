#include "CloudSeamanor/ToolSystem.hpp"
#include "CloudSeamanor/DataRegistry.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>

namespace CloudSeamanor::domain {

namespace {

// ============================================================================
// 【ToolEffectTable】工具效果查找表
// ============================================================================
// [ToolType][Tier]
// 锄头：效率倍率 = 翻土速度，范围倍率 = 翻土宽度
// 水壶：效率倍率 = 浇水速度，范围倍率 = 一次浇灌的地块数
// 洒水器：洒水器覆盖数
const ToolEffect kFallbackToolEffectTable
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
        {"sprinkler_normal", "普通洒水器", "每日自动浇一块地", 1.0f, 1.0f, 0.0f, 1, 1.0f, 1.0f},
        // Tier2 铜
        {"sprinkler_copper", "铜洒水器", "铜制洒水器，每日自动浇两块地", 1.2f, 1.0f, 0.0f, 2, 1.0f, 1.0f},
        // Tier3 银
        {"sprinkler_silver", "银洒水器", "银制洒水器，每日自动浇三块地", 1.4f, 1.0f, 0.1f, 3, 1.0f, 1.0f},
        // Tier4 金
        {"sprinkler_gold", "金洒水器", "金制洒水器，每日自动浇四块地", 1.6f, 1.0f, 0.2f, 4, 1.0f, 1.0f},
        // Tier5 灵金
        {"sprinkler_spirit", "灵金洒水器", "灵金洒水器！每日自动浇六块地", 2.0f, 1.0f, 0.35f, 6, 1.0f, 1.0f},
    },
    // ── 镰刀（收割工具）────────────────────────────────────────────────────
    {
        // Tier1 普通
        {"sickle_normal", "普通镰刀", "普通的收割工具", 1.0f, 1.0f, 0.0f, 0, 1.0f, 1.0f},
        // Tier2 铜
        {"sickle_copper", "铜镰刀", "铜制镰刀，收割效率更高", 1.0f, 1.0f, 0.0f, 0, 1.15f, 1.0f},
        // Tier3 银
        {"sickle_silver", "银镰刀", "银制镰刀，收割时产量+25%", 1.0f, 1.0f, 0.05f, 0, 1.25f, 1.1f},
        // Tier4 金
        {"sickle_gold", "金镰刀", "金制镰刀，收割时产量+40%", 1.0f, 1.0f, 0.1f, 0, 1.40f, 1.2f},
        // Tier5 灵金
        {"sickle_spirit", "灵金镰刀", "灵金镰刀！收割时产量+60%，范围扩大", 1.0f, 1.0f, 0.2f, 0, 1.60f, 1.3f},
    },
    // ── 剪刀（采集工具）────────────────────────────────────────────────────
    {
        // Tier1 普通
        {"scissors_normal", "普通剪刀", "普通的采集工具", 1.0f, 1.0f, 0.0f, 0, 1.0f, 1.0f},
        // Tier2 铜
        {"scissors_copper", "铜剪刀", "铜制剪刀，采集效率更高", 1.0f, 1.0f, 0.0f, 0, 1.15f, 1.0f},
        // Tier3 银
        {"scissors_silver", "银剪刀", "银制剪刀，采集时产量+20%", 1.0f, 1.0f, 0.05f, 0, 1.20f, 1.1f},
        // Tier4 金
        {"scissors_gold", "金剪刀", "金制剪刀，采集时产量+35%", 1.0f, 1.0f, 0.1f, 0, 1.35f, 1.2f},
        // Tier5 灵金
        {"scissors_spirit", "灵金剪刀", "灵金剪刀！采集时产量+50%，范围扩大", 1.0f, 1.0f, 0.2f, 0, 1.50f, 1.3f},
    },
    // ── 斧头（清除障碍）────────────────────────────────────────────────────
    {
        // Tier1 普通
        {"axe_normal", "普通斧头", "普通的清除障碍工具", 1.0f, 1.0f, 0.0f, 0, 1.0f, 1.0f},
        // Tier2 铜
        {"axe_copper", "铜斧头", "铜制斧头，砍伐更快", 1.2f, 1.0f, 0.0f, 0, 1.0f, 1.0f},
        // Tier3 银
        {"axe_silver", "银斧头", "银制斧头，节省体力", 1.4f, 1.0f, 0.1f, 0, 1.0f, 1.0f},
        // Tier4 金
        {"axe_gold", "金斧头", "金制斧头，极速砍伐", 1.6f, 1.0f, 0.2f, 0, 1.0f, 1.0f},
        // Tier5 灵金
        {"axe_spirit", "灵金斧头", "灵金斧头！一击清除小型障碍", 2.0f, 1.2f, 0.35f, 0, 1.0f, 1.0f},
    },
    // ── 镐子（采矿）────────────────────────────────────────────────────
    {
        // Tier1 普通
        {"pickaxe_normal", "普通镐子", "普通的采矿工具", 1.0f, 1.0f, 0.0f, 0, 1.0f, 1.0f},
        // Tier2 铜
        {"pickaxe_copper", "铜镐子", "铜制镐子，采矿更快", 1.2f, 1.0f, 0.0f, 0, 1.0f, 1.0f},
        // Tier3 银
        {"pickaxe_silver", "银镐子", "银制镐子，节省体力", 1.4f, 1.0f, 0.1f, 0, 1.0f, 1.0f},
        // Tier4 金
        {"pickaxe_gold", "金镐子", "金制镐子，高效采矿", 1.6f, 1.0f, 0.2f, 0, 1.0f, 1.0f},
        // Tier5 灵金
        {"pickaxe_spirit", "灵金镐子", "灵金镐子！采矿产量+50%", 2.0f, 1.2f, 0.35f, 0, 1.5f, 1.0f},
    },
};

ToolEffect gRuntimeToolEffectTable
    [static_cast<std::size_t>(ToolType::Count)]
    [static_cast<std::size_t>(ToolTier::COUNT)] = {};
bool gHasRuntimeToolEffectTable = false;

std::string TrimText(std::string text) {
    return CloudSeamanor::infrastructure::DataRegistry::Trim(text);
}

bool TryParseToolType(const std::string& text, ToolType& out_type) {
    static const std::unordered_map<std::string, ToolType> kMap = {
        {"Hoe", ToolType::Hoe},
        {"WateringCan", ToolType::WateringCan},
        {"Sprinkler", ToolType::Sprinkler},
        {"Sickle", ToolType::Sickle},
        {"Scissors", ToolType::Scissors},
        {"Axe", ToolType::Axe},
        {"Pickaxe", ToolType::Pickaxe},
    };
    const auto it = kMap.find(text);
    if (it == kMap.end()) {
        return false;
    }
    out_type = it->second;
    return true;
}

bool TryParseToolTier(const std::string& text, ToolTier& out_tier) {
    static const std::unordered_map<std::string, ToolTier> kMap = {
        {"Tier1", ToolTier::Tier1},
        {"Tier2", ToolTier::Tier2},
        {"Tier3", ToolTier::Tier3},
        {"Tier4", ToolTier::Tier4},
        {"Tier5", ToolTier::Tier5},
    };
    const auto it = kMap.find(text);
    if (it == kMap.end()) {
        return false;
    }
    out_tier = it->second;
    return true;
}

const char* kToolTypeNames[static_cast<std::size_t>(ToolType::Count)] = {
    "锄头",
    "水壶",
    "洒水器",
    "镰刀",
    "剪刀",
    "斧头",
    "镐子"
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
    if (gHasRuntimeToolEffectTable) {
        return gRuntimeToolEffectTable[static_cast<std::size_t>(type)][static_cast<std::size_t>(tier)];
    }
    return kFallbackToolEffectTable[static_cast<std::size_t>(type)][static_cast<std::size_t>(tier)];
}

// ============================================================================
// 【ToolSystem::Initialize】初始化
// ============================================================================
void ToolSystem::Initialize(const ToolUpgradeConfig& config) {
    config_ = config;
    (void)LoadFromFile();

    for (std::size_t i = 0; i < static_cast<std::size_t>(ToolType::Count); ++i) {
        const auto type = static_cast<ToolType>(i);
        tool_levels_[type] = ToolLevel{};

        // 填充效果表引用
        tool_effects_[type] = GetToolEffectTable(type, 1);
    }
}

bool ToolSystem::LoadFromFile(const std::string& file_path) {
    std::ifstream data_stream(file_path);
    if (!data_stream.is_open()) {
        gHasRuntimeToolEffectTable = false;
        return false;
    }

    ToolEffect runtime_table
        [static_cast<std::size_t>(ToolType::Count)]
        [static_cast<std::size_t>(ToolTier::COUNT)] = {};
    for (std::size_t type_index = 0; type_index < static_cast<std::size_t>(ToolType::Count); ++type_index) {
        for (std::size_t tier_index = 0; tier_index < static_cast<std::size_t>(ToolTier::COUNT); ++tier_index) {
            runtime_table[type_index][tier_index] = kFallbackToolEffectTable[type_index][tier_index];
        }
    }
    std::string line;
    bool is_header = true;
    int loaded_rows = 0;
    while (std::getline(data_stream, line)) {
        const std::string trimmed = TrimText(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        if (is_header) {
            is_header = false;
            continue;
        }

        const auto cells = CloudSeamanor::infrastructure::DataRegistry::SplitCsvLine(line);
        if (cells.size() < 11) {
            continue;
        }

        ToolType type;
        ToolTier tier;
        if (!TryParseToolType(TrimText(cells[1]), type)
            || !TryParseToolTier(TrimText(cells[2]), tier)) {
            continue;
        }

        ToolEffect effect;
        effect.name = TrimText(cells[0]);
        effect.display_name = TrimText(cells[3]);
        effect.description = TrimText(cells[4]);
        try {
            effect.efficiency_multiplier = std::stof(TrimText(cells[5]));
            effect.range_multiplier = std::stof(TrimText(cells[6]));
            effect.cost_reduction = std::stof(TrimText(cells[7]));
            effect.sprinkler_coverage = std::stoi(TrimText(cells[8]));
            effect.collect_efficiency = std::stof(TrimText(cells[9]));
            effect.collect_range = std::stof(TrimText(cells[10]));
        } catch (...) {
            continue;
        }

        runtime_table[static_cast<std::size_t>(type)][static_cast<std::size_t>(tier)] = std::move(effect);
        ++loaded_rows;
    }

    if (loaded_rows == 0) {
        gHasRuntimeToolEffectTable = false;
        return false;
    }

    for (std::size_t type_index = 0; type_index < static_cast<std::size_t>(ToolType::Count); ++type_index) {
        for (std::size_t tier_index = 0; tier_index < static_cast<std::size_t>(ToolTier::COUNT); ++tier_index) {
            gRuntimeToolEffectTable[type_index][tier_index] = runtime_table[type_index][tier_index];
        }
    }
    gHasRuntimeToolEffectTable = true;
    return true;
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

float ToolSystem::GetCollectEfficiency(ToolType type) const {
    return GetEffect(type).collect_efficiency;
}

float ToolSystem::GetCollectRange(ToolType type) const {
    return GetEffect(type).collect_range;
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
