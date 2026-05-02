#pragma once

#include "CloudSeamanor/engine/GameRuntime.hpp"
#include "CloudSeamanor/engine/InputManager.hpp"
#include "CloudSeamanor/infrastructure/UiLayoutConfig.hpp"
#include "CloudSeamanor/infrastructure/SpriteMapping.hpp"
#include "CloudSeamanor/engine/PixelGameHud.hpp"
#include "CloudSeamanor/engine/UISystem.hpp"
#include "CloudSeamanor/infrastructure/ResourceManager.hpp"
#include "CloudSeamanor/engine/input/InputHandler.hpp"
#include "CloudSeamanor/engine/rendering/HudRenderer.hpp"
#include "CloudSeamanor/engine/rendering/WorldRenderer.hpp"
#include "CloudSeamanor/engine/presentation/HudPanelPresenters.hpp"
#include "CloudSeamanor/engine/presentation/HudSideEffects.hpp"
#include "CloudSeamanor/engine/AudioManager.hpp"
#include "CloudSeamanor/engine/LoadingScreen.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Window/Event.hpp>
#include <memory>
#include <string>
#include <functional>
#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【SpriteThemeManager】多主题皮肤管理器
// ============================================================================
class SpriteThemeManager {
public:
    struct Theme {
        std::string id;
        std::string name;
        std::string display_name;      // 显示名称
        std::string mapping_csv;       // 映射表路径
        std::string description;
    };

    SpriteThemeManager() = default;

    // 加载主题列表配置
    bool LoadThemeConfig(const std::string& config_path);

    // 获取当前主题
    [[nodiscard]] const Theme* GetCurrentTheme() const;
    [[nodiscard]] const std::string& GetCurrentThemeId() const { return current_theme_id_; }

    // 切换主题
    bool SwitchTheme(const std::string& theme_id);

    // 获取所有主题
    [[nodiscard]] const std::vector<Theme>& GetAllThemes() const { return themes_; }

    // 加载映射表
    [[nodiscard]] CloudSeamanor::infrastructure::SpriteMapping* GetCurrentMapping() { return &current_mapping_; }

private:
    static std::vector<std::string> ParseCsvLine_(const std::string& line);
    static std::string Trim(const std::string& str);

    std::vector<Theme> themes_;
    std::string current_theme_id_;
    CloudSeamanor::infrastructure::SpriteMapping current_mapping_;
};

class GameApp {
public:
    // ========================================================================
    // 【公共常量与类型】放在类开头，供访问器和结构体使用
    // ========================================================================
    static constexpr int kMainMenuItemCount = 5;

    enum class MainMenuAction {
        None = -1,
        StartGame = 0,
        ContinueGame = 1,
        Settings = 2,
        About = 3,
        ExitGame = 4
    };

    struct SpiritNameInputState {
        bool active = false;
        std::string buffer;
    };

    struct MainMenuScreen {
        bool visible = true;
        int index = 0;
        bool has_save = false;
        MainMenuAction pending_action = MainMenuAction::None;
        bool transition_out = false;
        float fade_alpha = 0.0f;
        float anim_time = 0.0f;
        std::array<float, kMainMenuItemCount> hover_lerp{};
        sf::RectangleShape panel{sf::Vector2f{1.0f, 1.0f}};
        std::unique_ptr<sf::Sprite> background_sprite;
        bool background_loaded = false;
        std::string atlas_texture_id;
        std::array<sf::RectangleShape, 4> corner_blocks{
            sf::RectangleShape({1.0f, 1.0f}),
            sf::RectangleShape({1.0f, 1.0f}),
            sf::RectangleShape({1.0f, 1.0f}),
            sf::RectangleShape({1.0f, 1.0f})
        };
        std::array<sf::FloatRect, kMainMenuItemCount> button_rects{};
        std::unique_ptr<sf::Text> title;
        std::unique_ptr<sf::Text> items[kMainMenuItemCount];
        std::unique_ptr<sf::Text> save_preview_text;
        std::unique_ptr<sf::Text> status_text;
    };

    struct UiState {
        bool runtime_ready = false;
        bool pending_exit = false;
        HudSideEffects::State hud_side_effects{};

        std::optional<int> pending_save_overwrite_slot{};
        float pending_save_overwrite_seconds = 0.0f;

        SpiritNameInputState spirit_name_input{};
        MainMenuScreen main_menu{};
    };

    // ========================================================================
    // 【构造函数与主循环】
    // ========================================================================
    GameApp() : input_handler_(input_) {}
    ~GameApp();
    int Run();

    // ========================================================================
    // 【UiState 访问器】RE-206 过渡层：逐步替换宏定义
    // ========================================================================
    [[nodiscard]] bool& RuntimeReady() { return ui_state_.runtime_ready; }
    [[nodiscard]] bool& PendingExit() { return ui_state_.pending_exit; }
    [[nodiscard]] SpiritNameInputState& SpiritNameInput() { return ui_state_.spirit_name_input; }
    [[nodiscard]] std::optional<int>& PendingSaveOverwriteSlot() { return ui_state_.pending_save_overwrite_slot; }
    [[nodiscard]] float& PendingSaveOverwriteSeconds() { return ui_state_.pending_save_overwrite_seconds; }

    // MainMenuScreen 访问器
    [[nodiscard]] MainMenuScreen& MainMenu() { return ui_state_.main_menu; }
    [[nodiscard]] const MainMenuScreen& MainMenu() const { return ui_state_.main_menu; }
    [[nodiscard]] bool& ShowMainMenu() { return ui_state_.main_menu.visible; }
    [[nodiscard]] int& MainMenuIndex() { return ui_state_.main_menu.index; }
    [[nodiscard]] bool& MainMenuHasSave() { return ui_state_.main_menu.has_save; }
    [[nodiscard]] MainMenuAction& PendingMainMenuAction() { return ui_state_.main_menu.pending_action; }
    [[nodiscard]] bool& MainMenuTransitionOut() { return ui_state_.main_menu.transition_out; }
    [[nodiscard]] float& MainMenuFadeAlpha() { return ui_state_.main_menu.fade_alpha; }
    [[nodiscard]] float& MainMenuAnimTime() { return ui_state_.main_menu.anim_time; }
    [[nodiscard]] std::array<float, kMainMenuItemCount>& MainMenuHoverLerp() { return ui_state_.main_menu.hover_lerp; }
    [[nodiscard]] sf::RectangleShape& MainMenuPanel() { return ui_state_.main_menu.panel; }
    [[nodiscard]] std::unique_ptr<sf::Sprite>& MainMenuBackgroundSprite() { return ui_state_.main_menu.background_sprite; }
    [[nodiscard]] bool& MainMenuBackgroundLoaded() { return ui_state_.main_menu.background_loaded; }
    [[nodiscard]] std::string& MainMenuAtlasTextureId() { return ui_state_.main_menu.atlas_texture_id; }
    [[nodiscard]] std::array<sf::RectangleShape, 4>& MainMenuCornerBlocks() { return ui_state_.main_menu.corner_blocks; }
    [[nodiscard]] std::array<sf::FloatRect, kMainMenuItemCount>& MainMenuButtonRects() { return ui_state_.main_menu.button_rects; }
    [[nodiscard]] std::unique_ptr<sf::Text>& MainMenuTitle() { return ui_state_.main_menu.title; }
    [[nodiscard]] auto& MainMenuItems() { return ui_state_.main_menu.items; }
    [[nodiscard]] std::unique_ptr<sf::Text>& MainMenuSavePreviewText() { return ui_state_.main_menu.save_preview_text; }
    [[nodiscard]] std::unique_ptr<sf::Text>& MainMenuStatusText() { return ui_state_.main_menu.status_text; }

private:
    bool TryLoadConfiguredFont_(const std::string& font_id, const std::string& font_spec);
    void SetupInputCallbacks_();
    [[nodiscard]] InputHandler::PanelCallbacks BuildPanelCallbacks_();
    [[nodiscard]] InputHandler::GameCallbacks BuildGameCallbacks_();
    void ProcessEvents(sf::RenderWindow& window);
    void Update(float delta_seconds);
    void UpdateUi_();
    void Render(sf::RenderWindow& window);
    [[nodiscard]] HudSideEffects::HudEffectContext BuildHudEffectContext_();
    void HandleHudSideEffects_();
    void HandleWindowResize_(const sf::Event::Resized& resize_event, sf::RenderWindow& window);
    void ForwardEventToHud_(const sf::Event& event, sf::RenderWindow& window);
    void UpdatePixelHud_(float delta_seconds);
    void InitializeMainMenu_(const sf::Font* font, sf::Vector2u window_size);
    void UpdateMainMenuLayout_(sf::Vector2u window_size);
    void RenderMainMenu_(sf::RenderWindow& window);
    void HandleMainMenuInput_(const sf::Event::KeyPressed& key_event);
    void ExecuteMainMenuSelection_();
    void PerformMainMenuAction_(MainMenuAction action);
    void RunWithLoading_(sf::RenderWindow& window, const std::string& stage_text, const std::function<void()>& fn);
    [[nodiscard]] static std::string CloudStateKey_(const CloudSeamanor::domain::CloudState& state);
    [[nodiscard]] static sf::Color BgColor_(
        const CloudSeamanor::infrastructure::UiLayoutConfig& layout,
        const CloudSeamanor::domain::CloudState& state);
    [[nodiscard]] static sf::Color AuraColor_(
        const CloudSeamanor::infrastructure::UiLayoutConfig& layout,
        const CloudSeamanor::domain::CloudState& state);

    // ============================================================================
    // 新系统辅助函数
    // ============================================================================
    void TryStartFishing_();

    GameRuntime runtime_;
    InputManager input_;
    InputHandler input_handler_;
    WorldRenderer world_renderer_;
    std::unique_ptr<HudRenderer> hud_renderer_;
    std::unique_ptr<UISystem> ui_system_;
    std::unique_ptr<PixelGameHud> pixel_hud_;
    std::unique_ptr<CloudSeamanor::infrastructure::ResourceManager> resources_;
    std::unique_ptr<CloudSeamanor::engine::audio::AudioManager> audio_;
    LoadingScreen loading_screen_;
    CloudSeamanor::infrastructure::UiLayoutConfig ui_layout_config_;
    SpriteThemeManager theme_manager_;
    UiState ui_state_{};
};

}  // namespace CloudSeamanor::engine
