#include "CloudSeamanor/engine/PixelUiPanel.hpp"
#include "CloudSeamanor/engine/UiVertexHelpers.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::engine::uivx::AddQuad;
}  // namespace

PixelUiPanel::PixelUiPanel()
    : rect_{{0.0f, 0.0f}, {UiPanelConfig::DefaultWidth, UiPanelConfig::DefaultHeight}},
      vertices_(sf::PrimitiveType::Triangles) {
}

PixelUiPanel::PixelUiPanel(const sf::FloatRect& rect,
                           const std::string& title,
                           bool has_title_bar)
    : rect_(rect),
      title_(title),
      has_title_bar_(has_title_bar),
      vertices_(sf::PrimitiveType::Triangles) {
    RebuildGeometry_();
}

void PixelUiPanel::SetRect(const sf::FloatRect& rect) {
    rect_ = rect;
    is_dragging_ = false;
    geometry_dirty_ = true;
}

void PixelUiPanel::SetTitle(const std::string& title) {
    title_ = title;
    geometry_dirty_ = true;
}

void PixelUiPanel::SetAnimationDuration(float fade_in_seconds, float fade_out_seconds) {
    fade_in_duration_ = std::max(0.01f, fade_in_seconds);
    fade_out_duration_ = std::max(0.01f, fade_out_seconds);
}

void PixelUiPanel::FadeIn() {
    FadeIn(fade_in_duration_);
}

void PixelUiPanel::FadeIn(float duration_seconds) {
    anim_state_ = AnimationState::FadeIn;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
    visible_ = true;
    is_open_ = true;
}

void PixelUiPanel::FadeOut() {
    FadeOut(fade_out_duration_);
}

void PixelUiPanel::FadeOut(float duration_seconds) {
    anim_state_ = AnimationState::FadeOut;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
    is_open_ = false;
}

void PixelUiPanel::UpdateAnimation(float delta_seconds) {
    if (anim_state_ == AnimationState::Idle) return;

    anim_elapsed_ += delta_seconds;
    const float t = std::clamp(anim_elapsed_ / anim_duration_, 0.0f, 1.0f);

    if (anim_state_ == AnimationState::FadeIn) {
        alpha_ = t;
        if (t >= 1.0f) {
            alpha_ = 1.0f;
            anim_state_ = AnimationState::Idle;
        }
    } else if (anim_state_ == AnimationState::FadeOut) {
        alpha_ = 1.0f - t;
        if (t >= 1.0f) {
            alpha_ = 0.0f;
            anim_state_ = AnimationState::Idle;
            visible_ = false;
        }
    }
    geometry_dirty_ = true;
}

void PixelUiPanel::Open() {
    FadeIn();
}

void PixelUiPanel::Close() {
    FadeOut();
}

void PixelUiPanel::Toggle() {
    if (IsOpen() || IsAnimating()) {
        Close();
    } else {
        Open();
    }
}

bool PixelUiPanel::OnMousePressed(float mx, float my) {
    if (!visible_ || alpha_ <= 0.0f) return false;
    if (!has_title_bar_) return false;

    const sf::FloatRect wr = GetWorldRect();
    const sf::FloatRect title_bar{
        {wr.position.x, wr.position.y},
        {wr.size.x, PixelBorderConfig::TitleBarHeight}
    };
    if (!title_bar.contains({mx, my})) return false;

    is_dragging_ = true;
    drag_offset_ = {mx - wr.position.x, my - wr.position.y};
    return true;
}

void PixelUiPanel::OnMouseMoved(float mx, float my) {
    if (!is_dragging_) return;

    const float new_x = mx - drag_offset_.x;
    const float new_y = my - drag_offset_.y;

    const float max_x = ScreenConfig::Width - rect_.size.x;
    const float max_y = ScreenConfig::Height - rect_.size.y;
    rect_.position.x = std::clamp(new_x, 0.0f, std::max(0.0f, max_x));
    rect_.position.y = std::clamp(new_y, 0.0f, std::max(0.0f, max_y));
    geometry_dirty_ = true;
}

void PixelUiPanel::OnMouseReleased() {
    is_dragging_ = false;
}

void PixelUiPanel::Render(sf::RenderWindow& window) {
    if (!visible_ || alpha_ <= 0.0f) return;

    if (geometry_dirty_) {
        RebuildGeometry_();
        geometry_dirty_ = false;
    }

    sf::RenderStates states(getTransform());
    states.blendMode = sf::BlendMode();
    window.draw(vertices_, states);

    const float inner_offset = PixelBorderConfig::BorderThickness;
    const float title_h = has_title_bar_ ? PixelBorderConfig::TitleBarHeight : 0.0f;
    const sf::FloatRect inner{
        {rect_.position.x + inner_offset,
         rect_.position.y + inner_offset + title_h},
        {rect_.size.x - inner_offset * 2.0f,
         rect_.size.y - inner_offset * 2.0f - title_h}
    };
    RenderContent(window, inner);
}

void PixelUiPanel::RebuildGeometry_() {
    vertices_.clear();

    const sf::FloatRect r = rect_;
    const auto& style = art_style_.GetStyle();
    const float cs = PixelBorderConfig::CornerBlockSize;
    const float t = PixelBorderConfig::BorderThickness;

    const float l = r.position.x;
    const float r_x = r.position.x + r.size.x;
    const float t_y = r.position.y;
    const float b_y = r.position.y + r.size.y;

    AddQuad(vertices_,
            {l, t_y}, {r_x, t_y}, {r_x, b_y}, {l, b_y},
            style.fill_color, alpha_);

    AddQuad(vertices_, {l, t_y}, {l + cs, t_y}, {l + cs, t_y + cs}, {l, t_y + cs},
            style.outline_color, alpha_);
    AddQuad(vertices_, {r_x - cs, t_y}, {r_x, t_y}, {r_x, t_y + cs}, {r_x - cs, t_y + cs},
            style.outline_color, alpha_);
    AddQuad(vertices_, {l, b_y - cs}, {l + cs, b_y - cs}, {l + cs, b_y}, {l, b_y},
            style.outline_color, alpha_);
    AddQuad(vertices_, {r_x - cs, b_y - cs}, {r_x, b_y - cs}, {r_x, b_y}, {r_x - cs, b_y},
            style.outline_color, alpha_);

    AddQuad(vertices_, {l + cs, t_y}, {r_x - cs, t_y}, {r_x - cs, t_y + t}, {l + cs, t_y + t},
            style.outline_color, alpha_);
    AddQuad(vertices_, {l + cs, b_y - t}, {r_x - cs, b_y - t}, {r_x - cs, b_y}, {l + cs, b_y},
            style.outline_color, alpha_);
    AddQuad(vertices_, {l, t_y + cs}, {l + t, t_y + cs}, {l + t, b_y - cs}, {l, b_y - cs},
            style.outline_color, alpha_);
    AddQuad(vertices_, {r_x - t, t_y + cs}, {r_x, t_y + cs}, {r_x, b_y - cs}, {r_x - t, b_y - cs},
            style.outline_color, alpha_);

    const sf::Color shadow = ColorPalette::Darken(style.fill_color, 15);
    AddQuad(vertices_, {l + cs, t_y + t}, {r_x - cs, t_y + t}, {r_x - cs, t_y + t + PixelBorderConfig::InnerShadowThickness}, {l + cs, t_y + t + PixelBorderConfig::InnerShadowThickness},
            shadow, alpha_);
    AddQuad(vertices_, {l + t, t_y + cs}, {l + t + PixelBorderConfig::InnerShadowThickness, t_y + cs}, {l + t + PixelBorderConfig::InnerShadowThickness, b_y - cs}, {l + t, b_y - cs},
            shadow, alpha_);

    if (has_title_bar_) {
        const float th = PixelBorderConfig::TitleBarHeight;
        const sf::Color title_bg = ColorPalette::TitleBarLight;
        AddQuad(vertices_,
                {l + 1.0f, t_y + 1.0f},
                {r_x - 1.0f, t_y + 1.0f},
                {r_x - 1.0f, t_y + th},
                {l + 1.0f, t_y + th},
                title_bg, alpha_);

        AddQuad(vertices_,
                {l + 1.0f, t_y + th - 1.0f},
                {r_x - 1.0f, t_y + th - 1.0f},
                {r_x - 1.0f, t_y + th},
                {l + 1.0f, t_y + th},
                ColorPalette::Darken(title_bg, 20), alpha_);
    }
}

void PixelUiPanel::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!visible_ || alpha_ <= 0.0f) return;
    target.draw(vertices_, states);
}

}  // namespace CloudSeamanor::engine
