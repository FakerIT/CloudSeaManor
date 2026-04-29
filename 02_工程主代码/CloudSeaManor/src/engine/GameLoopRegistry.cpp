#include "CloudSeamanor/engine/GameLoopRegistry.hpp"
#include "CloudSeamanor/engine/GameRuntime.hpp"
#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/GameWorldSystems.hpp"
#include "CloudSeamanor/engine/systems/PlayerMovementSystem.hpp"
#include "CloudSeamanor/engine/systems/PickupSystemRuntime.hpp"
#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"
#include "CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp"
#include "CloudSeamanor/engine/systems/NpcScheduleSystem.hpp"
#include "CloudSeamanor/engine/systems/TutorialSystem.hpp"
#include "CloudSeamanor/engine/BattleManager.hpp"
#include "CloudSeamanor/engine/MainPlotSystem.hpp"
#include "CloudSeamanor/engine/systems/NpcDeliverySystem.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

namespace CloudSeamanor::engine {

using namespace LoopSystemNames;

// ============================================================================
// 【RegisterAllSystems】注册所有系统
// ============================================================================
void SystemLoopRegistry::RegisterAllSystems(
    GameLoopCoordinator& coordinator,
    GameWorldState& world_state,
    GameWorldSystems& systems,
    GameRuntime& runtime) {

    RegisterTimeSystems_(coordinator, world_state);
    RegisterInputSystems_(coordinator, world_state);
    RegisterWorldSystems_(coordinator, world_state, systems);
    RegisterCombatSystems_(coordinator, world_state, runtime);
    RegisterRuntimeSystems_(coordinator, world_state, systems, runtime);
    RegisterParticleSystems_(coordinator);

    CloudSeamanor::infrastructure::Logger::Info(
        "SystemLoopRegistry: Registered all systems to GameLoopCoordinator");
}

// ============================================================================
// 【UnregisterAllSystems】注销所有系统
// ============================================================================
void SystemLoopRegistry::UnregisterAllSystems(GameLoopCoordinator& coordinator) {
    // 时间系统
    coordinator.UnregisterSystem(kGameClock);
    coordinator.UnregisterSystem(kStaminaSystem);
    coordinator.UnregisterSystem(kHungerSystem);
    coordinator.UnregisterSystem(kBuffSystem);

    // 输入系统
    coordinator.UnregisterSystem(kPlayerMovement);

    // 世界系统
    coordinator.UnregisterSystem(kCropGrowth);
    coordinator.UnregisterSystem(kWorkshop);
    coordinator.UnregisterSystem(kNpcSchedule);
    coordinator.UnregisterSystem(kNpcDialogue);
    coordinator.UnregisterSystem(kSpiritBeast);
    coordinator.UnregisterSystem(kPickupSystem);
    coordinator.UnregisterSystem(kQuestManager);
    coordinator.UnregisterSystem(kAchievementSystem);
    coordinator.UnregisterSystem(kTeaGarden);

    // 战斗系统
    coordinator.UnregisterSystem(kBattleManager);

    // 运行时系统
    coordinator.UnregisterSystem(kTutorialSystem);
    coordinator.UnregisterSystem(kMainPlotSystem);
    coordinator.UnregisterSystem(kNpcDeliverySystem);
    coordinator.UnregisterSystem(kCloudSystem);
    coordinator.UnregisterSystem(kSceneTransition);
    coordinator.UnregisterSystem(kSpiritBeastDispatch);

    // 粒子系统
    coordinator.UnregisterSystem(kParticleSystem);
}

// ============================================================================
// 【RegisterTimeSystems_】注册时间系统
// ============================================================================
void SystemLoopRegistry::RegisterTimeSystems_(
    GameLoopCoordinator& coordinator,
    GameWorldState& world_state) {

    // GameClock - 最高优先级时间更新
    coordinator.RegisterSystem(
        LoopPhase::Time,
        kGameClock,
        [&world_state](float delta_seconds) {
            world_state.MutableClock().Tick(delta_seconds);
        },
        0);

    // StaminaSystem - 体力系统
    coordinator.RegisterSystem(
        LoopPhase::Time,
        kStaminaSystem,
        [&world_state](float delta_seconds) {
            const float recover_rate = world_state.GetStamina().GetRecoverPerSecond()
                * world_state.GetHunger().StaminaRecoveryMultiplier()
                * world_state.GetBuffs().StaminaRecoveryMultiplier();
            world_state.MutableStamina().Update(delta_seconds, recover_rate);
        },
        [&world_state]() {
            // 体力未满时更新
            return world_state.MutableStamina().Ratio() < 1.0f;
        },
        1);

    // HungerSystem - 饥饿系统
    coordinator.RegisterSystem(
        LoopPhase::Time,
        kHungerSystem,
        [&world_state](float delta_seconds) {
            world_state.MutableHunger().Tick(delta_seconds);
        },
        [&world_state]() {
            // 饥饿度未满时更新
            return world_state.GetHunger().GetCurrentHunger() < world_state.GetHunger().GetMaxHunger();
        },
        2);

    // BuffSystem - Buff系统
    coordinator.RegisterSystem(
        LoopPhase::Time,
        kBuffSystem,
        [&world_state](float delta_seconds) {
            world_state.MutableBuffs().Tick(delta_seconds);
        },
        [&world_state]() {
            // 有活跃buff时更新
            return !world_state.GetBuffs().GetActiveBuffs().empty();
        },
        3);
}

// ============================================================================
// 【RegisterInputSystems_】注册输入系统
// ============================================================================
void SystemLoopRegistry::RegisterInputSystems_(
    GameLoopCoordinator& coordinator,
    GameWorldState& world_state) {

    // PlayerMovementSystem - 玩家移动系统
    coordinator.RegisterSystem(
        LoopPhase::Input,
        kPlayerMovement,
        [&world_state](float delta_seconds) {
            // 注意：实际移动由 GameRuntime 处理
            // 此处仅做边界检测和状态更新
        },
        0);
}

// ============================================================================
// 【RegisterWorldSystems_】注册世界系统
// ============================================================================
void SystemLoopRegistry::RegisterWorldSystems_(
    GameLoopCoordinator& coordinator,
    GameWorldState& world_state,
    GameWorldSystems& systems) {

    // CropGrowthSystem - 作物生长系统
    coordinator.RegisterSystem(
        LoopPhase::World,
        kCropGrowth,
        [&world_state, &systems](float delta_seconds) {
            // 注意：实际更新在 GameRuntime::UpdateCropGrowth 中
            // 此委托用于框架集成
        },
        [&world_state]() {
            // 有未成熟的作物时更新
            for (const auto& plot : world_state.GetTeaPlots()) {
                if (plot.cleared && plot.seeded && !plot.ready) return true;
            }
            return false;
        },
        0);

    // WorkshopSystem - 工坊系统
    coordinator.RegisterSystem(
        LoopPhase::World,
        kWorkshop,
        [&world_state, &systems](float delta_seconds) {
            // 注意：实际更新在 GameRuntime::UpdateWorkshop 中
        },
        [&world_state, &systems]() {
            // 有进行中的加工时更新
            const auto& machines = systems.GetWorkshop().GetMachines();
            for (const auto& [id, machine] : machines) {
                if (machine.active && machine.progress < 1.0f) return true;
            }
            return false;
        },
        1);

    // NpcScheduleSystem - NPC日程系统
    coordinator.RegisterSystem(
        LoopPhase::World,
        kNpcSchedule,
        [&world_state, &systems](float delta_seconds) {
            // 注意：实际更新在 GameRuntime::UpdateNpcs 中
        },
        2);

    // NpcDialogueManager - NPC对话管理器
    coordinator.RegisterSystem(
        LoopPhase::World,
        kNpcDialogue,
        [&world_state](float delta_seconds) {
            // 对话系统被动更新
            world_state.MutableInteraction().dialogue_engine.Update(delta_seconds);
        },
        [&world_state]() {
            // 有活跃对话时更新
            return world_state.GetInteraction().dialogue_engine.IsActive();
        },
        3);

    // SpiritBeastSystem - 灵兽系统
    coordinator.RegisterSystem(
        LoopPhase::World,
        kSpiritBeast,
        [&world_state](float delta_seconds) {
            // 灵兽状态被动更新
        },
        [&world_state]() {
            return world_state.GetSpiritBeast().exists;
        },
        4);

    // PickupSystemRuntime - 可拾取物系统
    coordinator.RegisterSystem(
        LoopPhase::World,
        kPickupSystem,
        [&world_state](float delta_seconds) {
            // 拾取物自动消失计时
            // 注意：实际收集在 GameRuntime 中处理
        },
        [&world_state]() {
            return !world_state.GetPickups().empty();
        },
        5);

    // QuestManager - 任务管理器
    coordinator.RegisterSystem(
        LoopPhase::World,
        kQuestManager,
        [](float delta_seconds) {
            // 任务系统被动更新
        },
        6);

    // AchievementSystem - 成就系统
    coordinator.RegisterSystem(
        LoopPhase::World,
        kAchievementSystem,
        [](float delta_seconds) {
            // 成就检查
        },
        7);

    // TeaGarden - 茶园系统
    coordinator.RegisterSystem(
        LoopPhase::World,
        kTeaGarden,
        [&world_state](float delta_seconds) {
            // 茶园状态更新
        },
        8);
}

// ============================================================================
// 【RegisterCombatSystems_】注册战斗系统
// ============================================================================
void SystemLoopRegistry::RegisterCombatSystems_(
    GameLoopCoordinator& coordinator,
    GameWorldState& world_state,
    GameRuntime& runtime) {

    // BattleManager - 战斗管理器
    coordinator.RegisterSystem(
        LoopPhase::Combat,
        kBattleManager,
        [&world_state, &runtime](float delta_seconds) {
            // 注意：战斗更新在 GameRuntime::Update 中独立处理
            // 战斗管理器在战斗模式下由 GameRuntime 直接调用
        },
        [&world_state]() {
            return world_state.GetInSpiritRealm();
        },
        0);
}

// ============================================================================
// 【RegisterRuntimeSystems_】注册运行时系统
// ============================================================================
void SystemLoopRegistry::RegisterRuntimeSystems_(
    GameLoopCoordinator& coordinator,
    GameWorldState& world_state,
    GameWorldSystems& systems,
    GameRuntime& runtime) {

    // TutorialSystem - 教程系统
    coordinator.RegisterSystem(
        LoopPhase::Runtime,
        kTutorialSystem,
        [&world_state](float delta_seconds) {
            // 教程状态更新
        },
        [&world_state]() {
            // 新手教程未完成时更新
            return world_state.GetTutorial().tutorial_bubble_step <= 11;
        },
        0);

    // MainPlotSystem - 主线剧情系统
    coordinator.RegisterSystem(
        LoopPhase::Runtime,
        kMainPlotSystem,
        [&systems](float delta_seconds) {
            // 主线剧情被动更新
            systems.GetNpcDevelopment().Update(delta_seconds);
        },
        1);

    // NpcDeliverySystem - NPC委托系统
    coordinator.RegisterSystem(
        LoopPhase::Runtime,
        kNpcDeliverySystem,
        [&world_state, &systems](float delta_seconds) {
            // NPC委托检查（每日刷新在 OnDayChanged 中处理）
        },
        2);

    // CloudSystem - 云海系统
    coordinator.RegisterSystem(
        LoopPhase::Runtime,
        kCloudSystem,
        [&world_state, &systems](float delta_seconds) {
            systems.GetCloud().UpdateForecastVisibility(
                world_state.MutableClock().Day(),
                world_state.MutableClock().Hour());
        },
        3);

    // SceneTransition - 场景切换
    coordinator.RegisterSystem(
        LoopPhase::Runtime,
        kSceneTransition,
        [&world_state](float delta_seconds) {
            // 场景过渡动画（由 GameRuntime 更新）
        },
        [&world_state]() {
            return false; // 由 GameRuntime 显式控制
        },
        4);

    // SpiritBeastDispatch - 灵兽派遣
    coordinator.RegisterSystem(
        LoopPhase::Runtime,
        kSpiritBeastDispatch,
        [&world_state](float delta_seconds) {
            // 派遣倒计时更新（由 GameRuntime 更新）
        },
        [&world_state]() {
            return world_state.GetSpiritBeast().dispatch_active;
        },
        5);
}

// ============================================================================
// 【RegisterParticleSystems_】注册粒子系统
// ============================================================================
void SystemLoopRegistry::RegisterParticleSystems_(GameLoopCoordinator& coordinator) {
    // ParticleSystem - 粒子系统
    coordinator.RegisterSystem(
        LoopPhase::Particles,
        kParticleSystem,
        [](float delta_seconds) {
            // 粒子效果更新
        },
        0);
}

} // namespace CloudSeamanor::engine
