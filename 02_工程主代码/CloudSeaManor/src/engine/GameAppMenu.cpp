#include "CloudSeamanor/GameApp.hpp"

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/ResourceManager.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/UiLayoutConfig.hpp"
#include "CloudSeamanor/UiAtlasMappings.hpp"
#include "CloudSeamanor/PixelUiConfig.hpp"

#include <algorithm>
#include <cstdint>
#include <array>
#include <cmath>
#include <filesystem>

namespace CloudSeamanor::engine {

// RE-206 过渡层：主菜单状态已打包到 main_menu_。
#define main_menu_ ui_state_.main_menu
#define runtime_ready_ ui_state_.runtime_ready
#define show_main_menu_ main_menu_.visible
#define main_menu_index_ main_menu_.index
#define main_menu_has_save_ main_menu_.has_save
#define pending_main_menu_action_ main_menu_.pending_action
#define main_menu_transition_out_ main_menu_.transition_out
#define main_menu_fade_alpha_ main_menu_.fade_alpha
#define main_menu_anim_time_ main_menu_.anim_time
#define main_menu_hover_lerp_ main_menu_.hover_lerp
#define main_menu_panel_ main_menu_.panel
#define main_menu_background_sprite_ main_menu_.background_sprite
#define main_menu_background_loaded_ main_menu_.background_loaded
#define main_menu_corner_blocks_ main_menu_.corner_blocks
#define main_menu_button_rects_ main_menu_.button_rects
#define main_menu_title_ main_menu_.title
#define main_menu_items_ main_menu_.items
#define main_menu_save_preview_text_ main_menu_.save_preview_text
#define main_menu_status_text_ main_menu_.status_text

namespace {

std::uint32_t PackColor(const sf::Color& color) {
    return (static_cast<std::uint32_t>(color.r) << 24)
        | (static_cast<std::uint32_t>(color.g) << 16)
        | (static_cast<std::uint32_t>(color.b) << 8)
        | static_cast<std::uint32_t>(color.a);
}

sf::Color GetMainMenuSelectedColor(const infrastructure::UiLayoutConfig& layout) {
    const std::uint32_t fallback = PackColor(ColorPalette::ActiveGreen);
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
    const std::string atlas_id = main_menu_.atlas_texture_id;
    bool ui_button_loaded = !atlas_id.empty() && resources_ && resources_->HasTexture(atlas_id);
    const sf::Texture* ui_button_texture_ptr = nullptr;
    if (ui_button_loaded) {
        ui_button_texture_ptr = &resources_->GetTexture(atlas_id);
    }

    if (main_menu_background_loaded_ && main_menu_background_sprite_) {
        window.draw(*main_menu_background_sprite_);
    }
    sf::RectangleShape bg_dim({static_cast<float>(ScreenConfig::Width), static_cast<float>(ScreenConfig::Height)});
    bg_dim.setFillColor(sf::Color(24, 24, 24, 26));
    window.draw(bg_dim);
    main_menu_panel_.setFillColor(sf::Color(ColorPalette::Cream.r, ColorPalette::Cream.g, ColorPalette::Cream.b,
                                            static_cast<std::uint8_t>(220.0f * main_menu_fade_alpha_)));
    main_menu_panel_.setOutlineColor(sf::Color(ColorPalette::BrownOutline.r, ColorPalette::BrownOutline.g, ColorPalette::BrownOutline.b,
                                               static_cast<std::uint8_t>(255.0f * main_menu_fade_alpha_)));
    window.draw(main_menu_panel_);
    if (ui_button_loaded) {
        const sf::Vector2f panel_pos = main_menu_panel_.getPosition();
        const sf::Vector2f panel_size = main_menu_panel_.getSize();
        const float border = std::max(8.0f, std::round(main_menu_panel_.getOutlineThickness() * 4.0f));
        const auto draw_patch = [&](const sf::IntRect& src, const sf::FloatRect& dst, const sf::Color& tint) {
            sf::Sprite patch(*ui_button_texture_ptr, src);
            patch.setPosition(dst.position);
            patch.setScale({
                dst.size.x / static_cast<float>(src.size.x),
                dst.size.y / static_cast<float>(src.size.y)
            });
            patch.setColor(tint);
            window.draw(patch);
        };
        const sf::Color panel_tint(255, 255, 255, static_cast<std::uint8_t>(255.0f * main_menu_fade_alpha_));

        // atlas nine-slice: 4 corners + 4 edges
        draw_patch(atlas::kPanelCornerTl,
                   sf::FloatRect({panel_pos.x, panel_pos.y}, {border, border}),
                   panel_tint);
        draw_patch(atlas::kPanelCornerTr,
                   sf::FloatRect({panel_pos.x + panel_size.x - border, panel_pos.y}, {border, border}),
                   panel_tint);
        draw_patch(atlas::kPanelCornerBl,
                   sf::FloatRect({panel_pos.x, panel_pos.y + panel_size.y - border}, {border, border}),
                   panel_tint);
        draw_patch(atlas::kPanelCornerBr,
                   sf::FloatRect({panel_pos.x + panel_size.x - border, panel_pos.y + panel_size.y - border}, {border, border}),
                   panel_tint);
        draw_patch(atlas::kPanelEdgeH,
                   sf::FloatRect({panel_pos.x + border, panel_pos.y}, {panel_size.x - border * 2.0f, border}),
                   panel_tint);
        draw_patch(atlas::kPanelEdgeH,
                   sf::FloatRect({panel_pos.x + border, panel_pos.y + panel_size.y - border}, {panel_size.x - border * 2.0f, border}),
                   panel_tint);
        draw_patch(atlas::kPanelEdgeV,
                   sf::FloatRect({panel_pos.x, panel_pos.y + border}, {border, panel_size.y - border * 2.0f}),
                   panel_tint);
        draw_patch(atlas::kPanelEdgeV,
                   sf::FloatRect({panel_pos.x + panel_size.x - border, panel_pos.y + border}, {border, panel_size.y - border * 2.0f}),
                   panel_tint);
    } else {
        for (const auto& c : main_menu_corner_blocks_) {
            sf::RectangleShape shadow = c;
            shadow.setPosition({c.getPosition().x + 2.0f, c.getPosition().y + 2.0f});
            shadow.setFillColor(sf::Color(50, 34, 20, static_cast<std::uint8_t>(120.0f * main_menu_fade_alpha_)));
            window.draw(shadow);
            sf::RectangleShape corner = c;
            corner.setFillColor(sf::Color(c.getFillColor().r, c.getFillColor().g, c.getFillColor().b,
                                          static_cast<std::uint8_t>(255.0f * main_menu_fade_alpha_)));
            window.draw(corner);
        }
    }
    if (main_menu_title_) {
        main_menu_title_->setFillColor(sf::Color(45, 27, 14, static_cast<std::uint8_t>(255.0f * main_menu_fade_alpha_)));
        window.draw(*main_menu_title_);
    }
    for (int i = 0; i < kMainMenuItemCount; ++i) {
        if (!main_menu_items_[i]) continue;
        const auto& rect = main_menu_button_rects_[i];
        const bool is_continue_disabled = (i == 1 && !main_menu_has_save_);
        if (i == main_menu_index_) {
            const float breathe = 0.85f + 0.15f * (0.5f + 0.5f * std::sin((main_menu_anim_time_ / 0.3f) * 6.2831853f));
            sf::RectangleShape hi({rect.size.x, rect.size.y});
            hi.setPosition(rect.position);
            hi.setFillColor(sf::Color(
                is_continue_disabled ? 190 : ColorPalette::HighlightYellow.r,
                is_continue_disabled ? 190 : ColorPalette::HighlightYellow.g,
                is_continue_disabled ? 190 : ColorPalette::HighlightYellow.b,
                static_cast<std::uint8_t>(200.0f * breathe * main_menu_fade_alpha_)));
            hi.setOutlineThickness(2.0f);
            hi.setOutlineColor(sf::Color(
                is_continue_disabled ? 140 : ColorPalette::ActiveGreen.r,
                is_continue_disabled ? 140 : ColorPalette::ActiveGreen.g,
                is_continue_disabled ? 140 : ColorPalette::ActiveGreen.b,
                                         static_cast<std::uint8_t>(255.0f * breathe * main_menu_fade_alpha_)));
            window.draw(hi);
        }
        if (ui_button_loaded) {
            const int frame_index = is_continue_disabled ? 3
                : ((main_menu_transition_out_ && pending_main_menu_action_ == static_cast<MainMenuAction>(i)) ? 2
                                                                                                                : (i == main_menu_index_ ? 1 : 0));
            const sf::IntRect button_rect = (frame_index == 3) ? atlas::kBtnDefault3
                : ((frame_index == 2) ? atlas::kBtnDefault2
                                      : ((frame_index == 1) ? atlas::kBtnDefault1 : atlas::kBtnDefault0));
            sf::Sprite button(*ui_button_texture_ptr, button_rect);
            const float hover = main_menu_hover_lerp_[i];
            const float scale = 1.0f + hover * 0.04f;
            const sf::Vector2f center{rect.position.x + rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f};
            const sf::Vector2f target_size{rect.size.x * scale, rect.size.y * scale};
            const float uniform = std::min(target_size.x / static_cast<float>(button_rect.size.x),
                                           target_size.y / static_cast<float>(button_rect.size.y));
            const sf::Vector2f draw_size{static_cast<float>(button_rect.size.x) * uniform,
                                         static_cast<float>(button_rect.size.y) * uniform};
            button.setPosition({center.x - draw_size.x * 0.5f, center.y - draw_size.y * 0.5f});
            button.setScale({uniform, uniform});
            button.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.0f * main_menu_fade_alpha_)));
            window.draw(button);
        }
        main_menu_items_[i]->setFillColor(
            i == main_menu_index_ ? GetMainMenuSelectedColor(ui_layout_config_)
                                  : GetMainMenuNormalColor(ui_layout_config_));
        if (is_continue_disabled) {
            main_menu_items_[i]->setFillColor(sf::Color(120, 120, 120, static_cast<std::uint8_t>(210.0f * main_menu_fade_alpha_)));
            main_menu_items_[i]->setScale({1.0f, 1.0f});
        } else {
            main_menu_items_[i]->setScale({1.0f + main_menu_hover_lerp_[i] * 0.04f, 1.0f + main_menu_hover_lerp_[i] * 0.04f});
        }
        const auto c = main_menu_items_[i]->getFillColor();
        main_menu_items_[i]->setFillColor(sf::Color(c.r, c.g, c.b, static_cast<std::uint8_t>(255.0f * main_menu_fade_alpha_)));
        window.draw(*main_menu_items_[i]);
    }
    if (main_menu_save_preview_text_) {
        main_menu_save_preview_text_->setFillColor(sf::Color(80, 62, 45, static_cast<std::uint8_t>(235.0f * main_menu_fade_alpha_)));
        window.draw(*main_menu_save_preview_text_);
    }
    if (main_menu_status_text_) {
        main_menu_status_text_->setFillColor(sf::Color(96, 88, 74, static_cast<std::uint8_t>(235.0f * main_menu_fade_alpha_)));
        window.draw(*main_menu_status_text_);
    }
}

void GameApp::HandleMainMenuInput_(const sf::Event::KeyPressed& key_event) {
    if (key_event.scancode == sf::Keyboard::Scancode::Up) {
        main_menu_index_ = (main_menu_index_ + kMainMenuItemCount - 1) % kMainMenuItemCount;
        return;
    }
    if (key_event.scancode == sf::Keyboard::Scancode::Down) {
        main_menu_index_ = (main_menu_index_ + 1) % kMainMenuItemCount;
        return;
    }
    if (key_event.scancode == sf::Keyboard::Scancode::Left) {
        main_menu_index_ = (main_menu_index_ + kMainMenuItemCount - 1) % kMainMenuItemCount;
        return;
    }
    if (key_event.scancode == sf::Keyboard::Scancode::Right) {
        main_menu_index_ = (main_menu_index_ + 1) % kMainMenuItemCount;
        return;
    }
    if (key_event.scancode == sf::Keyboard::Scancode::Enter) {
        ExecuteMainMenuSelection_();
    }
}

void GameApp::ExecuteMainMenuSelection_() {
    const auto SetStatusUtf8 = [this](const char8_t* literal) {
        if (!main_menu_status_text_) return;
        const auto* begin = reinterpret_cast<const char*>(literal);
        const auto* end = begin;
        while (*end != '\0') {
            ++end;
        }
        main_menu_status_text_->setString(sf::String::fromUtf8(begin, end));
    };

    if (!runtime_ready_ && (main_menu_index_ == 0 || main_menu_index_ == 1)) {
        SetStatusUtf8(u8"运行时未就绪：请先检查资源配置。");
        return;
    }
    if (main_menu_index_ == 1 && !main_menu_has_save_) {
        SetStatusUtf8(u8"当前无存档：请先开始新游戏。");
        return;
    }
    pending_main_menu_action_ = static_cast<MainMenuAction>(main_menu_index_);
    main_menu_transition_out_ = true;
}

}  // namespace CloudSeamanor::engine
