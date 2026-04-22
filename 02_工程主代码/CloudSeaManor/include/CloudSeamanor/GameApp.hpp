#pragma once

#include "CloudSeamanor/GameRuntime.hpp"
#include "CloudSeamanor/InputManager.hpp"
#include "CloudSeamanor/UiLayoutConfig.hpp"
#include "CloudSeamanor/PixelGameHud.hpp"
#include "CloudSeamanor/UISystem.hpp"
#include "CloudSeamanor/ResourceManager.hpp"
#include "CloudSeamanor/engine/input/InputHandler.hpp"
#include "CloudSeamanor/engine/rendering/HudRenderer.hpp"
#include "CloudSeamanor/engine/rendering/WorldRenderer.hpp"
#include "CloudSeamanor/AudioManager.hpp"
#include "CloudSeamanor/LoadingScreen.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>
#include <memory>
#include <string>
#include <functional>

namespace CloudSeamanor::engine {

class GameApp {
public:
    GameApp() : input_handler_(input_) {}
    ~GameApp();

    int Run();

private:
    bool TryLoadConfiguredFont_(const std::string& font_id, const std::string& font_spec);
    void ProcessEvents(sf::RenderWindow& window);
    void Update(float delta_seconds);
    void UpdateUi_();
    void Render(sf::RenderWindow& window);
    void HandleWindowResize_(const sf::Event::Resized& resize_event, sf::RenderWindow& window);
    void ForwardEventToHud_(const sf::Event& event);
    void UpdatePixelHud_(float delta_seconds);
    void SetupInputCallbacks_();
    [[nodiscard]] InputHandler::PanelCallbacks BuildPanelCallbacks_();
    [[nodiscard]] InputHandler::GameCallbacks BuildGameCallbacks_();
    void SetupDialogueCallbacks_();
    void UpdateRuntimeMetrics_(float delta_seconds);
    void InitializeMainMenu_(const sf::Font* font);
    void RenderMainMenu_(sf::RenderWindow& window);
    void HandleMainMenuInput_(const sf::Event::KeyPressed& key_event);
    void ExecuteMainMenuSelection_();
    void RunWithLoading_(const std::string& stage_text, const std::function<void()>& fn);
    [[nodiscard]] static std::string CloudStateKey_(const CloudSeamanor::domain::CloudState& state);
    [[nodiscard]] static sf::Color BgColor_(
        const CloudSeamanor::infrastructure::UiLayoutConfig& layout,
        const CloudSeamanor::domain::CloudState& state);
    [[nodiscard]] static sf::Color AuraColor_(
        const CloudSeamanor::infrastructure::UiLayoutConfig& layout,
        const CloudSeamanor::domain::CloudState& state);

    sf::RenderWindow* window_ptr_ = nullptr;
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
    float fps_accumulator_ = 0.0f;
    int fps_frame_counter_ = 0;
    int latest_fps_ = 0;
    bool show_main_menu_ = true;
    int main_menu_index_ = 0;
    sf::RectangleShape main_menu_panel_;
    std::unique_ptr<sf::Text> main_menu_title_;
    std::unique_ptr<sf::Text> main_menu_items_[3];
};

}  // namespace CloudSeamanor::engine
