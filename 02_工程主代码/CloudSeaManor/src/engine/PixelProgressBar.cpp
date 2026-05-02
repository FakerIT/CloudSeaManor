#include "CloudSeamanor/engine/PixelProgressBar.hpp"
#include "CloudSeamanor/engine/UiVertexHelpers.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::engine::uivx::AddQuad;
}  // namespace

// ============================================================================
// 【PixelProgressBar::PixelProgressBar】默认构造
// ============================================================================
PixelProgressBar::PixelProgressBar()
    : rect_{StaminaBarConfig::Position, StaminaBarConfig::Size},
      bg_vertices_(sf::PrimitiveType::Triangles),
      fill_vertices_(sf::PrimitiveType::Triangles) {
    RebuildGeometry_();
}

// ============================================================================
// 【PixelProgressBar::PixelProgressBar】带参数构造
// ============================================================================
PixelProgressBar::PixelProgressBar(const sf::Vector2f& position,
                                   const sf::Vector2f& size)
    : rect_{position, size},
      bg_vertices_(sf::PrimitiveType::Triangles),
      fill_vertices_(sf::PrimitiveType::Triangles) {
    RebuildGeometry_();
}

// ============================================================================
// 【PixelProgressBar::SetProgress】
// ============================================================================
void PixelProgressBar::SetProgress(float ratio) {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    if (std::abs(progress_ - ratio) < 1e-5f) return;
    progress_ = ratio;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelProgressBar::SetLowState】
// ============================================================================
void PixelProgressBar::SetLowState(bool low) {
    if (low_state_ == low) return;
    low_state_ = low;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelProgressBar::SetLabel】
// ============================================================================
void PixelProgressBar::SetLabel(const std::string& label) {
    label_ = label;
}

void PixelProgressBar::SetStaminaStyle() {
    SetSize(StaminaBarConfig::Size);
    SetColors(ColorPalette::StaminaBarBg,
              ColorPalette::StaminaBarOutline,
              ColorPalette::StaminaNormal,
              ColorPalette::StaminaLow,
              ColorPalette::StaminaFull);
}

void PixelProgressBar::SetFavorStyle() {
    SetSize({120.0f, 12.0f});
    SetColors(ColorPalette::LightCream,
              ColorPalette::BrownOutline,
              ColorPalette::WarningPink,
              ColorPalette::WarningPink,
              ColorPalette::ActiveGreen);
}

void PixelProgressBar::SetCropStyle() {
    SetSize({64.0f, 8.0f});
    SetColors(ColorPalette::LightGray,
              ColorPalette::BrownOutline,
              ColorPalette::ActiveGreen,
              ColorPalette::WarningPink,
              ColorPalette::SuccessGreen);
}

void PixelProgressBar::SetWorkshopStyle() {
    SetSize({360.0f, 12.0f});
    SetColors(ColorPalette::LightGray,
              ColorPalette::BrownOutline,
              ColorPalette::DeepBrown,
              ColorPalette::WarningPink,
              ColorPalette::LightBrown);
}

void PixelProgressBar::SetContractStyle() {
    SetSize({160.0f, 10.0f});
    SetColors(ColorPalette::LightGray,
              ColorPalette::BrownOutline,
              ColorPalette::Season::SummerBlue,
              ColorPalette::WarningPink,
              ColorPalette::Season::SummerGreen);
}

// ============================================================================
// 【PixelProgressBar::UpdateLowStateAnimation】
// ============================================================================
void PixelProgressBar::UpdateLowStateAnimation(float delta_seconds) {
    if (!low_state_) return;
    blink_timer_ += delta_seconds;
    const float interval = AnimationConfig::BlinkInterval;
    if (blink_timer_ >= interval) {
        blink_timer_ = 0.0f;
        blink_on_ = !blink_on_;
        if (!geometry_dirty_) {
            UpdateFillColor_();
        }
    }
}

// ============================================================================
// 【PixelProgressBar::Render】
// ============================================================================
void PixelProgressBar::Render(sf::RenderWindow& window) {
    if (geometry_dirty_) {
        RebuildGeometry_();
        geometry_dirty_ = false;
    }

    sf::RenderStates alpha_states;
    alpha_states.blendMode = sf::BlendMode();
    window.draw(bg_vertices_, alpha_states);
    window.draw(fill_vertices_, alpha_states);
}

// ============================================================================
// 【PixelProgressBar::GetFillColor_】
// ============================================================================
sf::Color PixelProgressBar::GetFillColor_() const {
    if (progress_ >= 0.95f) {
        return low_state_ && !blink_on_
            ? ColorPalette::Darken(low_color_, 30)
            : full_color_;
    }
    if (low_state_) {
        return blink_on_ ? low_color_
                         : ColorPalette::Darken(low_color_, 30);
    }
    return normal_color_;
}

void PixelProgressBar::UpdateFillColor_() {
    if (fill_vertices_.getVertexCount() == 0) return;
    const sf::Color fill_color = GetFillColor_();
    for (std::size_t i = 0; i < fill_vertices_.getVertexCount(); ++i) {
        fill_vertices_[i].color = fill_color;
    }
}

// ============================================================================
// 【PixelProgressBar::RebuildGeometry_】
// ============================================================================
void PixelProgressBar::RebuildGeometry_() {
    bg_vertices_.clear();
    fill_vertices_.clear();

    const sf::FloatRect r = rect_;
    const float fill_w = r.size.x * progress_;

    // 背景
    AddQuad(bg_vertices_,
            {r.position.x, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            bg_color_);

    // 背景描边
    const sf::Color outline = outline_color_;
    AddQuad(bg_vertices_,
            {r.position.x, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + 1.0f},
            {r.position.x, r.position.y + 1.0f},
            outline);
    AddQuad(bg_vertices_,
            {r.position.x, r.position.y + r.size.y - 1.0f},
            {r.position.x + r.size.x, r.position.y + r.size.y - 1.0f},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            outline);

    // 填充
    if (fill_w > 0.0f) {
        const sf::Color fill_color = GetFillColor_();
        AddQuad(fill_vertices_,
                {r.position.x, r.position.y},
                {r.position.x + fill_w, r.position.y},
                {r.position.x + fill_w, r.position.y + r.size.y},
                {r.position.x, r.position.y + r.size.y},
                fill_color);
    }
}

// ============================================================================
// 【PixelProgressBar::draw】
// ============================================================================
void PixelProgressBar::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    (void)states;
    target.draw(bg_vertices_);
    target.draw(fill_vertices_);
}

}  // namespace CloudSeamanor::engine
