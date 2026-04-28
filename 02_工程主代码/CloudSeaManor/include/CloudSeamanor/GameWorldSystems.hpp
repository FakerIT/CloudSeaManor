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

#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/CloudGuardianContract.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/NpcDialogueManager.hpp"
#include "CloudSeamanor/PickupSystem.hpp"

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
    void UpdateWorkshop(float delta_time, float cloud_density, std::unordered_map<std::string, int>& output_items);
    void CheckContractUnlocks();
    void AddPlayerInfluence(int value);

    // ========================================================================
    // 【存档】
    // ========================================================================
    [[nodiscard]] std::string SaveContractsState() const;
    void LoadContractsState(const std::string& state);

private:
    CloudSeamanor::domain::CloudSystem cloud_;
    CloudSeamanor::domain::CloudGuardianContract contracts_;
    CloudSeamanor::domain::SkillSystem skills_;
    CloudSeamanor::domain::FestivalSystem festivals_;
    CloudSeamanor::domain::WorkshopSystem workshop_;
    CloudSeamanor::domain::DynamicLifeSystem dynamic_life_;
    NpcDialogueManager dialogue_manager_;
    PickupSystem pickups_;
};

} // namespace CloudSeamanor::engine
