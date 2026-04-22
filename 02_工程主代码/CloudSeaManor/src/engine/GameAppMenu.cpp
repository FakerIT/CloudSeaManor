#include "CloudSeamanor/GameApp.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/ResourceManager.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/UiLayoutConfig.hpp"

#include <cstdint>
#include <array>
#include <filesystem>

namespace CloudSeamanor::engine {

namespace {

std::uint32_t PackColor(const sf::Color& color) {
    return (static_cast<std::uint32_t>(color.r) << 24)
        | (static_cast<std::uint32_t>(color.g) << 16)
        | (static_cast<std::uint32_t>(color.b) << 8)
        | static_cast<std::uint32_t>(color.a);
}

sf::Color GetMainMenuSelectedColor(const infrastructure::UiLayoutConfig& layout) {
    const std::uint32_t fallback = PackColor(ColorPalette::LightBrown);
    return adapter::PackedRgbaToColor(layout.GetSemanticColor("main_menu_item_selected", fallback));
}

sf::Color GetMainMenuNormalColor(const infrastructure::UiLayoutConfig& layout) {
    const std::uint32_t fallback = PackColor(ColorPalette::TextBrown);
    return adapter::PackedRgbaToColor(layout.GetSemanticColor("main_menu_item_normal", fallback));
}

}  // namespace

bool GameApp::TryLoadConfiguredFont_(const std::string& font_id, const std::string& font_spec) {
    auto TryLoad = [this, &font_id](const std::filesystem::path& candidate) -> bool {
        std::error_code ec;
        if (!std::filesystem::exists(candidate, ec)) {
            return false;
        }
        if (!resources_->LoadFont(font_id, candidate.string())) {
            return false;
        }
        resources_->SetMainFontId(font_id);
        return true;
    };

    if (font_spec.rfind("system:", 0) == 0) {
        if (resources_->LoadBestAvailableFont(font_id)) {
            resources_->SetMainFontId(font_id);
            return true;
        }
        return false;
    }

    if (font_spec.rfind("assets:", 0) == 0) {
        const std::filesystem::path asset_path = std::filesystem::path("assets/fonts") / font_spec.substr(7);
        return TryLoad(asset_path);
    }

    const std::filesystem::path configured_path(font_spec);
    if (configured_path.is_absolute() && TryLoad(configured_path)) {
        return true;
    }
    if (TryLoad(configured_path)) {
        return true;
    }
    return TryLoad(std::filesystem::path("assets/fonts") / configured_path);
}

void GameApp::RenderMainMenu_(sf::RenderWindow& window) {
    window.draw(main_menu_panel_);
    if (main_menu_title_) {
        window.draw(*main_menu_title_);
    }
    for (int i = 0; i < 3; ++i) {
        if (!main_menu_items_[i]) continue;
        main_menu_items_[i]->setFillColor(
            i == main_menu_index_ ? GetMainMenuSelectedColor(ui_layout_config_)
                                  : GetMainMenuNormalColor(ui_layout_config_));
        window.draw(*main_menu_items_[i]);
    }
}

void GameApp::HandleMainMenuInput_(const sf::Event::KeyPressed& key_event) {
    if (key_event.scancode == sf::Keyboard::Scancode::Up) {
        main_menu_index_ = (main_menu_index_ + 2) % 3;
        return;
    }
    if (key_event.scancode == sf::Keyboard::Scancode::Down) {
        main_menu_index_ = (main_menu_index_ + 1) % 3;
        return;
    }
    if (key_event.scancode == sf::Keyboard::Scancode::Enter) {
        ExecuteMainMenuSelection_();
    }
}

void GameApp::ExecuteMainMenuSelection_() {
    if (main_menu_index_ == 0) {
        show_main_menu_ = false;
        return;
    }
    if (main_menu_index_ == 1) {
        runtime_.LoadGame();
        show_main_menu_ = false;
        return;
    }
    runtime_.Callbacks().push_hint("设置面板可在游戏内按 Esc 打开。", 2.4f);
}

}  // namespace CloudSeamanor::engine
