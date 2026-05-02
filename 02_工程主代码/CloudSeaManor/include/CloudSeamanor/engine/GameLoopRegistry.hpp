#pragma once

// ============================================================================
// 【GameLoopRegistry】游戏循环系统注册器
// ============================================================================
// 统一注册所有游戏系统到 GameLoopCoordinator
//
// 注册表：
//   阶段 0: TIME
//     - GameClock (priority: 0)
//     - StaminaSystem (priority: 1)
//     - HungerSystem (priority: 2)
//     - BuffSystem (priority: 3)
//
//   阶段 1: INPUT
//     - InputManager (priority: 0)
//     - PlayerMovementSystem (priority: 1)
//
//   阶段 2: WORLD
//     - CropGrowthSystem (priority: 0)
//     - WorkshopSystem (priority: 1)
//     - NpcScheduleSystem (priority: 2)
//     - NpcDialogueManager (priority: 3)
//     - SpiritBeastSystem (priority: 4)
//     - PickupSystemRuntime (priority: 5)
//     - QuestManager (priority: 6)
//     - AchievementSystem (priority: 7)
//
//   阶段 3: COMBAT
//     - BattleManager (priority: 0)
//
//   阶段 4: RUNTIME
//     - TutorialSystem (priority: 0)
//     - MainPlotSystem (priority: 1)
//     - NpcDeliverySystem (priority: 2)
//     - CloudSystem (priority: 3)
//     - EventBus (priority: 4)
//
//   阶段 5: PARTICLES
//     - ParticleSystem (priority: 0)
// ============================================================================

#include "CloudSeamanor/engine/GameLoopCoordinator.hpp"

// 前向声明（避免循环依赖）
// 实现在 GameLoopRegistry.cpp 中
namespace CloudSeamanor::engine {
class GameRuntime;
class GameWorldState;
class GameWorldSystems;
}

namespace CloudSeamanor::engine {

// ============================================================================
// 【SystemLoopRegistry】系统循环注册器
// ============================================================================
class SystemLoopRegistry {
public:
    /**
     * @brief 注册所有游戏系统
     * @param coordinator 循环协调器
     * @param world_state 游戏世界状态
     * @param systems 游戏世界系统
     * @param runtime 游戏运行时
     */
    static void RegisterAllSystems(
        GameLoopCoordinator& coordinator,
        GameWorldState& world_state,
        GameWorldSystems& systems,
        GameRuntime& runtime);

    /**
     * @brief 注销所有游戏系统
     */
    static void UnregisterAllSystems(GameLoopCoordinator& coordinator);

private:
    // 阶段 0: 时间系统
    static void RegisterTimeSystems_(
        GameLoopCoordinator& coordinator,
        GameWorldState& world_state);

    // 阶段 1: 输入系统
    static void RegisterInputSystems_(
        GameLoopCoordinator& coordinator,
        GameWorldState& world_state);

    // 阶段 2: 世界系统
    static void RegisterWorldSystems_(
        GameLoopCoordinator& coordinator,
        GameWorldState& world_state,
        GameWorldSystems& systems);

    // 阶段 3: 战斗系统
    static void RegisterCombatSystems_(
        GameLoopCoordinator& coordinator,
        GameWorldState& world_state,
        GameRuntime& runtime);

    // 阶段 4: 运行时系统
    static void RegisterRuntimeSystems_(
        GameLoopCoordinator& coordinator,
        GameWorldState& world_state,
        GameWorldSystems& systems,
        GameRuntime& runtime);

    // 阶段 5: 粒子系统
    static void RegisterParticleSystems_(GameLoopCoordinator& coordinator);
};

// ============================================================================
// 【系统名称常量】
// ============================================================================
namespace LoopSystemNames {
    // 阶段 0: 时间
    constexpr const char* kGameClock = "GameClock";
    constexpr const char* kStaminaSystem = "StaminaSystem";
    constexpr const char* kHungerSystem = "HungerSystem";
    constexpr const char* kBuffSystem = "BuffSystem";

    // 阶段 1: 输入
    constexpr const char* kPlayerMovement = "PlayerMovement";

    // 阶段 2: 世界
    constexpr const char* kCropGrowth = "CropGrowth";
    constexpr const char* kWorkshop = "Workshop";
    constexpr const char* kNpcSchedule = "NpcSchedule";
    constexpr const char* kNpcDialogue = "NpcDialogue";
    constexpr const char* kSpiritBeast = "SpiritBeast";
    constexpr const char* kPickupSystem = "PickupSystem";
    constexpr const char* kQuestManager = "QuestManager";
    constexpr const char* kAchievementSystem = "AchievementSystem";
    constexpr const char* kTeaGarden = "TeaGarden";

    // 阶段 3: 战斗
    constexpr const char* kBattleManager = "BattleManager";

    // 阶段 4: 运行时
    constexpr const char* kTutorialSystem = "TutorialSystem";
    constexpr const char* kMainPlotSystem = "MainPlotSystem";
    constexpr const char* kNpcDeliverySystem = "NpcDeliverySystem";
    constexpr const char* kCloudSystem = "CloudSystem";
    constexpr const char* kSceneTransition = "SceneTransition";
    constexpr const char* kSpiritBeastDispatch = "SpiritBeastDispatch";

    // 阶段 5: 粒子
    constexpr const char* kParticleSystem = "ParticleSystem";
}

} // namespace CloudSeamanor::engine
