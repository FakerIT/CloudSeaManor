#pragma once

// ============================================================================
// 【GameWorldSystems】游戏世界系统
// ============================================================================
// 统一管理游戏中的各种系统，包括技能、节日、工坊、动态人生等。
//
// 主要职责：
// - 管理领域系统（技能、节日、工坊、动态人生）
// - 管理云海系统和契约系统
// - 提供系统初始化和更新接口
//
// 设计原则：
// - 所有系统集中管理，便于协调更新
// - 系统之间通过引用互相访问
// - 提供简洁的初始化和更新接口
// ============================================================================

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/CloudGuardianContract.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/FestivalSystem.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/domain/NpcDevelopmentSystem.hpp"
#include "CloudSeamanor/domain/ManorEcologySystem.hpp"
#include "CloudSeamanor/domain/PlayerMemorySystem.hpp"
#include "CloudSeamanor/domain/TeaSpiritDexSystem.hpp"
#include "CloudSeamanor/engine/NpcDialogueManager.hpp"
#include "CloudSeamanor/engine/PickupSystem.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【GameWorldSystems】游戏世界系统类
// ============================================================================
class GameWorldSystems {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    GameWorldSystems();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize();
    void InitializeContracts();

    // ========================================================================
    // 【系统访问器】
    // ========================================================================
    [[nodiscard]] CloudSeamanor::domain::CloudSystem& GetCloud() { return cloud_; }
    [[nodiscard]] const CloudSeamanor::domain::CloudSystem& GetCloud() const { return cloud_; }
    [[nodiscard]] CloudSeamanor::domain::CloudSystem& MutableCloud() { return cloud_; }
    [[nodiscard]] CloudSeamanor::domain::CloudGuardianContract& GetContracts() { return contracts_; }
    [[nodiscard]] const CloudSeamanor::domain::CloudGuardianContract& GetContracts() const { return contracts_; }
    [[nodiscard]] CloudSeamanor::domain::CloudGuardianContract& MutableContracts() { return contracts_; }
    [[nodiscard]] CloudSeamanor::domain::SkillSystem& GetSkills() { return skills_; }
    [[nodiscard]] const CloudSeamanor::domain::SkillSystem& GetSkills() const { return skills_; }
    [[nodiscard]] CloudSeamanor::domain::SkillSystem& MutableSkills() { return skills_; }
    [[nodiscard]] CloudSeamanor::domain::FestivalSystem& GetFestivals() { return festivals_; }
    [[nodiscard]] const CloudSeamanor::domain::FestivalSystem& GetFestivals() const { return festivals_; }
    [[nodiscard]] CloudSeamanor::domain::FestivalSystem& MutableFestivals() { return festivals_; }
    [[nodiscard]] CloudSeamanor::domain::WorkshopSystem& GetWorkshop() { return workshop_; }
    [[nodiscard]] const CloudSeamanor::domain::WorkshopSystem& GetWorkshop() const { return workshop_; }
    [[nodiscard]] CloudSeamanor::domain::WorkshopSystem& MutableWorkshop() { return workshop_; }
    [[nodiscard]] CloudSeamanor::domain::DynamicLifeSystem& GetDynamicLife() { return dynamic_life_; }
    [[nodiscard]] const CloudSeamanor::domain::DynamicLifeSystem& GetDynamicLife() const { return dynamic_life_; }
    [[nodiscard]] CloudSeamanor::domain::DynamicLifeSystem& MutableDynamicLife() { return dynamic_life_; }
    [[nodiscard]] CloudSeamanor::domain::NpcDevelopmentSystem& GetNpcDevelopment() { return npc_development_; }
    [[nodiscard]] const CloudSeamanor::domain::NpcDevelopmentSystem& GetNpcDevelopment() const { return npc_development_; }
    [[nodiscard]] CloudSeamanor::domain::NpcDevelopmentSystem& MutableNpcDevelopment() { return npc_development_; }
    [[nodiscard]] CloudSeamanor::domain::ManorEcologySystem& GetEcology() { return ecology_; }
    [[nodiscard]] const CloudSeamanor::domain::ManorEcologySystem& GetEcology() const { return ecology_; }
    [[nodiscard]] CloudSeamanor::domain::ManorEcologySystem& MutableEcology() { return ecology_; }
    [[nodiscard]] CloudSeamanor::domain::PlayerMemorySystem& GetMemory() { return memory_; }
    [[nodiscard]] const CloudSeamanor::domain::PlayerMemorySystem& GetMemory() const { return memory_; }
    [[nodiscard]] CloudSeamanor::domain::PlayerMemorySystem& MutableMemory() { return memory_; }
    [[nodiscard]] CloudSeamanor::domain::TeaSpiritDexSystem& GetTeaSpiritDex() { return tea_spirit_dex_; }
    [[nodiscard]] const CloudSeamanor::domain::TeaSpiritDexSystem& GetTeaSpiritDex() const { return tea_spirit_dex_; }
    [[nodiscard]] CloudSeamanor::domain::TeaSpiritDexSystem& MutableTeaSpiritDex() { return tea_spirit_dex_; }
    [[nodiscard]] NpcDialogueManager& GetDialogueManager() { return dialogue_manager_; }
    [[nodiscard]] const NpcDialogueManager& GetDialogueManager() const { return dialogue_manager_; }
    [[nodiscard]] NpcDialogueManager& MutableDialogueManager() { return dialogue_manager_; }
    [[nodiscard]] PickupSystem& GetPickups() { return pickups_; }
    [[nodiscard]] const PickupSystem& GetPickups() const { return pickups_; }
    [[nodiscard]] PickupSystem& MutablePickups() { return pickups_; }

    // ========================================================================
    // 【每日更新】
    // ========================================================================
    void UpdateDaily(CloudSeamanor::domain::Season season, int day_in_season, float cloud_density);
    void CheckDailyTransitions(int seeded_plots_count, bool main_house_repaired, bool spirit_beast_interacted);

    // ========================================================================
    // 【系统更新】
    // ========================================================================
    void UpdateWorkshop(
        float delta_time,
        float cloud_density,
        int skill_level,
        int tool_level,
        std::unordered_map<std::string, int>& output_items);
    std::vector<CloudSeamanor::domain::MachineCompletionInfo> UpdateWorkshopWithCompletion(
        float delta_time,
        float cloud_density,
        int skill_level,
        int tool_level,
        std::unordered_map<std::string, int>& output_items);
    void CheckContractUnlocks();
    void AddPlayerInfluence(int value);

    // ========================================================================
    // 【存档】
    // ========================================================================
    [[nodiscard]] std::string SaveContractsState() const;
    void LoadContractsState(const std::string& state);
    [[nodiscard]] std::string SaveEcologyState() const;
    void LoadEcologyState(const std::string& state);
    [[nodiscard]] std::string SaveMemoryState() const;
    void LoadMemoryState(const std::string& state);
    [[nodiscard]] std::string SaveTeaSpiritDexState() const;
    void LoadTeaSpiritDexState(const std::string& state);

private:
    CloudSeamanor::domain::CloudSystem cloud_;
    CloudSeamanor::domain::CloudGuardianContract contracts_;
    CloudSeamanor::domain::SkillSystem skills_;
    CloudSeamanor::domain::FestivalSystem festivals_;
    CloudSeamanor::domain::WorkshopSystem workshop_;
    CloudSeamanor::domain::DynamicLifeSystem dynamic_life_;
    CloudSeamanor::domain::NpcDevelopmentSystem npc_development_;
    CloudSeamanor::domain::ManorEcologySystem ecology_;
    CloudSeamanor::domain::PlayerMemorySystem memory_;
    CloudSeamanor::domain::TeaSpiritDexSystem tea_spirit_dex_;
    NpcDialogueManager dialogue_manager_;
    PickupSystem pickups_;
};

} // namespace CloudSeamanor::engine
