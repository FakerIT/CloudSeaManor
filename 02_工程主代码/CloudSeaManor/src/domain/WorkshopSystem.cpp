#include "CloudSeamanor/domain/WorkshopSystem.hpp"

#include "CloudSeamanor/infrastructure/Logger.hpp"
#include "CloudSeamanor/domain/RecipeData.hpp"

#include <algorithm>

namespace CloudSeamanor::domain {

namespace {

bool HasTag_(const RecipeDefinition& recipe, const std::string& tag) {
    for (const auto& t : recipe.tags) {
        if (t == tag) {
            return true;
        }
    }
    return false;
}

TeaProcessStage NextTeaStage_(TeaProcessStage stage) {
    switch (stage) {
    case TeaProcessStage::FreshLeaf: return TeaProcessStage::Withering;
    case TeaProcessStage::Withering: return TeaProcessStage::Fixation;
    case TeaProcessStage::Fixation: return TeaProcessStage::Rolling;
    case TeaProcessStage::Rolling: return TeaProcessStage::Drying;
    case TeaProcessStage::Drying: return TeaProcessStage::FinishedTea;
    case TeaProcessStage::FinishedTea:
    case TeaProcessStage::Idle:
    default:
        return TeaProcessStage::FinishedTea;
    }
}

}  // namespace

const char* TeaProcessStageText(TeaProcessStage stage) noexcept {
    switch (stage) {
    case TeaProcessStage::Idle: return "待机";
    case TeaProcessStage::FreshLeaf: return "鲜叶";
    case TeaProcessStage::Withering: return "萎凋";
    case TeaProcessStage::Fixation: return "杀青";
    case TeaProcessStage::Rolling: return "揉捻";
    case TeaProcessStage::Drying: return "干燥";
    case TeaProcessStage::FinishedTea: return "成茶";
    }
    return "待机";
}

// ============================================================================
// 【Initialize】初始化工坊系统
// ============================================================================
void WorkshopSystem::Initialize() {
    // 统一真值源：仅允许从 recipes.csv 加载，避免双表漂移。
    auto& recipe_table = GetGlobalRecipeTable();
    if (!recipe_table.LoadFromFile("assets/data/recipes.csv")) {
        infrastructure::Logger::Error(
            "WorkshopSystem: recipes.csv 加载失败，工坊配方不可用。"
            "（RecipeTable.csv 已下线为弃用表，不再作为运行时回退）");
    }

    machines_.clear();
    machines_.push_back({"tea_machine", "", 0.0f, false, 1, TeaProcessStage::Idle, 0.0f, 0.0f});
    machines_.push_back({"ferment_machine", "", 0.0f, false, 2, TeaProcessStage::Idle, 0.0f, 0.0f});
    level_ = 1;
    unlocked_slots_ = 1;
    unlocked_recipe_ids_.clear();
}

// ============================================================================
// 【Update】更新机器进度
// ============================================================================
std::vector<std::string> WorkshopSystem::Update(
    float delta_time,
    float cloud_density,
    int skill_level,
    int tool_level,
    std::unordered_map<std::string, int>& output_items
) {
    std::vector<std::string> completed_machines;

    for (auto& machine : machines_) {
        if (!machine.is_processing) {
            continue;
        }

        const RecipeDefinition* recipe = GetRecipe(machine.recipe_id);
        if (!recipe) {
            continue;
        }

        // 云海加成：cloud_density * 0.5 表示灵气越浓加工越快
        const float speed = 1.0f + cloud_density * 0.5f;
        machine.progress += (100.0f / static_cast<float>(recipe->process_time)) * delta_time * speed;
        machine.stage_progress += (100.0f / 5.0f) * delta_time * speed;
        if (machine.stage_progress >= 100.0f && machine.stage != TeaProcessStage::FinishedTea) {
            machine.stage = NextTeaStage_(machine.stage);
            machine.stage_progress = 0.0f;
        }
        machine.quality_score += delta_time * std::max(0.0f, cloud_density) * 0.2f
            + static_cast<float>(std::max(0, skill_level - 1)) * 0.04f
            + static_cast<float>(std::max(0, tool_level - 1)) * 0.03f;

        if (machine.progress >= 100.0f) {
            output_items[recipe->output_item] += recipe->output_count;
            completed_machines.push_back(machine.machine_id);
            machine.progress = 0.0f;
            machine.is_processing = false;
            machine.recipe_id = "";
            machine.stage = TeaProcessStage::Idle;
            machine.stage_progress = 0.0f;
            machine.quality_score = 0.0f;
        }
    }

    return completed_machines;
}

// ============================================================================
// 【StartProcessing】开始加工
// ============================================================================
bool WorkshopSystem::StartProcessing(
    const std::string& machine_id,
    const std::string& recipe_id,
    Inventory& inventory
) {
    const RecipeDefinition* recipe = GetRecipe(recipe_id);
    if (!recipe) {
        return false;
    }
    if (!IsRecipeUnlocked(*recipe)) {
        return false;
    }

    if (inventory.CountOf(recipe->input_item) < recipe->input_count) {
        return false;
    }

    for (auto& machine : machines_) {
        if (machine.machine_id == machine_id) {
            if (level_ < machine.required_level) {
                return false;
            }
            int active = 0;
            for (const auto& m : machines_) {
                if (m.is_processing) ++active;
            }
            if (active >= unlocked_slots_) {
                return false;
            }
            const auto remove_result = inventory.TryRemoveItem(recipe->input_item, recipe->input_count);
            if (!remove_result) {
                return false;
            }
            machine.recipe_id = recipe_id;
            machine.progress = 0.0f;
            machine.is_processing = true;
            machine.stage = TeaProcessStage::FreshLeaf;
            machine.stage_progress = 0.0f;
            machine.quality_score = 0.0f;
            return true;
        }
    }

    return false;
}

bool WorkshopSystem::Upgrade(int level, int slots) {
    if (level <= level_ || slots <= unlocked_slots_) {
        return false;
    }
    level_ = level;
    unlocked_slots_ = slots;
    return true;
}

int WorkshopSystem::RequiredLevelForRecipe(const RecipeDefinition& recipe) const noexcept {
    if (recipe.machine_id == "ferment_machine") {
        return 2;
    }
    if (HasTag_(recipe, "legendary")) {
        return 4;
    }
    if (HasTag_(recipe, "advanced") || HasTag_(recipe, "upgrade") || HasTag_(recipe, "craft")) {
        return 3;
    }
    if (HasTag_(recipe, "spirit") || HasTag_(recipe, "tool")) {
        return 2;
    }
    return 1;
}

bool WorkshopSystem::IsRecipeUnlocked(const RecipeDefinition& recipe) const noexcept {
    if (unlocked_recipe_ids_.count(recipe.id) > 0) {
        return true;
    }
    return level_ >= RequiredLevelForRecipe(recipe);
}

void WorkshopSystem::UnlockRecipe(const std::string& recipe_id) {
    if (!recipe_id.empty()) {
        unlocked_recipe_ids_.insert(recipe_id);
    }
}

void WorkshopSystem::ResetUnlockedRecipes() {
    unlocked_recipe_ids_.clear();
}

// ============================================================================
// 【GetMachine】获取机器状态
// ============================================================================
const MachineState* WorkshopSystem::GetMachine(const std::string& machine_id) const {
    for (const auto& machine : machines_) {
        if (machine.machine_id == machine_id) {
            return &machine;
        }
    }
    return nullptr;
}

// ============================================================================
// 【GetRecipe】获取配方（委托 RecipeTable）
// ============================================================================
const RecipeDefinition* WorkshopSystem::GetRecipe(const std::string& recipe_id) const {
    return GetGlobalRecipeTable().Get(recipe_id);
}

// ============================================================================
// 【GetRecipesForMachine】获取指定机器的所有配方
// ============================================================================
std::vector<const RecipeDefinition*> WorkshopSystem::GetRecipesForMachine(
    const std::string& machine_id) const {
    return GetGlobalRecipeTable().GetByMachine(machine_id);
}

// ============================================================================
// 【GetMachineProgress】获取机器进度百分比
// ============================================================================
float WorkshopSystem::GetMachineProgress(const std::string& machine_id) const noexcept {
    const MachineState* machine = GetMachine(machine_id);
    return machine ? machine->progress : 0.0f;
}

// ============================================================================
// 【GetMachineDuration】获取配方所需总时间
// ============================================================================
float WorkshopSystem::GetMachineDuration(const std::string& machine_id) const noexcept {
    const MachineState* machine = GetMachine(machine_id);
    if (!machine || machine->recipe_id.empty()) {
        return 60.0f;
    }
    const RecipeDefinition* recipe = GetRecipe(machine->recipe_id);
    return recipe ? static_cast<float>(recipe->process_time) : 60.0f;
}

// ============================================================================
// 【SetMachineState】设置机器状态（存档恢复）
// ============================================================================
bool WorkshopSystem::SetMachineState(
    const std::string& machine_id,
    const std::string& recipe_id,
    float progress,
    bool is_processing
) {
    for (auto& machine : machines_) {
        if (machine.machine_id == machine_id) {
            machine.recipe_id = recipe_id;
            machine.progress = std::max(0.0f, std::min(100.0f, progress));
            machine.is_processing = is_processing;
            if (!is_processing || recipe_id.empty()) {
                machine.stage = TeaProcessStage::Idle;
                machine.stage_progress = 0.0f;
                machine.quality_score = 0.0f;
            } else {
                machine.stage = TeaProcessStage::FreshLeaf;
                machine.stage_progress = 0.0f;
            }
            return true;
        }
    }
    return false;
}

} // namespace CloudSeamanor::domain
