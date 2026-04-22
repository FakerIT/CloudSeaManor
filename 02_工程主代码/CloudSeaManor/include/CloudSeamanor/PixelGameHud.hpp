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

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelDialogueBox.hpp"
#include "CloudSeamanor/PixelFontRenderer.hpp"
#include "CloudSeamanor/PixelInventoryGrid.hpp"
#include "CloudSeamanor/PixelNotificationBanner.hpp"
#include "CloudSeamanor/PixelTooltip.hpp"
#include "CloudSeamanor/PixelCloudSeaForecastPanel.hpp"
#include "CloudSeamanor/PixelPlayerStatusPanel.hpp"
#include "CloudSeamanor/PixelTeaGardenPanel.hpp"
#include "CloudSeamanor/PixelWorkshopPanel.hpp"
#include "CloudSeamanor/PixelContractPanel.hpp"
#include "CloudSeamanor/PixelNpcDetailPanel.hpp"
#include "CloudSeamanor/PixelSpiritBeastPanel.hpp"
#include "CloudSeamanor/PixelFestivalPanel.hpp"
#include "CloudSeamanor/PixelSpiritRealmPanel.hpp"
#include "CloudSeamanor/PixelBuildingPanel.hpp"
#include "CloudSeamanor/PixelShopPanel.hpp"
#include "CloudSeamanor/PixelMailPanel.hpp"
#include "CloudSeamanor/PixelAchievementPanel.hpp"
#include "CloudSeamanor/PixelBeastiaryPanel.hpp"
#include "CloudSeamanor/PixelContextMenu.hpp"
#include "CloudSeamanor/PixelMinimap.hpp"
#include "CloudSeamanor/PixelProgressBar.hpp"
#include "CloudSeamanor/PixelQuestMenu.hpp"
#include "CloudSeamanor/PixelSettingsPanel.hpp"
#include "CloudSeamanor/PixelToolbar.hpp"
#include "CloudSeamanor/PixelTutorialBubble.hpp"
#include "CloudSeamanor/PixelUiConfig.hpp"
#include "CloudSeamanor/UiLayoutConfig.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
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
    bool cloud_forecast_open = false;
    bool player_status_open = false;
};

// ============================================================================
// 【PixelGameHud】像素 HUD 管理器
// ============================================================================
class PixelGameHud {
public:
    enum class UiEventType : std::uint8_t { Open, Close, Select, Error, Achievement };
    // ========================================================================
    // 【初始化】
    // ========================================================================

    /**
     * @brief 初始化所有像素 UI 组件
     * @param font 字体引用
     */
    void Initialize(const sf::Font& font);
    void Initialize(const sf::Font& font, const infrastructure::UiLayoutConfig* layout_config);
    void SetUiScale(float scale);
    void SetUiEventCallback(std::function<void(UiEventType)> cb) { on_ui_event_ = std::move(cb); }

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
    void PushNotification(const std::string& message);
    void UpdateDailyRecommendations(const std::vector<std::string>& items);
    void UpdateTutorialBubble(int step_index,
                              const std::string& text,
                              const sf::FloatRect& highlight_rect,
                              bool visible);
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
    [[nodiscard]] PixelToolbar& GetToolbar() { return toolbar_; }
    [[nodiscard]] PixelDialogueBox& GetDialogueBox() { return dialogue_box_; }
    [[nodiscard]] PixelInventoryGrid& GetInventory() { return inventory_grid_; }
    [[nodiscard]] PixelQuestMenu& GetQuestMenu() { return quest_menu_; }
    [[nodiscard]] PixelMinimap& GetMinimap() { return minimap_; }
    [[nodiscard]] PixelProgressBar& GetStaminaBar() { return stamina_bar_; }
    [[nodiscard]] PixelSettingsPanel& GetSettingsPanel() { return settings_panel_; }
    [[nodiscard]] PixelCloudSeaForecastPanel& GetCloudForecastPanel() { return cloud_forecast_panel_; }
    [[nodiscard]] PixelPlayerStatusPanel& GetPlayerStatusPanel() { return player_status_panel_; }
    [[nodiscard]] PixelFontRenderer* GetFontRenderer() { return font_renderer_.get(); }

private:
    struct Focusable {
        sf::FloatRect rect;
    };
    void ApplyLayout_(const infrastructure::UiLayoutConfig* layout_config);
    void ApplyScaleLayout_();
    bool HandleFocusNavigation_(const sf::Event::KeyPressed& key);
    void RenderFocusRing_(sf::RenderWindow& window);
    void EmitUiEvent_(UiEventType event_type);
    void RenderTopRightInfo_(sf::RenderWindow& window);
    void RenderBottomRightStatus_(sf::RenderWindow& window);
    void RenderDailyRecommendations_(sf::RenderWindow& window);
    void RenderTutorialOverlay_(sf::RenderWindow& window);

    // 字体
    std::unique_ptr<PixelFontRenderer> font_renderer_;

    // 组件
    PixelToolbar toolbar_;
    PixelDialogueBox dialogue_box_;
    PixelInventoryGrid inventory_grid_;
    PixelQuestMenu quest_menu_;
    PixelMinimap minimap_;
    PixelProgressBar stamina_bar_;
    PixelSettingsPanel settings_panel_;
    PixelCloudSeaForecastPanel cloud_forecast_panel_;
    PixelPlayerStatusPanel player_status_panel_;
    PixelTeaGardenPanel tea_garden_panel_;
    PixelWorkshopPanel workshop_panel_;
    PixelContractPanel contract_panel_;
    PixelNpcDetailPanel npc_detail_panel_;
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
    bool low_stamina_active_ = false;
    bool critical_stamina_active_ = false;
    float ui_scale_ = 1.0f;
    std::function<void(UiEventType)> on_ui_event_;
    std::vector<Focusable> focusables_;
    int focus_index_ = -1;
    float focus_breath_timer_ = 0.0f;

    infrastructure::UiLayoutData layout_data_ = infrastructure::UiLayoutConfig::GetDefaults();
};

}  // namespace CloudSeamanor::engine
