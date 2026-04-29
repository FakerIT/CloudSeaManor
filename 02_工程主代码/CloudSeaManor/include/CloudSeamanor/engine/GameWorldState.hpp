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

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/CloudGuardianContract.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/engine/Interactable.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/engine/PickupDrop.hpp"
#include "CloudSeamanor/engine/DialogueEngine.hpp"
#include "CloudSeamanor/domain/Player.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/FestivalSystem.hpp"
#include "CloudSeamanor/engine/FestivalRuntimeData.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"
#include "CloudSeamanor/domain/DynamicLifeSystem.hpp"
#include "CloudSeamanor/domain/RelationshipSystem.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"
#include "CloudSeamanor/domain/HungerSystem.hpp"
#include "CloudSeamanor/domain/BuffSystem.hpp"
#include "CloudSeamanor/domain/TeaBush.hpp"
#include "CloudSeamanor/engine/rendering/SceneVisualStore.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <algorithm>
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

    // 灵兽互动菜单（BE-077）：G 键打开，数字键选择，Enter/G 确认。
    bool spirit_beast_menu_open = false;
    int spirit_beast_menu_selection = 0; // 0=喂食 1=抚摸 2=派遣

    // 宠物购买菜单（BE-079）：E 打开，数字键选择，Enter/E 确认。
    bool pet_shop_menu_open = false;
    int pet_shop_menu_selection = 0; // 0=猫 1=狗 2=鸟（按可售列表映射）

    // 收购商出售菜单（P4-ECON-001）：E 打开，数字键选择，Enter/E 确认出售。
    bool purchaser_menu_open = false;
    int purchaser_menu_selection = 0;
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
// 【SocialState】社交/关系终局状态（告白/婚礼/婚后）
// ============================================================================
struct SocialState {
    CloudSeamanor::domain::RelationshipState relationship;
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
    void SyncSceneVisuals();

    // ========================================================================
    // 【状态访问器】
    // ========================================================================
    // 场景对象
    [[nodiscard]] const CloudSeamanor::domain::Player& GetPlayer() const { return player_; }
    [[nodiscard]] CloudSeamanor::domain::Player& MutablePlayer() { return player_; }
    [[nodiscard]] const std::vector<TeaPlot>& GetTeaPlots() const { return tea_plots_; }
    [[nodiscard]] std::vector<TeaPlot>& MutableTeaPlots() { return tea_plots_; }
    [[nodiscard]] const std::vector<TeaPlot>& GetTeaGardenPlots() const { return tea_garden_plots_; }
    [[nodiscard]] std::vector<TeaPlot>& MutableTeaGardenPlots() { return tea_garden_plots_; }
    [[nodiscard]] const std::vector<CloudSeamanor::domain::TeaBush>& GetTeaBushes() const { return tea_bushes_; }
    [[nodiscard]] std::vector<CloudSeamanor::domain::TeaBush>& MutableTeaBushes() { return tea_bushes_; }
    [[nodiscard]] const std::vector<NpcActor>& GetNpcs() const { return npcs_; }
    [[nodiscard]] std::vector<NpcActor>& MutableNpcs() { return npcs_; }
    [[nodiscard]] const SpiritBeast& GetSpiritBeast() const { return spirit_beast_; }
    [[nodiscard]] SpiritBeast& MutableSpiritBeast() { return spirit_beast_; }
    [[nodiscard]] const std::vector<CloudSeamanor::domain::PickupDrop>& GetPickups() const { return pickups_; }
    [[nodiscard]] std::vector<CloudSeamanor::domain::PickupDrop>& MutablePickups() { return pickups_; }
    [[nodiscard]] const std::vector<CloudSeamanor::domain::Interactable>& GetInteractables() const { return interactables_; }
    [[nodiscard]] std::vector<CloudSeamanor::domain::Interactable>& MutableInteractables() { return interactables_; }
    [[nodiscard]] const std::vector<sf::FloatRect>& GetObstacleBounds() const { return obstacle_bounds_; }
    [[nodiscard]] std::vector<sf::FloatRect>& MutableObstacleBounds() { return obstacle_bounds_; }
    [[nodiscard]] const std::vector<sf::RectangleShape>& GetGroundTiles() const { return ground_tiles_; }
    [[nodiscard]] std::vector<sf::RectangleShape>& MutableGroundTiles() { return ground_tiles_; }
    [[nodiscard]] const std::vector<sf::RectangleShape>& GetObstacleShapes() const { return obstacle_shapes_; }
    [[nodiscard]] std::vector<sf::RectangleShape>& MutableObstacleShapes() { return obstacle_shapes_; }
    [[nodiscard]] const SceneVisualStore& GetSceneVisuals() const { return scene_visuals_; }
    [[nodiscard]] SceneVisualStore& MutableSceneVisuals() { return scene_visuals_; }

    // 时钟
    [[nodiscard]] const CloudSeamanor::domain::GameClock& GetClock() const { return clock_; }
    [[nodiscard]] CloudSeamanor::domain::GameClock& MutableClock() { return clock_; }

    // 背包与体力
    [[nodiscard]] const CloudSeamanor::domain::Inventory& GetInventory() const { return inventory_; }
    [[nodiscard]] CloudSeamanor::domain::Inventory& MutableInventory() { return inventory_; }
    [[nodiscard]] const CloudSeamanor::domain::StaminaSystem& GetStamina() const { return stamina_; }
    [[nodiscard]] CloudSeamanor::domain::StaminaSystem& MutableStamina() { return stamina_; }
    [[nodiscard]] const CloudSeamanor::domain::HungerSystem& GetHunger() const { return hunger_; }
    [[nodiscard]] CloudSeamanor::domain::HungerSystem& MutableHunger() { return hunger_; }
    [[nodiscard]] const CloudSeamanor::domain::BuffSystem& GetBuffs() const { return buffs_; }
    [[nodiscard]] CloudSeamanor::domain::BuffSystem& MutableBuffs() { return buffs_; }

    // 项目状态
    [[nodiscard]] const RepairProject& GetMainHouseRepair() const { return main_house_repair_; }
    [[nodiscard]] RepairProject& MutableMainHouseRepair() { return main_house_repair_; }
    [[nodiscard]] const TeaMachine& GetTeaMachine() const { return tea_machine_; }
    [[nodiscard]] TeaMachine& MutableTeaMachine() { return tea_machine_; }

    // UI
    [[nodiscard]] const UiPanels& GetPanels() const { return panels_; }
    [[nodiscard]] UiPanels& MutablePanels() { return panels_; }
    [[nodiscard]] const UiTexts& GetTexts() const { return texts_; }
    [[nodiscard]] UiTexts& MutableTexts() { return texts_; }

    // 交互状态
    [[nodiscard]] const InteractionState& GetInteraction() const { return interaction_; }
    [[nodiscard]] InteractionState& MutableInteraction() { return interaction_; }
    [[nodiscard]] const NpcTextMappings& GetNpcTextMappings() const { return npc_text_mappings_; }
    [[nodiscard]] NpcTextMappings& MutableNpcTextMappings() { return npc_text_mappings_; }
    [[nodiscard]] CloudSeamanor::domain::CloudState GetLastCloudState() const { return last_cloud_state_; }
    [[nodiscard]] CloudSeamanor::domain::CloudState& MutableLastCloudState() { return last_cloud_state_; }
    void SetLastCloudState(CloudSeamanor::domain::CloudState value) { last_cloud_state_ = value; }

    // 教程
    [[nodiscard]] const TutorialState& GetTutorial() const { return tutorial_; }
    [[nodiscard]] TutorialState& MutableTutorial() { return tutorial_; }

    // 社交/关系终局
    [[nodiscard]] const SocialState& GetSocial() const { return social_; }
    [[nodiscard]] SocialState& MutableSocial() { return social_; }

    // 粒子
    [[nodiscard]] const std::vector<HeartParticle>& GetHeartParticles() const { return heart_particles_; }
    [[nodiscard]] std::vector<HeartParticle>& MutableHeartParticles() { return heart_particles_; }

    // 运行时状态
    [[nodiscard]] float GetSessionTime() const { return session_time_; }
    [[nodiscard]] float& MutableSessionTime() { return session_time_; }
    void SetSessionTime(float value) { session_time_ = value; }
    
    [[nodiscard]] bool GetSpiritBeastWateredToday() const { return spirit_beast_watered_today_; }
    [[nodiscard]] bool& MutableSpiritBeastWateredToday() { return spirit_beast_watered_today_; }
    void SetSpiritBeastWateredToday(bool value) { spirit_beast_watered_today_ = value; }
    
    [[nodiscard]] bool GetLevelUpOverlayActive() const { return level_up_overlay_active_; }
    [[nodiscard]] bool& MutableLevelUpOverlayActive() { return level_up_overlay_active_; }
    void SetLevelUpOverlayActive(bool value) { level_up_overlay_active_ = value; }
    
    [[nodiscard]] float GetLevelUpOverlayTimer() const { return level_up_overlay_timer_; }
    [[nodiscard]] float& MutableLevelUpOverlayTimer() { return level_up_overlay_timer_; }
    void SetLevelUpOverlayTimer(float value) { level_up_overlay_timer_ = value; }
    
    [[nodiscard]] CloudSeamanor::domain::SkillType GetLevelUpSkillType() const { return level_up_skill_type_; }
    [[nodiscard]] CloudSeamanor::domain::SkillType& MutableLevelUpSkillType() { return level_up_skill_type_; }
    void SetLevelUpSkillType(CloudSeamanor::domain::SkillType value) { level_up_skill_type_ = value; }
    
    [[nodiscard]] bool GetLowStaminaWarningActive() const { return low_stamina_warning_active_; }
    [[nodiscard]] bool& MutableLowStaminaWarningActive() { return low_stamina_warning_active_; }
    void SetLowStaminaWarningActive(bool value) { low_stamina_warning_active_ = value; }
    
    [[nodiscard]] float GetWorldTipPulse() const { return world_tip_pulse_; }
    [[nodiscard]] float& MutableWorldTipPulse() { return world_tip_pulse_; }
    void SetWorldTipPulse(float value) { world_tip_pulse_ = value; }
    
    [[nodiscard]] const std::string& GetFestivalNoticeText() const { return festival_notice_text_; }
    [[nodiscard]] std::string& MutableFestivalNoticeText() { return festival_notice_text_; }
    void SetFestivalNoticeText(const std::string& value) { festival_notice_text_ = value; }
    [[nodiscard]] const std::string& GetActiveFestivalId() const { return active_festival_id_; }
    [[nodiscard]] std::string& MutableActiveFestivalId() { return active_festival_id_; }
    void SetActiveFestivalId(const std::string& value) { active_festival_id_ = value; }

    [[nodiscard]] const FestivalRuntimeData& GetFestivalRuntime() const { return festival_runtime_; }
    [[nodiscard]] FestivalRuntimeData& MutableFestivalRuntime() { return festival_runtime_; }

    [[nodiscard]] bool GetFontLoaded() const { return font_loaded_; }
    [[nodiscard]] bool& MutableFontLoaded() { return font_loaded_; }
    void SetFontLoaded(bool value) { font_loaded_ = value; }

    [[nodiscard]] int GetGold() const { return gold_; }
    [[nodiscard]] int& MutableGold() { return gold_; }
    void SetGold(int value) { gold_ = value; }
    [[nodiscard]] const std::vector<PriceTableEntry>& GetPriceTable() const { return price_table_; }
    [[nodiscard]] std::vector<PriceTableEntry>& MutablePriceTable() { return price_table_; }
    [[nodiscard]] const std::vector<MailOrderEntry>& GetMailOrders() const { return mail_orders_; }
    [[nodiscard]] std::vector<MailOrderEntry>& MutableMailOrders() { return mail_orders_; }
    [[nodiscard]] const std::vector<InnOrderEntry>& GetInnOrders() const { return inn_orders_; }
    [[nodiscard]] std::vector<InnOrderEntry>& MutableInnOrders() { return inn_orders_; }
    [[nodiscard]] CloudSeamanor::domain::CropQuality GetLastTradeQuality() const { return last_trade_quality_; }
    [[nodiscard]] CloudSeamanor::domain::CropQuality& MutableLastTradeQuality() { return last_trade_quality_; }
    void SetLastTradeQuality(CloudSeamanor::domain::CropQuality q) { last_trade_quality_ = q; }
    [[nodiscard]] bool GetInSpiritRealm() const { return in_spirit_realm_; }
    [[nodiscard]] bool& MutableInSpiritRealm() { return in_spirit_realm_; }
    void SetInSpiritRealm(bool value) { in_spirit_realm_ = value; }
    [[nodiscard]] int GetSpiritRealmDailyMax() const { return spirit_realm_daily_max_; }
    [[nodiscard]] int& MutableSpiritRealmDailyMax() { return spirit_realm_daily_max_; }
    void SetSpiritRealmDailyMax(int value) { spirit_realm_daily_max_ = std::max(1, value); }
    [[nodiscard]] int GetSpiritRealmDailyRemaining() const { return spirit_realm_daily_remaining_; }
    [[nodiscard]] int& MutableSpiritRealmDailyRemaining() { return spirit_realm_daily_remaining_; }
    void SetSpiritRealmDailyRemaining(int value) {
        spirit_realm_daily_remaining_ = std::clamp(value, 0, spirit_realm_daily_max_);
    }
    [[nodiscard]] std::unordered_map<std::string, int>& MutableSpiritPlantLastHarvestHour() {
        return spirit_plant_last_harvest_hour_;
    }
    [[nodiscard]] const std::unordered_map<std::string, int>& GetSpiritPlantLastHarvestHour() const {
        return spirit_plant_last_harvest_hour_;
    }
    [[nodiscard]] std::vector<RuntimeQuest>& MutableRuntimeQuests() { return runtime_quests_; }
    [[nodiscard]] const std::vector<RuntimeQuest>& GetRuntimeQuests() const { return runtime_quests_; }
    [[nodiscard]] std::unordered_map<std::string, int>& MutableWeeklyBuyCount() { return weekly_buy_count_; }
    [[nodiscard]] std::unordered_map<std::string, int>& MutableWeeklySellCount() { return weekly_sell_count_; }
    [[nodiscard]] const std::unordered_map<std::string, int>& GetWeeklyBuyCount() const { return weekly_buy_count_; }
    [[nodiscard]] const std::unordered_map<std::string, int>& GetWeeklySellCount() const { return weekly_sell_count_; }
    [[nodiscard]] const std::vector<std::string>& GetDailyGeneralStoreStock() const { return daily_general_store_stock_; }
    [[nodiscard]] std::vector<std::string>& MutableDailyGeneralStoreStock() { return daily_general_store_stock_; }
    [[nodiscard]] int GetInnGoldReserve() const { return inn_gold_reserve_; }
    [[nodiscard]] int& MutableInnGoldReserve() { return inn_gold_reserve_; }
    [[nodiscard]] int GetInnVisitorsToday() const { return inn_visitors_today_; }
    [[nodiscard]] int& MutableInnVisitorsToday() { return inn_visitors_today_; }
    [[nodiscard]] int GetInnIncomeToday() const { return inn_income_today_; }
    [[nodiscard]] int& MutableInnIncomeToday() { return inn_income_today_; }
    [[nodiscard]] int GetInnReputation() const { return inn_reputation_; }
    [[nodiscard]] int& MutableInnReputation() { return inn_reputation_; }
    [[nodiscard]] int GetCoopFedToday() const { return coop_fed_today_; }
    [[nodiscard]] int& MutableCoopFedToday() { return coop_fed_today_; }
    [[nodiscard]] int GetLivestockEggsToday() const { return livestock_eggs_today_; }
    [[nodiscard]] int& MutableLivestockEggsToday() { return livestock_eggs_today_; }
    [[nodiscard]] int GetLivestockMilkToday() const { return livestock_milk_today_; }
    [[nodiscard]] int& MutableLivestockMilkToday() { return livestock_milk_today_; }
    [[nodiscard]] int GetDecorationScore() const { return decoration_score_; }
    [[nodiscard]] int& MutableDecorationScore() { return decoration_score_; }
    [[nodiscard]] const std::vector<std::string>& GetWeeklyReports() const { return weekly_reports_; }
    [[nodiscard]] std::vector<std::string>& MutableWeeklyReports() { return weekly_reports_; }

    // ========== P2-001 日记 ==========
    [[nodiscard]] const std::vector<DiaryEntryState>& GetDiaryEntries() const { return diary_entries_; }
    [[nodiscard]] std::vector<DiaryEntryState>& MutableDiaryEntries() { return diary_entries_; }

    // ========== P0-002 配方/食谱解锁（轻量） ==========
    [[nodiscard]] const std::unordered_map<std::string, bool>& GetRecipeUnlocks() const { return recipe_unlocks_; }
    [[nodiscard]] std::unordered_map<std::string, bool>& MutableRecipeUnlocks() { return recipe_unlocks_; }

    // ========== P2-002 技能分支 ==========
    [[nodiscard]] const std::unordered_map<std::string, std::string>& GetSkillBranches() const { return skill_branches_; }
    [[nodiscard]] std::unordered_map<std::string, std::string>& MutableSkillBranches() { return skill_branches_; }
    [[nodiscard]] const std::vector<std::string>& GetPendingSkillBranches() const { return pending_skill_branches_; }
    [[nodiscard]] std::vector<std::string>& MutablePendingSkillBranches() { return pending_skill_branches_; }

    // ========== P3-002 DIY 摆放 ==========
    [[nodiscard]] const std::vector<PlacedObject>& GetPlacedObjects() const { return placed_objects_; }
    [[nodiscard]] std::vector<PlacedObject>& MutablePlacedObjects() { return placed_objects_; }

    // ========== P0-003 净化回流 ==========
    [[nodiscard]] int GetPurifyReturnDays() const { return purify_return_days_; }
    [[nodiscard]] int& MutablePurifyReturnDays() { return purify_return_days_; }
    [[nodiscard]] int GetPurifyReturnSpirits() const { return purify_return_spirits_; }
    [[nodiscard]] int& MutablePurifyReturnSpirits() { return purify_return_spirits_; }
    [[nodiscard]] int GetFishingAttempts() const { return fishing_attempts_; }
    [[nodiscard]] int& MutableFishingAttempts() { return fishing_attempts_; }
    [[nodiscard]] const std::string& GetLastFishCatch() const { return last_fish_catch_; }
    [[nodiscard]] std::string& MutableLastFishCatch() { return last_fish_catch_; }
    [[nodiscard]] const std::string& GetPetType() const { return pet_type_; }
    [[nodiscard]] std::string& MutablePetType() { return pet_type_; }
    [[nodiscard]] bool GetPetAdopted() const { return pet_adopted_; }
    [[nodiscard]] bool& MutablePetAdopted() { return pet_adopted_; }
    [[nodiscard]] const std::unordered_map<std::string, bool>& GetAchievements() const { return achievements_; }
    [[nodiscard]] std::unordered_map<std::string, bool>& MutableAchievements() { return achievements_; }
    [[nodiscard]] const std::vector<std::string>& GetModHooks() const { return mod_hooks_; }
    [[nodiscard]] std::vector<std::string>& MutableModHooks() { return mod_hooks_; }
    [[nodiscard]] bool GetGreenhouseUnlocked() const { return greenhouse_unlocked_; }
    [[nodiscard]] bool& MutableGreenhouseUnlocked() { return greenhouse_unlocked_; }
    void SetGreenhouseUnlocked(bool value) { greenhouse_unlocked_ = value; }
    [[nodiscard]] bool GetGreenhouseTagNextPlanting() const { return greenhouse_tag_next_planting_; }
    [[nodiscard]] bool& MutableGreenhouseTagNextPlanting() { return greenhouse_tag_next_planting_; }

    // ========== 战斗系统状态 ==========
    [[nodiscard]] bool GetInBattleMode() const { return in_battle_mode_; }
    [[nodiscard]] bool& MutableInBattleMode() { return in_battle_mode_; }
    void SetInBattleMode(bool value) { in_battle_mode_ = value; }

    [[nodiscard]] bool GetBattleAvailable() const { return battle_available_; }
    [[nodiscard]] bool& MutableBattleAvailable() { return battle_available_; }
    void SetBattleAvailable(bool value) { battle_available_ = value; }

    [[nodiscard]] const std::vector<std::string>& GetBattleActivePartners() const { return battle_active_partners_; }
    [[nodiscard]] std::vector<std::string>& MutableBattleActivePartners() { return battle_active_partners_; }

    // 配置
    [[nodiscard]] const WorldConfig& GetConfig() const { return config_; }
    [[nodiscard]] WorldConfig& MutableConfig() { return config_; }

private:
    // 时钟
    CloudSeamanor::domain::GameClock clock_;

    // 场景对象
    CloudSeamanor::domain::Player player_;
    std::vector<TeaPlot> tea_plots_;
    std::vector<TeaPlot> tea_garden_plots_;
    std::vector<CloudSeamanor::domain::TeaBush> tea_bushes_;
    std::vector<NpcActor> npcs_;
    SpiritBeast spirit_beast_;
    std::vector<CloudSeamanor::domain::PickupDrop> pickups_;
    std::vector<CloudSeamanor::domain::Interactable> interactables_;
    std::vector<sf::FloatRect> obstacle_bounds_;
    std::vector<sf::RectangleShape> ground_tiles_;
    std::vector<sf::RectangleShape> obstacle_shapes_;
    SceneVisualStore scene_visuals_;

    // 背包与体力
    CloudSeamanor::domain::Inventory inventory_;
    CloudSeamanor::domain::StaminaSystem stamina_;
    CloudSeamanor::domain::HungerSystem hunger_;
    CloudSeamanor::domain::BuffSystem buffs_;

    // 项目状态
    RepairProject main_house_repair_;
    TeaMachine tea_machine_;

    // UI
    UiPanels panels_;
    UiTexts texts_;

    // 交互状态
    InteractionState interaction_;
    NpcTextMappings npc_text_mappings_;
    CloudSeamanor::domain::CloudState last_cloud_state_ = CloudSeamanor::domain::CloudState::Mist;

    // 教程
    TutorialState tutorial_;
    SocialState social_;

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
    std::string active_festival_id_;
    FestivalRuntimeData festival_runtime_;
    bool font_loaded_ = false;
    int gold_ = 500;
    std::vector<PriceTableEntry> price_table_;
    std::vector<MailOrderEntry> mail_orders_;
    std::vector<InnOrderEntry> inn_orders_;
    CloudSeamanor::domain::CropQuality last_trade_quality_ =
        CloudSeamanor::domain::CropQuality::Normal;
    bool in_spirit_realm_ = false;
    int spirit_realm_daily_max_ = 5;
    int spirit_realm_daily_remaining_ = 5;
    std::unordered_map<std::string, int> spirit_plant_last_harvest_hour_;
    std::vector<RuntimeQuest> runtime_quests_;
    std::unordered_map<std::string, int> weekly_buy_count_;
    std::unordered_map<std::string, int> weekly_sell_count_;
    std::vector<std::string> daily_general_store_stock_;
    int inn_gold_reserve_ = 0;
    int inn_visitors_today_ = 0;
    int inn_income_today_ = 0;
    int inn_reputation_ = 0;
    int coop_fed_today_ = 0;
    int livestock_eggs_today_ = 0;
    int livestock_milk_today_ = 0;
    int decoration_score_ = 0;
    std::vector<std::string> weekly_reports_;
    std::vector<DiaryEntryState> diary_entries_;
    std::unordered_map<std::string, bool> recipe_unlocks_;
    std::unordered_map<std::string, std::string> skill_branches_;
    std::vector<std::string> pending_skill_branches_;
    std::vector<PlacedObject> placed_objects_;
    int purify_return_days_ = 0;
    int purify_return_spirits_ = 0;
    int fishing_attempts_ = 0;
    std::string last_fish_catch_;
    std::string pet_type_;
    bool pet_adopted_ = false;
    std::unordered_map<std::string, bool> achievements_;
    std::vector<std::string> mod_hooks_;
    bool greenhouse_unlocked_ = false;
    bool greenhouse_tag_next_planting_ = false;

    // 战斗系统状态
    bool in_battle_mode_ = false;
    bool battle_available_ = true;
    std::vector<std::string> battle_active_partners_;

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
