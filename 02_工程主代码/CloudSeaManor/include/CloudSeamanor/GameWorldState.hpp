#pragma once

// ============================================================================
// 【GameWorldState】游戏世界状态
// ============================================================================
// 统一管理游戏中所有运行时状态，包括场景对象、UI 状态和交互状态。
//
// 主要职责：
// - 管理场景对象（地块、NPC、灵兽、可拾取物）
// - 管理交互状态（高亮索引、提示消息）
// - 管理 UI 面板状态
// - 提供状态访问器和修改器
//
// 设计原则：
// - 所有状态集中管理，便于存档和调试
// - 使用结构体分组相关状态
// - 避免裸指针，使用引用和智能指针
// ============================================================================

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/CloudGuardianContract.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/PickupDrop.hpp"
#include "CloudSeamanor/DialogueEngine.hpp"
#include "CloudSeamanor/Player.hpp"
#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/game_state/RuntimeState.hpp"

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【WorldConfig】世界配置参数
// ============================================================================
struct WorldConfig {
    float player_speed = 240.0f;
    float stamina_move_per_second = 4.0f;
    float stamina_interact_cost = 8.0f;
    float stamina_recover_per_second = 2.5f;
    sf::FloatRect world_bounds{{40.0f, 40.0f}, {1200.0f, 640.0f}};
};

enum class QuestState : std::uint8_t {
    NotTaken,
    InProgress,
    Completed,
    Claimed
};

struct RuntimeQuest {
    std::string id;
    std::string title;
    std::string description;
    std::string objective;
    std::string reward;
    bool auto_accept = true;  // NPC 委托默认自动接取，契约任务默认手动接取
    QuestState state = QuestState::NotTaken;
};

// ============================================================================
// 【UiPanels】UI 面板集合
// ============================================================================
struct UiPanels {
    sf::RectangleShape main_panel;
    sf::RectangleShape inventory_panel;
    sf::RectangleShape dialogue_panel;
    sf::RectangleShape hint_panel;
    sf::RectangleShape stamina_bar_bg;
    sf::RectangleShape stamina_bar_fill;
    sf::RectangleShape workshop_progress_bg;
    sf::RectangleShape workshop_progress_fill;
    sf::RectangleShape aura_overlay;
    sf::RectangleShape festival_notice_panel;
};

// ============================================================================
// 【UiTexts】UI 文本对象集合
// ============================================================================
struct UiTexts {
    std::unique_ptr<sf::Text> hud_text;
    std::unique_ptr<sf::Text> inventory_text;
    std::unique_ptr<sf::Text> hint_text;
    std::unique_ptr<sf::Text> dialogue_text;
    std::unique_ptr<sf::Text> debug_text;
    std::unique_ptr<sf::Text> world_tip_text;
    std::unique_ptr<sf::Text> festival_notice_text;
    std::unique_ptr<sf::Text> level_up_text;

    UiTexts() = default;
};

// ============================================================================
// 【InteractionState】交互状态
// ============================================================================
struct InteractionState {
    int highlighted_index = -1;
    int highlighted_plot_index = -1;
    int highlighted_npc_index = -1;
    bool spirit_beast_highlighted = false;
    std::string dialogue_text = "暂时还没有对话。";
    std::string hint_message = "欢迎回到云海山庄。";
    float hint_timer = 0.0f;
    std::vector<DialogueNode> dialogue_nodes;
    std::string dialogue_start_id;

    DialogueEngine dialogue_engine;

    // 心事件追踪（对话完成后用于结算奖励）
    std::string current_heart_event_id;
    int current_heart_event_reward = 0;
    std::string current_heart_event_flag;
};

// ============================================================================
// 【TutorialState】教程引导状态
// ============================================================================
struct TutorialState {
    bool intro_move_hint_shown = false;
    bool intro_interact_hint_shown = false;
    bool intro_crop_hint_shown = false;
    bool intro_save_hint_shown = false;
    bool show_debug_overlay = true;
    int daily_cloud_report_day_shown = -1;
    std::uint16_t tutorial_bubble_completed_mask = 0;
    int tutorial_bubble_step = 1;  // 1..11
};

// ============================================================================
// 【GameWorldState】游戏世界状态类
// ============================================================================
class GameWorldState {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    GameWorldState();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void InitializePanels();
    void InitializeTexts(const sf::Font& font);
    void InitializeWorld(const WorldConfig& config);

    // ========================================================================
    // 【状态访问器】
    // ========================================================================
    // 场景对象
    [[nodiscard]] CloudSeamanor::domain::Player& GetPlayer() { return player_; }
    [[nodiscard]] const CloudSeamanor::domain::Player& GetPlayer() const { return player_; }
    [[nodiscard]] std::vector<TeaPlot>& GetTeaPlots() { return tea_plots_; }
    [[nodiscard]] const std::vector<TeaPlot>& GetTeaPlots() const { return tea_plots_; }
    [[nodiscard]] std::vector<TeaPlot>& GetTeaGardenPlots() { return tea_garden_plots_; }
    [[nodiscard]] const std::vector<TeaPlot>& GetTeaGardenPlots() const { return tea_garden_plots_; }
    [[nodiscard]] std::vector<NpcActor>& GetNpcs() { return npcs_; }
    [[nodiscard]] const std::vector<NpcActor>& GetNpcs() const { return npcs_; }
    [[nodiscard]] SpiritBeast& GetSpiritBeast() { return spirit_beast_; }
    [[nodiscard]] const SpiritBeast& GetSpiritBeast() const { return spirit_beast_; }
    [[nodiscard]] std::vector<CloudSeamanor::domain::PickupDrop>& GetPickups() { return pickups_; }
    [[nodiscard]] const std::vector<CloudSeamanor::domain::PickupDrop>& GetPickups() const { return pickups_; }
    [[nodiscard]] std::vector<CloudSeamanor::domain::Interactable>& GetInteractables() { return interactables_; }
    [[nodiscard]] const std::vector<CloudSeamanor::domain::Interactable>& GetInteractables() const { return interactables_; }
    [[nodiscard]] std::vector<sf::FloatRect>& GetObstacleBounds() { return obstacle_bounds_; }
    [[nodiscard]] const std::vector<sf::FloatRect>& GetObstacleBounds() const { return obstacle_bounds_; }
    [[nodiscard]] std::vector<sf::RectangleShape>& GetGroundTiles() { return ground_tiles_; }
    [[nodiscard]] const std::vector<sf::RectangleShape>& GetGroundTiles() const { return ground_tiles_; }
    [[nodiscard]] std::vector<sf::RectangleShape>& GetObstacleShapes() { return obstacle_shapes_; }
    [[nodiscard]] const std::vector<sf::RectangleShape>& GetObstacleShapes() const { return obstacle_shapes_; }

    // 时钟
    [[nodiscard]] CloudSeamanor::domain::GameClock& GetClock() { return clock_; }
    [[nodiscard]] const CloudSeamanor::domain::GameClock& GetClock() const { return clock_; }

    // 背包与体力
    [[nodiscard]] CloudSeamanor::domain::Inventory& GetInventory() { return inventory_; }
    [[nodiscard]] const CloudSeamanor::domain::Inventory& GetInventory() const { return inventory_; }
    [[nodiscard]] CloudSeamanor::domain::StaminaSystem& GetStamina() { return stamina_; }
    [[nodiscard]] const CloudSeamanor::domain::StaminaSystem& GetStamina() const { return stamina_; }

    // 项目状态
    [[nodiscard]] RepairProject& GetMainHouseRepair() { return main_house_repair_; }
    [[nodiscard]] const RepairProject& GetMainHouseRepair() const { return main_house_repair_; }
    [[nodiscard]] TeaMachine& GetTeaMachine() { return tea_machine_; }
    [[nodiscard]] const TeaMachine& GetTeaMachine() const { return tea_machine_; }

    // UI
    [[nodiscard]] UiPanels& GetPanels() { return panels_; }
    [[nodiscard]] const UiPanels& GetPanels() const { return panels_; }
    [[nodiscard]] UiTexts& GetTexts() { return texts_; }
    [[nodiscard]] const UiTexts& GetTexts() const { return texts_; }

    // 交互状态
    [[nodiscard]] InteractionState& GetInteraction() { return interaction_; }
    [[nodiscard]] const InteractionState& GetInteraction() const { return interaction_; }
    [[nodiscard]] NpcTextMappings& GetNpcTextMappings() { return npc_text_mappings_; }
    [[nodiscard]] const NpcTextMappings& GetNpcTextMappings() const { return npc_text_mappings_; }

    // 教程
    [[nodiscard]] TutorialState& GetTutorial() { return tutorial_; }
    [[nodiscard]] const TutorialState& GetTutorial() const { return tutorial_; }

    // 粒子
    [[nodiscard]] std::vector<HeartParticle>& GetHeartParticles() { return heart_particles_; }
    [[nodiscard]] const std::vector<HeartParticle>& GetHeartParticles() const { return heart_particles_; }

    // 运行时状态
    [[nodiscard]] float& GetSessionTime() { return session_time_; }
    [[nodiscard]] float GetSessionTime() const { return session_time_; }
    void SetSessionTime(float value) { session_time_ = value; }
    
    [[nodiscard]] bool& GetSpiritBeastWateredToday() { return spirit_beast_watered_today_; }
    [[nodiscard]] bool GetSpiritBeastWateredToday() const { return spirit_beast_watered_today_; }
    void SetSpiritBeastWateredToday(bool value) { spirit_beast_watered_today_ = value; }
    
    [[nodiscard]] bool& GetLevelUpOverlayActive() { return level_up_overlay_active_; }
    [[nodiscard]] bool GetLevelUpOverlayActive() const { return level_up_overlay_active_; }
    void SetLevelUpOverlayActive(bool value) { level_up_overlay_active_ = value; }
    
    [[nodiscard]] float& GetLevelUpOverlayTimer() { return level_up_overlay_timer_; }
    [[nodiscard]] float GetLevelUpOverlayTimer() const { return level_up_overlay_timer_; }
    void SetLevelUpOverlayTimer(float value) { level_up_overlay_timer_ = value; }
    
    [[nodiscard]] CloudSeamanor::domain::SkillType& GetLevelUpSkillType() { return level_up_skill_type_; }
    [[nodiscard]] CloudSeamanor::domain::SkillType GetLevelUpSkillType() const { return level_up_skill_type_; }
    void SetLevelUpSkillType(CloudSeamanor::domain::SkillType value) { level_up_skill_type_ = value; }
    
    [[nodiscard]] bool& GetLowStaminaWarningActive() { return low_stamina_warning_active_; }
    [[nodiscard]] bool GetLowStaminaWarningActive() const { return low_stamina_warning_active_; }
    void SetLowStaminaWarningActive(bool value) { low_stamina_warning_active_ = value; }
    
    [[nodiscard]] float& GetWorldTipPulse() { return world_tip_pulse_; }
    [[nodiscard]] float GetWorldTipPulse() const { return world_tip_pulse_; }
    void SetWorldTipPulse(float value) { world_tip_pulse_ = value; }
    
    [[nodiscard]] std::string& GetFestivalNoticeText() { return festival_notice_text_; }
    [[nodiscard]] const std::string& GetFestivalNoticeText() const { return festival_notice_text_; }
    void SetFestivalNoticeText(const std::string& value) { festival_notice_text_ = value; }

    [[nodiscard]] CloudSeamanor::game_state::RuntimeState& GetRuntimeState() { return runtime_state_; }
    [[nodiscard]] const CloudSeamanor::game_state::RuntimeState& GetRuntimeState() const { return runtime_state_; }
    
    [[nodiscard]] bool& GetFontLoaded() { return font_loaded_; }
    [[nodiscard]] bool GetFontLoaded() const { return font_loaded_; }
    void SetFontLoaded(bool value) { font_loaded_ = value; }

    [[nodiscard]] int& GetGold() { return gold_; }
    [[nodiscard]] int GetGold() const { return gold_; }
    void SetGold(int value) { gold_ = value; }
    [[nodiscard]] std::vector<PriceTableEntry>& GetPriceTable() { return price_table_; }
    [[nodiscard]] const std::vector<PriceTableEntry>& GetPriceTable() const { return price_table_; }
    [[nodiscard]] std::vector<MailOrderEntry>& GetMailOrders() { return mail_orders_; }
    [[nodiscard]] const std::vector<MailOrderEntry>& GetMailOrders() const { return mail_orders_; }
    [[nodiscard]] CloudSeamanor::domain::CropQuality& GetLastTradeQuality() { return last_trade_quality_; }
    [[nodiscard]] CloudSeamanor::domain::CropQuality GetLastTradeQuality() const { return last_trade_quality_; }
    void SetLastTradeQuality(CloudSeamanor::domain::CropQuality q) { last_trade_quality_ = q; }
    [[nodiscard]] bool& GetInSpiritRealm() { return in_spirit_realm_; }
    [[nodiscard]] bool GetInSpiritRealm() const { return in_spirit_realm_; }
    void SetInSpiritRealm(bool value) { in_spirit_realm_ = value; }
    [[nodiscard]] std::unordered_map<std::string, int>& GetSpiritPlantLastHarvestHour() {
        return spirit_plant_last_harvest_hour_;
    }
    [[nodiscard]] const std::unordered_map<std::string, int>& GetSpiritPlantLastHarvestHour() const {
        return spirit_plant_last_harvest_hour_;
    }
    [[nodiscard]] std::vector<RuntimeQuest>& GetRuntimeQuests() { return runtime_quests_; }
    [[nodiscard]] const std::vector<RuntimeQuest>& GetRuntimeQuests() const { return runtime_quests_; }
    [[nodiscard]] std::unordered_map<std::string, int>& GetWeeklyBuyCount() { return weekly_buy_count_; }
    [[nodiscard]] std::unordered_map<std::string, int>& GetWeeklySellCount() { return weekly_sell_count_; }
    [[nodiscard]] const std::vector<std::string>& GetDailyGeneralStoreStock() const { return daily_general_store_stock_; }
    [[nodiscard]] std::vector<std::string>& GetDailyGeneralStoreStock() { return daily_general_store_stock_; }
    [[nodiscard]] int& GetInnGoldReserve() { return inn_gold_reserve_; }
    [[nodiscard]] int& GetCoopFedToday() { return coop_fed_today_; }
    [[nodiscard]] int& GetDecorationScore() { return decoration_score_; }
    [[nodiscard]] std::string& GetPetType() { return pet_type_; }
    [[nodiscard]] bool& GetPetAdopted() { return pet_adopted_; }
    [[nodiscard]] std::unordered_map<std::string, bool>& GetAchievements() { return achievements_; }
    [[nodiscard]] std::vector<std::string>& GetModHooks() { return mod_hooks_; }
    [[nodiscard]] bool& GetGreenhouseUnlocked() { return greenhouse_unlocked_; }
    [[nodiscard]] bool GetGreenhouseUnlocked() const { return greenhouse_unlocked_; }
    [[nodiscard]] bool& GetGreenhouseTagNextPlanting() { return greenhouse_tag_next_planting_; }
    [[nodiscard]] bool GetGreenhouseTagNextPlanting() const { return greenhouse_tag_next_planting_; }

    // 配置
    [[nodiscard]] WorldConfig& GetConfig() { return config_; }
    [[nodiscard]] const WorldConfig& GetConfig() const { return config_; }

private:
    // 时钟
    CloudSeamanor::domain::GameClock clock_;

    // 场景对象
    CloudSeamanor::domain::Player player_;
    std::vector<TeaPlot> tea_plots_;
    std::vector<TeaPlot> tea_garden_plots_;
    std::vector<NpcActor> npcs_;
    SpiritBeast spirit_beast_;
    std::vector<CloudSeamanor::domain::PickupDrop> pickups_;
    std::vector<CloudSeamanor::domain::Interactable> interactables_;
    std::vector<sf::FloatRect> obstacle_bounds_;
    std::vector<sf::RectangleShape> ground_tiles_;
    std::vector<sf::RectangleShape> obstacle_shapes_;

    // 背包与体力
    CloudSeamanor::domain::Inventory inventory_;
    CloudSeamanor::domain::StaminaSystem stamina_;

    // 项目状态
    RepairProject main_house_repair_;
    TeaMachine tea_machine_;

    // UI
    UiPanels panels_;
    UiTexts texts_;

    // 交互状态
    InteractionState interaction_;
    NpcTextMappings npc_text_mappings_;

    // 教程
    TutorialState tutorial_;

    // 粒子
    std::vector<HeartParticle> heart_particles_;

    // 运行时状态
    float session_time_ = 0.0f;
    bool spirit_beast_watered_today_ = false;
    bool level_up_overlay_active_ = false;
    float level_up_overlay_timer_ = 0.0f;
    CloudSeamanor::domain::SkillType level_up_skill_type_ = CloudSeamanor::domain::SkillType::SpiritFarm;
    bool low_stamina_warning_active_ = false;
    float world_tip_pulse_ = 0.0f;
    std::string festival_notice_text_;
    CloudSeamanor::game_state::RuntimeState runtime_state_{};
    bool font_loaded_ = false;
    int gold_ = 500;
    std::vector<PriceTableEntry> price_table_;
    std::vector<MailOrderEntry> mail_orders_;
    CloudSeamanor::domain::CropQuality last_trade_quality_ =
        CloudSeamanor::domain::CropQuality::Normal;
    bool in_spirit_realm_ = false;
    std::unordered_map<std::string, int> spirit_plant_last_harvest_hour_;
    std::vector<RuntimeQuest> runtime_quests_;
    std::unordered_map<std::string, int> weekly_buy_count_;
    std::unordered_map<std::string, int> weekly_sell_count_;
    std::vector<std::string> daily_general_store_stock_;
    int inn_gold_reserve_ = 0;
    int coop_fed_today_ = 0;
    int decoration_score_ = 0;
    std::string pet_type_;
    bool pet_adopted_ = false;
    std::unordered_map<std::string, bool> achievements_;
    std::vector<std::string> mod_hooks_;
    bool greenhouse_unlocked_ = false;
    bool greenhouse_tag_next_planting_ = false;

    // 配置
    WorldConfig config_;
};

// ============================================================================
// 【SetHintMessage】设置提示消息
// ============================================================================
void SetHintMessage(GameWorldState& state, const std::string& message, float duration = 2.6f);

// ============================================================================
// 【UpdateStaminaBar】更新体力条显示
// ============================================================================
void UpdateStaminaBar(GameWorldState& state);

// ============================================================================
// 【UpdateWorkshopProgressBar】更新工坊进度条显示
// ============================================================================
void UpdateWorkshopProgressBar(GameWorldState& state, const CloudSeamanor::domain::WorkshopSystem& workshop);

// ============================================================================
// 【UpdateWorldTipPulse】更新世界提示脉冲
// ============================================================================
void UpdateWorldTipPulse(GameWorldState& state, float delta_seconds);

// ============================================================================
// 【ResetDailyInteractionState】重置每日交互状态
// ============================================================================
void ResetDailyInteractionState(GameWorldState& state, int current_day);

// ============================================================================
// 【ResetPlotsWateredState】重置地块浇水状态
// ============================================================================
void ResetPlotsWateredState(GameWorldState& state,
                             const std::function<void(TeaPlot&, bool)>& refresh_visual);

} // namespace CloudSeamanor::engine
