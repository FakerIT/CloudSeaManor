#include "CloudSeamanor/PixelSettingsPanel.hpp"

#include "CloudSeamanor/PixelUiConfig.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <string>

namespace CloudSeamanor::engine {

PixelSettingsPanel::PixelSettingsPanel()
    : PixelUiPanel({{300.0f, 110.0f}, {680.0f, 500.0f}}, "设置", true) {
    for (int i = 0; i < 3; ++i) {
        slots_[static_cast<std::size_t>(i)].slot_index = i + 1;
    }
}

void PixelSettingsPanel::SetVisible(bool visible) {
    visible_ = visible;
    PixelUiPanel::SetVisible(visible);
}

void PixelSettingsPanel::MoveSelection(int delta) {
    selected_row_ = std::clamp(selected_row_ + delta, 0, 4);
}

void PixelSettingsPanel::AdjustValue(int delta) {
    if (selected_row_ == 0 || selected_row_ == 1) {
        selected_slot_ = std::clamp(selected_slot_ + delta, 1, 3);
        return;
    }
    if (selected_row_ == 2) {
        bgm_volume_ = std::clamp(bgm_volume_ + static_cast<float>(delta) * 0.05f, 0.0f, 1.0f);
        return;
    }
    if (selected_row_ == 3) {
        sfx_volume_ = std::clamp(sfx_volume_ + static_cast<float>(delta) * 0.05f, 0.0f, 1.0f);
        return;
    }
    if (selected_row_ == 4 && delta != 0) {
        fullscreen_ = !fullscreen_;
    }
}

void PixelSettingsPanel::SetSlots(const std::vector<CloudSeamanor::infrastructure::SaveSlotMetadata>& slots) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (i < slots.size()) {
            slots_[i] = slots[i];
        } else {
            slots_[i].exists = false;
            slots_[i].saved_at_text.clear();
            slots_[i].season_text.clear();
            slots_[i].day = 0;
        }
    }
    RefreshThumbnails_();
}

void PixelSettingsPanel::RefreshThumbnails_() {
    for (std::size_t i = 0; i < thumbnails_.size(); ++i) {
        auto& t = thumbnails_[i];
        t.loaded = false;

        const std::string& p = slots_[i].thumbnail_path;
        if (p.empty() || !slots_[i].exists) {
            t.path.clear();
            continue;
        }
        if (t.path == p && t.texture.getSize().x > 0) {
            t.loaded = true;
            continue;
        }
        t.path = p;
        if (t.texture.loadFromFile(p)) {
            t.texture.setSmooth(false);
            t.loaded = true;
        } else {
            t.loaded = false;
        }
    }
}

void PixelSettingsPanel::RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
    if (!visible_ || font_renderer_ == nullptr || !font_renderer_->IsLoaded()) {
        return;
    }
    const float x = inner_rect.position.x + 16.0f;
    const float y = inner_rect.position.y + 12.0f;
    const float line_h = 28.0f;
    const char* cursor = ">";
    const auto row_prefix = [&](int row) -> std::string {
        return selected_row_ == row ? std::string(cursor) + " " : "  ";
    };

    font_renderer_->DrawText(window, text_config_.slots_title, x, y, TextStyle::Default());
    font_renderer_->DrawText(window, row_prefix(0) + text_config_.save_slot_prefix + " " + std::to_string(selected_slot_), x, y + line_h, TextStyle::Default());
    font_renderer_->DrawText(window, row_prefix(1) + text_config_.load_slot_prefix + " " + std::to_string(selected_slot_), x, y + line_h * 2.0f, TextStyle::Default());
    font_renderer_->DrawText(window, row_prefix(2) + text_config_.bgm_prefix + " " + std::to_string(static_cast<int>(bgm_volume_ * 100.0f)) + "%", x, y + line_h * 3.0f, TextStyle::Default());
    font_renderer_->DrawText(window, row_prefix(3) + text_config_.sfx_prefix + " " + std::to_string(static_cast<int>(sfx_volume_ * 100.0f)) + "%", x, y + line_h * 4.0f, TextStyle::Default());
    font_renderer_->DrawText(window,
                             row_prefix(4) + text_config_.display_mode_prefix + " "
                                 + (fullscreen_ ? text_config_.fullscreen_text : text_config_.windowed_text),
                             x,
                             y + line_h * 5.0f,
                             TextStyle::Default());
    font_renderer_->DrawText(window, text_config_.operation_hint, x, y + line_h * 6.3f, TextStyle::HotkeyHint());

    float slot_y = y + line_h * 7.3f;
    const float preview_w = 160.0f;
    const float preview_h = 90.0f;
    const float preview_x = inner_rect.position.x + inner_rect.size.x - preview_w - 16.0f;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const auto& slot = slots_[i];
        std::string line = text_config_.slot_prefix + " " + std::to_string(i + 1) + ": ";
        if (!slot.exists) {
            line += text_config_.empty_slot_text;
        } else {
            line += (slot.saved_at_text.empty() ? text_config_.has_save_text : slot.saved_at_text);
            if (slot.day > 0) {
                line += " | " + text_config_.day_prefix + std::to_string(slot.day);
            }
            if (!slot.season_text.empty()) {
                line += " | " + slot.season_text;
            }
        }
        font_renderer_->DrawText(window, line, x, slot_y, TextStyle::TopRightInfo());

        // 存档缩略图：160x90。无缩略图时显示占位符。
        const float preview_y = slot_y - 6.0f;
        if (thumbnails_[i].loaded) {
            sf::Sprite s(thumbnails_[i].texture);
            s.setPosition({preview_x, preview_y});
            const auto ts = thumbnails_[i].texture.getSize();
            if (ts.x > 0 && ts.y > 0) {
                s.setScale({preview_w / static_cast<float>(ts.x), preview_h / static_cast<float>(ts.y)});
            }
            window.draw(s, sf::RenderStates::Default);
        } else {
            sf::RectangleShape box;
            box.setPosition({preview_x, preview_y});
            box.setSize({preview_w, preview_h});
            box.setFillColor(ColorPalette::LightGray);
            box.setOutlineColor(ColorPalette::BrownOutline);
            box.setOutlineThickness(1.0f);
            window.draw(box, sf::RenderStates::Default);

            TextStyle ph = TextStyle::Default();
            ph.character_size = 12;
            ph.fill_color = ColorPalette::DeepBrown;
            font_renderer_->DrawCenteredText(window, text_config_.no_preview_text,
                                             {preview_x + preview_w * 0.5f, preview_y + preview_h * 0.5f},
                                             ph);
        }
        slot_y += 22.0f;
    }
}

}  // namespace CloudSeamanor::engine
