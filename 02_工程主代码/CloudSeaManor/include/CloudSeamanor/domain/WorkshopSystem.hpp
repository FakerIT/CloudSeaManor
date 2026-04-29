#pragma once

// ============================================================================
// 【WorkshopSystem】工坊制作与加工系统
// ============================================================================
// 统一管理工坊机器、加工配方和产物生成。
//
// 主要职责：
// - 管理工坊机器（制茶机、发酵罐等）
// - 处理物品加工配方（配方数据由 RecipeTable 驱动）
// - 计算加工时间和产物
// - 应用云海品质加成
//
// 与其他系统的关系：
// - 依赖：RecipeTable（配方数据）、CloudSystem（云海加成）、Inventory（原料消耗/产物增加）
// - 被依赖：GameAppHud（加工状态显示）、GameAppSave（存档/读档）
//
// 设计原则：
// - 工坊产物受云海浓度影响品质
// - 机器可绑定灵兽进行协助
// - 加工成功率和品质由多因素决定
// - 配方定义迁移到 RecipeTable.csv，数据驱动
// ============================================================================

#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/domain/RecipeData.hpp"

#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CloudSeamanor::domain {

enum class TeaProcessStage : std::uint8_t {
    Idle = 0,
    FreshLeaf = 1,
    Withering = 2,
    Fixation = 3,
    Rolling = 4,
    Drying = 5,
    FinishedTea = 6
};

// ============================================================================
// 【MachineState】机器状态
// ============================================================================
// 表示一台工坊机器的当前运行时状态。
struct MachineState {
    std::string machine_id;     // 机器 ID
    std::string recipe_id;      // 当前配方 ID（空字符串表示空闲）
    float progress = 0.0f;      // 加工进度（0-100）
    bool is_processing = false;  // 是否正在加工
    int required_level = 1;
    TeaProcessStage stage = TeaProcessStage::Idle;
    float stage_progress = 0.0f;
    float quality_score = 0.0f;  // 品质分数（用于茶灵解锁判定）
};

// ============================================================================
// 【MachineCompletionInfo】机器完成信息
// ============================================================================
// 用于在机器完成加工时传递详细信息给调用者。
struct MachineCompletionInfo {
    std::string machine_id;     // 机器 ID
    std::string recipe_id;     // 配方 ID
    std::string output_item;   // 产出物品 ID
    int output_count = 0;      // 产出数量
    float quality_score = 0.0f;  // 品质分数
};

[[nodiscard]] const char* TeaProcessStageText(TeaProcessStage stage) noexcept;

// ============================================================================
// 【WorkshopSystem】工坊系统领域对象
// ============================================================================
// 管理工坊机器和配方执行。
//
// 设计决策：
// - 配方定义由 RecipeTable 全局单例提供，无需内部存储
// - 支持多种工坊机器（制茶机、发酵罐等）
// - 加工进度实时更新，云海浓度影响品质加成
class WorkshopSystem {
public:
    // ========================================================================
    // 【Initialize】初始化工坊系统
    // ========================================================================
    // @note 同时初始化 RecipeTable（惰性加载 RecipeTable.csv）
    void Initialize();

    // ========================================================================
    // 【Update】更新工坊机器进度
    // ========================================================================
    // @param delta_time  距离上次更新的时间（秒）
    // @param cloud_density  云海密度（影响加工速度）
    // @param output_items  用于记录产出的物品
    // @return 完成加工的机器详细信息列表（包含配方 ID、产出物品、品质分数）
    std::vector<MachineCompletionInfo> Update(
        float delta_time,
        float cloud_density,
        int skill_level,
        int tool_level,
        std::unordered_map<std::string, int>& output_items
    );

    // ========================================================================
    // 【StartProcessing】开始加工
    // ========================================================================
    // @param machine_id  机器 ID
    // @param recipe_id   配方 ID
    // @param inventory   背包引用（用于检查原料和扣减）
    // @return true 表示开始成功
    bool StartProcessing(
        const std::string& machine_id,
        const std::string& recipe_id,
        Inventory& inventory
    );

    // ========================================================================
    // 【GetMachine】获取机器状态
    // ========================================================================
    [[nodiscard]] const MachineState* GetMachine(const std::string& machine_id) const;

    // ========================================================================
    // 【GetMachineProgress】获取机器进度百分比
    // ========================================================================
    [[nodiscard]] float GetMachineProgress(const std::string& machine_id) const noexcept;

    // ========================================================================
    // 【GetMachineDuration】获取配方所需总时间
    // ========================================================================
    [[nodiscard]] float GetMachineDuration(const std::string& machine_id) const noexcept;

    // ========================================================================
    // 【SetMachineState】设置机器状态（用于存档恢复）
    // ========================================================================
    bool SetMachineState(const std::string& machine_id,
                         const std::string& recipe_id,
                         float progress,
                         bool is_processing);

    // ========================================================================
    // 【GetRecipe】获取配方（委托 RecipeTable）
    // ========================================================================
    [[nodiscard]] const RecipeDefinition* GetRecipe(const std::string& recipe_id) const;
    [[nodiscard]] int RequiredLevelForRecipe(const RecipeDefinition& recipe) const noexcept;
    [[nodiscard]] bool IsRecipeUnlocked(const RecipeDefinition& recipe) const noexcept;
    void UnlockRecipe(const std::string& recipe_id);
    void ResetUnlockedRecipes();

    // ========================================================================
    // 【GetRecipesForMachine】获取指定机器的所有配方
    // ========================================================================
    [[nodiscard]] std::vector<const RecipeDefinition*> GetRecipesForMachine(
        const std::string& machine_id) const;

    // ========================================================================
    // 【访问器】
    // ========================================================================
    [[nodiscard]] const std::vector<MachineState>& GetMachines() const noexcept {
        return machines_;
    }
    [[nodiscard]] int Level() const noexcept { return level_; }
    [[nodiscard]] int UnlockedSlots() const noexcept { return unlocked_slots_; }
    bool Upgrade(int level, int slots);

private:
    std::vector<MachineState> machines_;
    int level_ = 1;
    int unlocked_slots_ = 1;
    std::unordered_set<std::string> unlocked_recipe_ids_;
};
        
} // namespace CloudSeamanor::domain
