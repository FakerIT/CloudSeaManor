#pragma once

// ============================================================================
// 【GameRuntime】游戏运行时协调器
// ============================================================================
// Design refs:
// - docs/ENGINEERING_STANDARDS.md
// - docs/TASKS.md (P9 NPC 动态发展、P10 工程治理)
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

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/GameWorldSystems.hpp"
#include "CloudSeamanor/engine/PlayerInteractRuntime.hpp"
#include "CloudSeamanor/infrastructure/GameConfig.hpp"
#include "CloudSeamanor/infrastructure/TmxMap.hpp"
#include "CloudSeamanor/engine/LevelUpSystem.hpp"
#include "CloudSeamanor/infrastructure/DataRegistry.hpp"
#include "CloudSeamanor/infrastructure/ResourceManager.hpp"

#include "CloudSeamanor/engine/systems/PlayerMovementSystem.hpp"
#include "CloudSeamanor/engine/systems/PickupSystemRuntime.hpp"
#include "CloudSeamanor/engine/systems/CropGrowthSystem.hpp"
#include "CloudSeamanor/engine/systems/AchievementSystem.hpp"
#include "CloudSeamanor/engine/systems/AchievementRewardLoader.hpp"
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
#include "CloudSeamanor/engine/MainPlotSystem.hpp"
#include "CloudSeamanor/engine/NpcDialogueManager.hpp"
#include "CloudSeamanor/infrastructure/SaveSlotManager.hpp"
#include "CloudSeamanor/engine/SceneManager.hpp"
#include "CloudSeamanor/engine/TeaGarden.hpp"
#include "CloudSeamanor/ModApi.hpp"
#include "CloudSeamanor/engine/BattleManager.hpp"
#include "CloudSeamanor/domain/RelationshipSystem.hpp"
#include "CloudSeamanor/engine/GameLoopCoordinator.hpp"
#include "CloudSeamanor/engine/LoopDebugPanel.hpp"

// ============================================================================
// 新系统头文件
// ============================================================================
#include "CloudSeamanor/FishingSystem.hpp"
#include "CloudSeamanor/MysticMirrorSystem.hpp"
#include "CloudSeamanor/ContractSystem.hpp"

using CloudSeamanor::infrastructure::GameConfig;
using CloudSeamanor::infrastructure::TmxMap;

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    [[nodiscard]] PickupSystem& Pickups() { return systems_.MutablePickups(); }
    [[nodiscard]] const PickupSystem& Pickups() const { return systems_.GetPickups(); }
    [[nodiscard]] NpcScheduleSystem& NpcSchedule() { return services_.npc_schedule; }
    [[nodiscard]] const NpcScheduleSystem& NpcSchedule() const { return services_.npc_schedule; }
    [[nodiscard]] NpcDialogueManager& DialogueManager() { return services_.dialogue_manager; }
    [[nodiscard]] const NpcDialogueManager& DialogueManager() const { return services_.dialogue_manager; }
    [[nodiscard]] MainPlotSystem& PlotSystem() { return services_.plot_system; }
    [[nodiscard]] const MainPlotSystem& PlotSystem() const { return services_.plot_system; }

    // ========================================================================
    // 【窗口】
    // ========================================================================
    [[nodiscard]] sf::RenderWindow* Window() { return window_; }
    void SetWindow(sf::RenderWindow* window);
    void SetResourceManager(CloudSeamanor::infrastructure::ResourceManager* resource_manager) {
        resource_manager_ = resource_manager;
    }

    // ========================================================================
    // 【配置访问】
    // ========================================================================
    [[nodiscard]] const GameConfig& Config() const { return state_.config; }
    [[nodiscard]] float TimeScale() const { return state_.time_scale; }
    [[nodiscard]] float CloudMultiplier() const;

    // ========================================================================
    // 【帧更新】
    // ========================================================================
    void OnDayChanged();
    void OnPlayerMoved(float delta_seconds, const sf::Vector2f& direction);
    void OnPlayerInteracted(const CloudSeamanor::domain::Interactable& target);
    void Update(float delta_seconds);
    void UpdateWithLoopCoordinator(float delta_seconds);  // 使用循环协调器（调试用）
    void RenderSceneTransition(sf::RenderWindow& window);
    void RenderBattle(sf::RenderWindow& window);
    SleepResult SleepToNextMorning();
    [[nodiscard]] bool IsInBattleMode() const { return state_.in_battle_mode; }
    bool HandleBattleKey(int skill_slot);
    void ToggleBattlePause();
    void RetreatBattle();
    bool TryEnterBattleByPlayerPosition();
    void RestoreWorldBGM_();
    void CycleSpiritBeastName();
    void CycleTrackingContractVolume(int delta);
    void CollectArrivedMail();
    void ToggleSpiritBeastDispatch();
    [[nodiscard]] bool HasPendingSkillBranchChoice() const;
    [[nodiscard]] std::string PendingSkillBranchSkill() const;
    void CommitPendingSkillBranch(bool choose_a);
    [[nodiscard]] bool IsFishingQteActive() const;
    [[nodiscard]] float FishingQteProgress() const;
    [[nodiscard]] float FishingQteTargetCenter() const;
    [[nodiscard]] float FishingQteTargetWidth() const;
    [[nodiscard]] const std::string& FishingQteLabel() const;
    void StartFishingQte(const std::string& source_label);
    void ResolveFishingQte();
    [[nodiscard]] bool IsDiyPlacementActive() const;
    [[nodiscard]] int DiyCursorX() const;
    [[nodiscard]] int DiyCursorY() const;
    [[nodiscard]] int DiyRotation() const;
    [[nodiscard]] std::string DiyPreviewObjectId() const;
    void MoveDiyCursor(int dx, int dy);
    void RotateDiyPreview();
    void ConfirmDiyPlacement();
    void PickupLastDiyObject();
    [[nodiscard]] int SpiritBeastDispatchRemainingSeconds() const {
        return static_cast<int>(std::max(0.0f, state_.spirit_beast_dispatch_remaining_seconds));
    }

    // ========================================================================
    // 【循环协调器开关】
    // ========================================================================
    void EnableLoopCoordinatorProfiling(bool enable);
    [[nodiscard]] std::string GetLoopStatsReport() const;

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
    bool DeleteSaveSlot(int slot_index);
    bool CopySaveSlot(int from_slot_index, int to_slot_index, bool overwrite_target);
    bool RenameSaveSlot(int slot_index, const std::string& display_name);
    void SetActiveSaveSlot(int slot_index);
    [[nodiscard]] int ActiveSaveSlot() const { return state_.active_save_slot; }
    [[nodiscard]] std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata> ReadSaveSlots() const;
    [[nodiscard]] const std::filesystem::path& SavePath() const { return state_.save_path; }

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

    // ========================================================================
    // 【循环协调器访问】
    // ========================================================================
    [[nodiscard]] GameLoopCoordinator& LoopCoordinator() { return loop_coordinator_; }
    [[nodiscard]] const GameLoopCoordinator& LoopCoordinator() const { return loop_coordinator_; }
    void InitializeLoopCoordinator();
    void InitializeLoopDebugPanel(const sf::Font& font);
    void ToggleLoopDebugPanel();
    void UpdateLoopDebugPanel();
    void RenderLoopDebugPanel(sf::RenderWindow& window);

private:
    struct RuntimeModules {
        std::unique_ptr<TutorialSystem> tutorial;
        std::unique_ptr<PickupSystemRuntime> pickup;
        std::unique_ptr<WorkshopSystemRuntime> workshop;
    };

    struct RuntimeState {
        GameConfig config{};
        float time_scale = 1.0f;
        std::filesystem::path save_path = std::filesystem::path("saves") / "save_slot_01.txt";
        int active_save_slot = 1;
        int last_reset_day = -1;
        int last_contract_completed_count = 0;
        std::string current_bgm_path{};
        std::string current_ambient_path{};
        bool in_battle_mode = false;
        bool battle_available = true;
        float battle_trigger_cooldown_seconds = 0.0f;
        std::vector<std::string> battle_active_partners{"spirit_beast_main"};
        std::vector<std::string> current_battle_spirit_ids{};
        std::vector<std::string> active_polluted_spirit_ids{"spirit_wisp"};
        std::unordered_map<std::string, float> spirit_retreat_timers{};
        float spirit_refresh_timer = 0.0f;
        float spirit_beast_dispatch_remaining_seconds = 0.0f;
        std::string spirit_beast_dispatch_reward_item_id = "spirit_dust";
        int spirit_beast_dispatch_reward_count = 1;
        std::vector<MailTriggerRule> mail_trigger_rules{};
        std::unordered_map<std::string, int> mail_rule_last_trigger_day{};
        std::unordered_map<std::string, int> mail_rule_last_trigger_season_key{};
        std::unordered_map<std::string, bool> mail_rule_triggered_once{};
        std::unordered_set<std::string> npc_relationship_event_triggered_keys{};
        std::string dialogue_data_root = "assets/data";
        std::string map_root_override{};
        bool festival_event_subscribed = false;
        std::size_t festival_event_subscription_id = 0;
        bool tide_festival_battle_pending = false;
        bool tide_festival_battle_active = false;
        int tide_festival_battle_day = -1;
        int deferred_story_due_to_festival_day = -1;
        int festival_quick_flow_hint_day = -1;
        std::string equipped_weapon_id = "weapon_cloud_blade";
        std::unordered_set<std::string> unlocked_quest_skills{};
        float quest_wind_step_remaining_seconds = 0.0f;
        int qi_deficit_days = 0;
        bool battle_exit_by_retreat = false;
        // R7-003/R28-001/R28-002: retention telemetry and weekly summary accumulators.
        int retention_weekly_inn_income = 0;
        int retention_weekly_eggs = 0;
        int retention_weekly_milk = 0;
        int retention_weekly_workshop_ready_sum = 0;
        int retention_weekly_battle_victories = 0;
        bool retention_midterm_goal_committed = false;
        bool fishing_qte_active = false;
        float fishing_qte_progress = 0.0f;
        float fishing_qte_velocity = 0.9f;
        float fishing_qte_target_center = 0.55f;
        float fishing_qte_target_width = 0.22f;
        std::string fishing_qte_label{};
    };

    struct RuntimeServices {
        TmxMap tmx_map{};

        PlayerMovementSystem player_movement{};
        CropGrowthSystem crop_growth{};
        AchievementSystem achievement_system{};
        AchievementRewardLoader achievement_rewards{};
        NpcScheduleSystem npc_schedule{};
        QuestManager quest_manager{};
        NpcDeliverySystem npc_delivery{};
        InnSystem inn_system{};
        CoopSystem coop_system{};
        BarnSystem barn_system{};
        PetSystem pet_system{};
        SpiritBeastSystem spirit_beast{};
        SpiritRealmManager spirit_realm_manager{};

        CloudSeamanor::domain::TeaGarden tea_garden{};
        RuntimeModules modules{};
        NpcDialogueManager dialogue_manager{"assets/data"};
        MainPlotSystem plot_system{};
        ModLoader mod_loader{};
        BattleManager battle_manager{};
    };

    // 领域层：关系终局状态机（告白/婚礼/婚后）
    CloudSeamanor::domain::RelationshipSystem relationship_system_{};

    void SyncTeaMachineFromWorkshop_();
    void RecoverStaminaScaled_(float amount);
    bool ConsumeStaminaScaled_(float amount);
    void InitializeTeaBushes_();
    void SyncTeaBushInteractables_();
    void SyncNpcHouseVariantInteractables_();
    void UpdateTeaBushes_();
    void SyncEquippedWeaponFromSaveAndInventory_();
    static std::string InferWeaponFromInventory_(const CloudSeamanor::domain::Inventory& inv);
    void SyncQuestSkills_();
    void ApplyQuestSkillExploreEffects_(float delta_seconds);
    [[nodiscard]] bool HasQuestSkill_(const std::string& id) const;
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
    [[nodiscard]] PlacedObject* FindActiveDiyPreview_();
    [[nodiscard]] const PlacedObject* FindActiveDiyPreview_() const;

    // P8 新系统数据加载
    void LoadEcologyData_();
    void LoadMemoryConfig_();
    void LoadTeaSpiritDexConfig_();

    // ========================================================================
    // 【循环阶段更新函数】
    // ========================================================================
    void UpdateTimePhase_(float delta_seconds);       // 阶段0: 时间更新
    void UpdateWorldPhase_(float delta_seconds);        // 阶段2: 世界更新
    void UpdateRuntimePhase_(float delta_seconds);      // 阶段4: 运行时更新
    void UpdateBattleMode_(float delta_seconds);        // 战斗模式更新
    void UpdateCloudReport_();                        // 云海日报
    void UpdateSpiritRealmAutoReturn_();               // 灵界自动返回
    void UpdateBattleTrigger_(float delta_seconds);    // 战斗触发检测
    void UpdateLevelUpOverlay_(float delta_seconds);  // 升级动画
    void UpdateTutorialSystem_(float delta_seconds);   // 教程系统
    void UpdateMainPlot_(float delta_seconds);         // 主线剧情
    void UpdateCloudStateChange_();                    // 云海状态变化
    void UpdateStaminaWarning_();                      // 体力警告

    GameWorldState world_state_;
    GameWorldSystems systems_;
    RuntimeState state_{};
    RuntimeServices services_{};

    sf::RenderWindow* window_ = nullptr;
    CloudSeamanor::infrastructure::ResourceManager* resource_manager_ = nullptr;
    GameRuntimeCallbacks callbacks_;
    CloudSeamanor::infrastructure::DataRegistry data_registry_{};

    CloudSeamanor::infrastructure::SaveSlotManager save_slot_manager_;

    SceneTransition scene_transition_;

    // 统一游戏循环协调器
    GameLoopCoordinator loop_coordinator_;

    // 循环性能调试面板
    std::unique_ptr<LoopDebugPanel> loop_debug_panel_;
};

} // namespace CloudSeamanor::engine
