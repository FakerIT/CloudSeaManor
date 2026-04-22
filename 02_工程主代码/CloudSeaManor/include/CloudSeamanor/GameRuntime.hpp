#pragma once

// ============================================================================
// 【GameRuntime】游戏运行时协调器
// ============================================================================
// 协调所有游戏系统和状态，提供统一的运行时管理接口。
//
// 主要职责：
// - 管理所有系统和状态的引用
// - 提供统一的更新接口
// - 处理游戏主循环中的协调逻辑
// - 管理教程引导状态
//
// 设计原则：
// - 使用依赖注入，所有系统和状态通过构造函数传入
// - 提供简洁的更新接口供 GameApp 调用
// - 封装复杂的跨系统协调逻辑
// ============================================================================

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/GameWorldSystems.hpp"
#include "CloudSeamanor/FarmingSystem.hpp"
#include "CloudSeamanor/PlayerInteractRuntime.hpp"
#include "CloudSeamanor/PickupSystem.hpp"
#include "CloudSeamanor/GameConfig.hpp"
#include "CloudSeamanor/TmxMap.hpp"
#include "CloudSeamanor/LevelUpSystem.hpp"

#include "CloudSeamanor/engine/systems/PlayerMovementSystem.hpp"
#include "CloudSeamanor/engine/systems/PickupSystemRuntime.hpp"
#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"
#include "CloudSeamanor/engine/systems/AchievementSystem.hpp"
#include "CloudSeamanor/engine/systems/NpcScheduleSystem.hpp"
#include "CloudSeamanor/engine/systems/QuestManager.hpp"
#include "CloudSeamanor/engine/systems/NpcDeliverySystem.hpp"
#include "CloudSeamanor/engine/systems/InnSystem.hpp"
#include "CloudSeamanor/engine/systems/CoopSystem.hpp"
#include "CloudSeamanor/engine/systems/PetSystem.hpp"
#include "CloudSeamanor/engine/systems/SpiritBeastSystem.hpp"
#include "CloudSeamanor/engine/systems/SpiritRealmManager.hpp"
#include "CloudSeamanor/engine/systems/TutorialSystem.hpp"
#include "CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp"
#include "CloudSeamanor/MainPlotSystem.hpp"
#include "CloudSeamanor/NpcDialogueManager.hpp"
#include "CloudSeamanor/SaveSlotManager.hpp"
#include "CloudSeamanor/SceneManager.hpp"
#include "CloudSeamanor/TeaGarden.hpp"
#include "CloudSeamanor/ModApi.hpp"
#include "CloudSeamanor/engine/BattleManager.hpp"

using CloudSeamanor::infrastructure::GameConfig;
using CloudSeamanor::infrastructure::TmxMap;

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【GameRuntimeCallbacks】运行时回调函数
// ============================================================================
struct GameRuntimeCallbacks {
    std::function<void(const std::string&, float)> push_hint;
    std::function<void(const std::string&)> push_notification;
    std::function<void(const std::string&)> log_info;
    std::function<void(const std::string&)> play_sfx;
    std::function<void(const std::string& bgm_path, bool loop, float fade_in, float fade_out)> play_bgm;
    std::function<void()> update_hud_text;
    std::function<void()> refresh_window_title;
};

// ============================================================================
// 【SleepResult】睡眠结算结果
// ============================================================================
struct SleepResult {
    std::string message;
    std::string festival_notice;
    bool has_festival = false;
    int spirit_gain = 0;
    int contract_progress = 0;
};

// ============================================================================
// 【GameRuntime】游戏运行时协调器类
// ============================================================================
class GameRuntime {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    GameRuntime();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize(
        const std::string& config_path,
        const std::string& schedule_path,
        const std::string& gift_path,
        const std::string& npc_text_path,
        const std::string& map_path,
        const GameRuntimeCallbacks& callbacks
    );

    // ========================================================================
    // 【回调】
    // ========================================================================
    [[nodiscard]] const GameRuntimeCallbacks& Callbacks() const { return callbacks_; }
    [[nodiscard]] GameRuntimeCallbacks& Callbacks() { return callbacks_; }

    // ========================================================================
    // 【交互处理】
    // ========================================================================
    void HandleGiftInteraction();
    void HandlePrimaryInteraction();

    // ========================================================================
    // 【窗口标题】
    // ========================================================================
    void RefreshWindowTitle();

    // ========================================================================
    // 【系统访问】
    // ========================================================================
    [[nodiscard]] GameWorldState& WorldState() { return world_state_; }
    [[nodiscard]] const GameWorldState& WorldState() const { return world_state_; }
    [[nodiscard]] GameWorldSystems& Systems() { return systems_; }
    [[nodiscard]] const GameWorldSystems& Systems() const { return systems_; }
    [[nodiscard]] FarmingSystem& Farming() { return farming_; }
    [[nodiscard]] const FarmingSystem& Farming() const { return farming_; }
    [[nodiscard]] PickupSystem& Pickups() { return pickups_; }
    [[nodiscard]] const PickupSystem& Pickups() const { return pickups_; }
    [[nodiscard]] NpcDialogueManager& DialogueManager() { return dialogue_manager_; }
    [[nodiscard]] const NpcDialogueManager& DialogueManager() const { return dialogue_manager_; }
    [[nodiscard]] MainPlotSystem& PlotSystem() { return plot_system_; }
    [[nodiscard]] const MainPlotSystem& PlotSystem() const { return plot_system_; }

    // ========================================================================
    // 【窗口】
    // ========================================================================
    [[nodiscard]] sf::RenderWindow* Window() { return window_; }
    void SetWindow(sf::RenderWindow* window);

    // ========================================================================
    // 【配置访问】
    // ========================================================================
    [[nodiscard]] const GameConfig& Config() const { return config_; }
    [[nodiscard]] float TimeScale() const { return time_scale_; }
    [[nodiscard]] float CloudMultiplier() const;

    // ========================================================================
    // 【帧更新】
    // ========================================================================
    void OnDayChanged();
    void OnPlayerMoved(float delta_seconds, const sf::Vector2f& direction);
    void OnPlayerInteracted(const CloudSeamanor::domain::Interactable& target);
    void Update(float delta_seconds);
    void RenderSceneTransition(sf::RenderWindow& window);
    void RenderBattle(sf::RenderWindow& window);
    SleepResult SleepToNextMorning();
    [[nodiscard]] bool IsInBattleMode() const { return in_battle_mode_; }
    bool HandleBattleKey(int skill_slot);
    void ToggleBattlePause();
    void RetreatBattle();
    bool TryEnterBattleByPlayerPosition();
    void CycleSpiritBeastName();

    // ========================================================================
    // 【教程】
    // ========================================================================
    void CheckTutorialHints();

    // ========================================================================
    // 【存档】
    // ========================================================================
    bool SaveGame();
    bool LoadGame();
    bool SaveGameToSlot(int slot_index);
    bool LoadGameFromSlot(int slot_index);
    void SetActiveSaveSlot(int slot_index);
    [[nodiscard]] int ActiveSaveSlot() const { return active_save_slot_; }
    [[nodiscard]] std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata> ReadSaveSlots() const;
    [[nodiscard]] const std::filesystem::path& SavePath() const { return save_path_; }

    // ========================================================================
    // 【技能升级】
    // ========================================================================
    [[nodiscard]] LevelUpEvent ConsumeLevelUpEvent();

    // ========================================================================
    // 【状态查询】
    // ========================================================================
    [[nodiscard]] bool CanSleep() const;
    [[nodiscard]] std::string GetCurrentTargetText() const;
    [[nodiscard]] std::string GetControlsHint() const;
    [[nodiscard]] std::size_t GetEntityCount() const;
    [[nodiscard]] std::size_t GetPickupCount() const;

private:
    void SyncTeaMachineFromWorkshop_();
    void UpdatePlayerMovement(float delta_seconds, float dx, float dy);
    void UpdateCropGrowth(float delta_seconds);
    void UpdateWorkshop(float delta_seconds);
    void UpdateNpcs(float delta_seconds);
    void UpdateSpiritBeast(float delta_seconds);
    void UpdateParticles(float delta_seconds);
    void UpdateHighlightedInteractable();
    void UpdateUi(float delta_seconds);
    PlayerInteractRuntimeContext BuildInteractionContext_();
    void HandleInteractionCommon_(bool is_gift_interaction);
    void RequestSpiritRealmTravel_(bool to_spirit_realm);

    GameWorldState world_state_;
    GameWorldSystems systems_;
    FarmingSystem farming_;
    PickupSystem pickups_;
    GameConfig config_;
    TmxMap tmx_map_;

    PlayerMovementSystem player_movement_;
    CropGrowthSystem crop_growth_;
    AchievementSystem achievement_system_;
    NpcScheduleSystem npc_schedule_;
    QuestManager quest_manager_;
    NpcDeliverySystem npc_delivery_;
    InnSystem inn_system_;
    CoopSystem coop_system_;
    BarnSystem barn_system_;
    PetSystem pet_system_;
    SpiritBeastSystem spirit_beast_;
    SpiritRealmManager spirit_realm_manager_;
    CloudSeamanor::domain::TeaGarden tea_garden_;
    std::unique_ptr<TutorialSystem> tutorial_system_;
    std::unique_ptr<PickupSystemRuntime> pickup_runtime_;
    std::unique_ptr<WorkshopSystemRuntime> workshop_runtime_;
    NpcDialogueManager dialogue_manager_{"assets/data"};
    MainPlotSystem plot_system_{};
    ModLoader mod_loader_{};
    BattleManager battle_manager_{};

    sf::RenderWindow* window_ = nullptr;
    GameRuntimeCallbacks callbacks_;

    CloudSeamanor::domain::CloudState last_cloud_state_ = CloudSeamanor::domain::CloudState::Mist;
    float time_scale_ = 1.0f;
    std::filesystem::path save_path_ = std::filesystem::path("saves") / "save_slot_01.txt";
    int active_save_slot_ = 1;
    CloudSeamanor::infrastructure::SaveSlotManager save_slot_manager_;
    int last_reset_day_ = -1;

    NpcTextMappings npc_text_mappings_;
    int last_contract_completed_count_ = 0;
    std::string current_bgm_path_;

    SceneTransition scene_transition_;
    bool in_battle_mode_ = false;
    std::string dialogue_data_root_ = "assets/data";
    std::string map_root_override_;
};

} // namespace CloudSeamanor::engine
