#pragma once

// ============================================================================
// 【PixelGameHud】像素风格游戏 HUD 管理系统
// ============================================================================
// Responsibilities:
// - 持有并管理所有像素 UI 组件
// - 统一初始化、更新、渲染
// - 处理面板开关逻辑（Esc / I / F / M）
// - 渲染所有 UI 层（按 RenderLayer 顺序）
//
// 设计原则：
// - 不实现游戏逻辑
// - 所有组件通过独立方法更新
// - 支持键盘 + 鼠标双操作
// - 支持 Esc 关闭面板
// ============================================================================

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelDialogueBox.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"
#include "CloudSeamanor/engine/PixelInventoryGrid.hpp"
#include "CloudSeamanor/engine/PixelNotificationBanner.hpp"
#include "CloudSeamanor/engine/PixelTooltip.hpp"
#include "CloudSeamanor/engine/PixelCloudSeaForecastPanel.hpp"
#include "CloudSeamanor/engine/PixelPlayerStatusPanel.hpp"
#include "CloudSeamanor/engine/PixelTeaGardenPanel.hpp"
#include "CloudSeamanor/engine/PixelWorkshopPanel.hpp"
#include "CloudSeamanor/engine/PixelContractPanel.hpp"
#include "CloudSeamanor/engine/PixelNpcDetailPanel.hpp"
#include "CloudSeamanor/engine/PixelSpiritBeastPanel.hpp"
#include "CloudSeamanor/engine/PixelFestivalPanel.hpp"
#include "CloudSeamanor/engine/PixelSpiritRealmPanel.hpp"
#include "CloudSeamanor/engine/PixelBuildingPanel.hpp"
#include "CloudSeamanor/engine/PixelShopPanel.hpp"
#include "CloudSeamanor/engine/PixelMailPanel.hpp"
#include "CloudSeamanor/engine/PixelAchievementPanel.hpp"
#include "CloudSeamanor/engine/PixelBeastiaryPanel.hpp"
#include "CloudSeamanor/engine/PixelContextMenu.hpp"
#include "CloudSeamanor/engine/PixelMinimap.hpp"
#include "CloudSeamanor/engine/PixelNpcSchedulePanel.hpp"
#include "CloudSeamanor/engine/PixelSocialPanel.hpp"
#include "CloudSeamanor/engine/PixelProgressBar.hpp"
#include "CloudSeamanor/engine/PixelQuestMenu.hpp"
#include "CloudSeamanor/engine/PixelSettingsPanel.hpp"
#include "CloudSeamanor/engine/PixelToolbar.hpp"
#include "CloudSeamanor/engine/PixelTutorialBubble.hpp"
#include "CloudSeamanor/engine/PixelUiConfig.hpp"
#include "CloudSeamanor/infrastructure/UiLayoutConfig.hpp"
#include "CloudSeamanor/infrastructure/SaveSlotManager.hpp"
#include "CloudSeamanor/engine/DialogueEngine.hpp"
#include "CloudSeamanor/engine/PixelMinimap.hpp"

namespace CloudSeamanor::engine {
class ResourceManager;
}

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PanelState】面板开启状态
// ============================================================================
struct PanelState {
    bool inventory_open = false;
    bool quest_menu_open = false;
    bool map_open = false;
    bool dialogue_open = false;
    bool npc_schedule_open = false;
    bool cloud_forecast_open = false;
    bool player_status_open = false;
    bool social_open = false;
};

// ============================================================================
// 【PixelGameHud】像素 HUD 管理器
// ============================================================================
class PixelGameHud {
public:
    enum class UiEventType : std::uint8_t { Open, Close, Select, Hover, Error, Achievement };
    struct InventoryActionCallbacks {
        std::function<bool(const std::string&, const std::string&, int)> use_item;
        std::function<bool(const std::string&, const std::string&, int)> sell_item;
        std::function<bool(const std::string&, const std::string&, int)> gift_item;
        std::function<bool(const std::string&, const std::string&, int)> drop_item;
    };
    struct PanelActionCallbacks {
        std::function<void(int)> contract_cycle_tracking_volume;
        std::function<void()> mail_collect_arrived;
        std::function<void()> spirit_beast_toggle_dispatch;
    };
    // ========================================================================
    // 【初始化】
    // ========================================================================

    /**
     * @brief 初始化所有像素 UI 组件
     * @param font 字体引用
     */
    void Initialize(const sf::Font& font);
    void Initialize(const sf::Font& font, const CloudSeamanor::infrastructure::UiLayoutConfig* layout_config);
    void SetResourceManager(CloudSeamanor::infrastructure::ResourceManager* rm);
    void SetUiScale(float scale);
    void SetUiEventCallback(std::function<void(UiEventType)> cb) { on_ui_event_ = std::move(cb); }
    void SetInventoryActionCallbacks(InventoryActionCallbacks callbacks) { inventory_action_callbacks_ = std::move(callbacks); }
    void SetPanelActionCallbacks(PanelActionCallbacks callbacks) { panel_action_callbacks_ = std::move(callbacks); }

    /**
     * @brief 是否有有效字体
     */
    [[nodiscard]] bool HasFont() const { return font_renderer_ != nullptr && font_renderer_->IsLoaded(); }

    // ========================================================================
    // 【面板开关】
    // ========================================================================

    /**
     * @brief 切换背包面板
     */
    void ToggleInventory();

    /**
     * @brief 切换任务面板
     */
    void ToggleQuestMenu();

    /**
     * @brief 切换地图
     */
    void ToggleMap();
    void ToggleNpcSchedule();
    void ToggleSocial();
    void ToggleSettings();
    void ToggleCloudForecast();
    void TogglePlayerStatus();
    void ToggleTeaGarden();
    void ToggleWorkshop();
    void ToggleContract();
    void ToggleNpcDetail();
    void ToggleSpiritBeast();
    void ToggleFestival();
    void ToggleSpiritRealm();
    void ToggleBuilding();
    void ToggleShop();
    void ToggleMail();
    void ToggleAchievement();
    void ToggleBeastiary();
    void SettingsMoveSelection(int delta);
    void SettingsAdjustValue(int delta);
    void SetSettingsSlots(const std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata>& slots);
    [[nodiscard]] int SettingsSelectedSlot() const;
    [[nodiscard]] int SettingsSelectedRow() const;

    /**
     * @brief 关闭所有面板（Esc）
     */
    void CloseAllPanels();

    /**
     * @brief 获取当前面板状态
     */
    [[nodiscard]] const PanelState& GetPanelState() const { return panel_state_; }

    /**
     * @brief 是否有任何面板打开
     */
    [[nodiscard]] bool IsAnyPanelOpen() const;

    /**
     * @brief 是否有对话框打开（优先级最高）
     */
    [[nodiscard]] bool IsDialogueOpen() const { return panel_state_.dialogue_open; }
    [[nodiscard]] bool IsSettingsOpen() const { return settings_panel_.IsVisible(); }
    void DialogueMoveChoice(int delta);
    void DialogueConfirmChoice();

    // ========================================================================
    // 【更新】
    // ========================================================================

    /**
     * @brief 每帧更新
     * @param delta_seconds 帧时间
     */
    void Update(float delta_seconds, const DialogueEngine* dialogue_engine = nullptr);

    /**
     * @brief 更新右上角信息（时间/季节/天气）
     */
    void UpdateTopRightInfo(const std::string& time_text,
                           const std::string& season_text,
                           const std::string& weather_text,
                           bool has_new_quest);

    /**
     * @brief 更新体力条
     */
    void UpdateStaminaBar(float stamina_ratio, float current, float max_stamina);
    void UpdateHungerBar(float hunger_ratio, float current, float max_hunger);

    /**
     * @brief 更新金币
     */
    void UpdateCoins(int coin_amount);

    /**
     * @brief 更新工具栏数据
     */
    void UpdateToolbar(const std::array<ToolbarSlot, 12>& slots, int selected_slot);

    /**
     * @brief 更新对话框
     */
    void UpdateDialogue(const std::string& speaker_name,
                        const std::string& full_text,
                        const std::vector<DialogueChoice>& choices,
                        const sf::Texture* avatar);

    /**
     * @brief 设置对话框完成/选择回调（由 GameApp 注入）
     */
    void SetDialogueBoxCallbacks(PixelDialogueBox::OnCompleteCallback on_complete,
                                 PixelDialogueBox::OnChoiceCallback on_choice);

    /**
     * @brief 更新任务列表
     */
    void UpdateQuests(const std::vector<Quest>& quests);

    /**
     * @brief 更新地图标记
     */
    void UpdateMapMarkers(const std::vector<MapMarker>& markers);
    void UpdateCloudForecast(const CloudForecastViewData& data);
    void UpdatePlayerStatus(const PlayerStatusViewData& data);
    void UpdateTeaGardenPanel(const TeaGardenPanelViewData& data);
    void UpdateFestivalPanel(const FestivalPanelViewData& data);
    void UpdateShopPanel(const ShopPanelViewData& data);
    void UpdateMailPanel(const MailPanelViewData& data);
    void UpdateAchievementPanel(const AchievementPanelViewData& data);
    void UpdateSpiritBeastPanel(const SpiritBeastPanelViewData& data);
    void UpdateBuildingPanel(const BuildingPanelViewData& data);
    void UpdateContractPanel(const ContractPanelViewData& data);
    void UpdateNpcDetailPanel(const NpcDetailPanelViewData& data);
    void UpdateNpcSchedulePanel(const NpcSchedulePanelViewData& data);
    void UpdateSocialPanel(const SocialPanelViewData& data);
    void UpdateSpiritRealmPanel(const SpiritRealmPanelViewData& data);
    void UpdateBeastiaryPanel(const BeastiaryPanelViewData& data);
    void UpdateWorkshopPanel(const WorkshopPanelViewData& data);
    void PushNotification(const std::string& message);
    void UpdateSkillBranchOverlay(bool visible,
                                  const std::string& skill_name,
                                  const std::string& option_a,
                                  const std::string& option_b);
    void UpdateFishingQteOverlay(bool visible,
                                 float progress,
                                 float target_center,
                                 float target_width,
                                 const std::string& title);
    void UpdateDiyPlacementOverlay(bool visible,
                                   const std::string& object_name,
                                   int tile_x,
                                   int tile_y,
                                   int rotation);
    void UpdateDailyRecommendations(const std::vector<std::string>& items);
    void ConfigureNotificationTimings(float fade_in_seconds,
                                      float hold_seconds,
                                      float fade_out_seconds,
                                      float cloud_report_total_seconds);
    void UpdateTutorialBubble(int step_index,
                              const std::string& text,
                              const sf::FloatRect& highlight_rect,
                              bool visible);
    void SetBottomRightHotkeyHints(std::string interact_key, std::string tool_key);
    [[nodiscard]] std::optional<int> ConsumeTutorialStepDelta();

    // ========================================================================
    // 【事件处理】
    // ========================================================================

    /**
     * @brief 处理按键
     * @return true 如果已消费该按键
     */
    bool HandleKeyPressed(const sf::Event::KeyPressed& key);

    /**
     * @brief 处理鼠标移动
     */
    void HandleMouseMove(float mx, float my);

    /**
     * @brief 处理鼠标点击
     * @return true 如果点击落在 UI 区域上
     */
    bool HandleMouseClick(float mx, float my, sf::Mouse::Button button = sf::Mouse::Button::Left);

    /**
     * @brief 处理鼠标松开（用于标题栏拖动等）
     */
    void HandleMouseRelease(float mx, float my, sf::Mouse::Button button = sf::Mouse::Button::Left);

    /**
     * @brief 处理鼠标滚轮（用于滚动列表等）
     * @return true 如果滚轮作用在 UI 上
     */
    bool HandleMouseWheel(float mx, float my, float delta);

    // ========================================================================
    // 【渲染】
    // ========================================================================

    /**
     * @brief 渲染所有像素 UI 层
     */
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【组件访问器】
    // ========================================================================
    [[nodiscard]] const PixelToolbar& GetToolbar() const { return toolbar_; }
    [[nodiscard]] PixelToolbar& MutableToolbar() { return toolbar_; }
    [[nodiscard]] const PixelDialogueBox& GetDialogueBox() const { return dialogue_box_; }
    [[nodiscard]] PixelDialogueBox& MutableDialogueBox() { return dialogue_box_; }
    [[nodiscard]] const PixelInventoryGrid& GetInventory() const { return inventory_grid_; }
    [[nodiscard]] PixelInventoryGrid& MutableInventory() { return inventory_grid_; }
    [[nodiscard]] const PixelQuestMenu& GetQuestMenu() const { return quest_menu_; }
    [[nodiscard]] PixelQuestMenu& MutableQuestMenu() { return quest_menu_; }
    [[nodiscard]] const PixelMinimap& GetMinimap() const { return minimap_; }
    [[nodiscard]] PixelMinimap& MutableMinimap() { return minimap_; }
    [[nodiscard]] const PixelProgressBar& GetStaminaBar() const { return stamina_bar_; }
    [[nodiscard]] PixelProgressBar& MutableStaminaBar() { return stamina_bar_; }
    [[nodiscard]] const PixelSettingsPanel& GetSettingsPanel() const { return settings_panel_; }
    [[nodiscard]] PixelSettingsPanel& MutableSettingsPanel() { return settings_panel_; }
    [[nodiscard]] const PixelCloudSeaForecastPanel& GetCloudForecastPanel() const { return cloud_forecast_panel_; }
    [[nodiscard]] PixelCloudSeaForecastPanel& MutableCloudForecastPanel() { return cloud_forecast_panel_; }
    [[nodiscard]] const PixelPlayerStatusPanel& GetPlayerStatusPanel() const { return player_status_panel_; }
    [[nodiscard]] PixelPlayerStatusPanel& MutablePlayerStatusPanel() { return player_status_panel_; }
    [[nodiscard]] const PixelFontRenderer* GetFontRenderer() const { return font_renderer_.get(); }
    [[nodiscard]] PixelFontRenderer* MutableFontRenderer() { return font_renderer_.get(); }

private:
    struct Focusable {
        sf::FloatRect rect;
    };
    void ApplyLayout_(const CloudSeamanor::infrastructure::UiLayoutConfig* layout_config);
    void ApplyScaleLayout_();
    bool HandleFocusNavigation_(const sf::Event::KeyPressed& key);
    void RenderFocusRing_(sf::RenderWindow& window);
    void EmitUiEvent_(UiEventType event_type);
    void RenderTopRightInfo_(sf::RenderWindow& window);
    void RenderBottomRightStatus_(sf::RenderWindow& window);
    void RenderDailyRecommendations_(sf::RenderWindow& window);
    void RenderTutorialOverlay_(sf::RenderWindow& window);
    void RenderSkillBranchOverlay_(sf::RenderWindow& window);
    void RenderFishingQteOverlay_(sf::RenderWindow& window);
    void RenderDiyPlacementOverlay_(sf::RenderWindow& window);
    void ApplySeasonTheme_(const std::string& season_text);
    bool LoadUiAtlas_();
    void DrawUiFrame_(sf::RenderWindow& window,
                      const sf::IntRect& rect,
                      const sf::Vector2f& position,
                      const sf::Vector2f& size) const;

    // 字体
    std::unique_ptr<PixelFontRenderer> font_renderer_;
    CloudSeamanor::infrastructure::ResourceManager* resource_manager_ = nullptr;
    std::string atlas_texture_id_;
    bool ui_atlas_loaded_ = false;

    // 组件
    PixelToolbar toolbar_;
    PixelDialogueBox dialogue_box_;
    PixelInventoryGrid inventory_grid_;
    PixelQuestMenu quest_menu_;
    PixelMinimap minimap_;
    PixelProgressBar stamina_bar_;
    PixelProgressBar hunger_bar_;
    PixelSettingsPanel settings_panel_;
    PixelCloudSeaForecastPanel cloud_forecast_panel_;
    PixelPlayerStatusPanel player_status_panel_;
    PixelTeaGardenPanel tea_garden_panel_;
    PixelWorkshopPanel workshop_panel_;
    PixelContractPanel contract_panel_;
    PixelNpcDetailPanel npc_detail_panel_;
    PixelNpcSchedulePanel npc_schedule_panel_;
    PixelSocialPanel social_panel_;
    PixelSpiritBeastPanel spirit_beast_panel_;
    PixelFestivalPanel festival_panel_;
    PixelSpiritRealmPanel spirit_realm_panel_;
    PixelBuildingPanel building_panel_;
    PixelShopPanel shop_panel_;
    PixelMailPanel mail_panel_;
    PixelAchievementPanel achievement_panel_;
    PixelBeastiaryPanel beastiary_panel_;
    PixelNotificationBanner notification_banner_;
    PixelTooltip tooltip_;
    PixelContextMenu context_menu_;
    PixelTutorialBubble tutorial_bubble_;
    InventoryActionCallbacks inventory_action_callbacks_{};
    PanelActionCallbacks panel_action_callbacks_{};

    // 状态
    PanelState panel_state_;

    // 右上角信息缓存
    std::string top_right_time_text_;
    std::string top_right_season_text_;
    std::string top_right_weather_text_;
    bool top_right_has_new_quest_ = false;
    float top_right_blink_timer_ = 0.0f;

    // 金币缓存
    int coin_amount_ = 0;
    std::string interact_key_hint_ = "E";
    std::string tool_key_hint_ = "Q";
    std::string last_tooltip_item_id_;
    sf::Vector2f last_mouse_pos_{0.0f, 0.0f};

    // UI-021: 今日推荐三件事
    std::vector<std::string> daily_recommendations_;
    int selected_recommendation_ = -1;

    // UI-022
    bool tutorial_visible_ = false;
    sf::FloatRect tutorial_highlight_rect_{{0.0f, 0.0f}, {0.0f, 0.0f}};
    int tutorial_step_delta_ = 0;

    // 几何缓存（用于内联文本）
    mutable sf::VertexArray top_right_bg_;
    mutable sf::VertexArray coin_bg_;
    mutable bool top_right_geometry_dirty_ = true;
    mutable bool coin_geometry_dirty_ = true;
    float stamina_ratio_ = 1.0f;
    float hunger_ratio_ = 1.0f;
    bool low_stamina_active_ = false;
    bool critical_stamina_active_ = false;
    float ui_scale_ = 1.0f;
    std::function<void(UiEventType)> on_ui_event_;
    std::vector<Focusable> focusables_;
    int focus_index_ = -1;
    float focus_breath_timer_ = 0.0f;
    float last_window_width_ = ScreenConfig::Width;

    struct SkillBranchOverlayState {
        bool visible = false;
        std::string skill_name;
        std::string option_a;
        std::string option_b;
        bool choose_a = true;
        std::optional<bool> submitted_choice;
    } skill_branch_overlay_;

    struct FishingQteOverlayState {
        bool visible = false;
        float progress = 0.0f;
        float target_center = 0.5f;
        float target_width = 0.2f;
        std::string title;
        bool confirm_requested = false;
    } fishing_qte_overlay_;

    struct DiyPlacementOverlayState {
        bool visible = false;
        std::string object_name;
        int tile_x = 0;
        int tile_y = 0;
        int rotation = 0;
        int move_x = 0;
        int move_y = 0;
        bool rotate_requested = false;
        bool confirm_requested = false;
        bool pickup_requested = false;
    } diy_overlay_;

public:
    [[nodiscard]] std::optional<bool> ConsumeSkillBranchChoice();
    [[nodiscard]] bool ConsumeFishingQteConfirm();
    [[nodiscard]] sf::Vector2i ConsumeDiyMoveDelta();
    [[nodiscard]] bool ConsumeDiyRotate();
    [[nodiscard]] bool ConsumeDiyConfirm();
    [[nodiscard]] bool ConsumeDiyPickup();

    CloudSeamanor::infrastructure::UiLayoutData layout_data_ = CloudSeamanor::infrastructure::UiLayoutConfig::GetDefaults();
};

}  // namespace CloudSeamanor::engine
