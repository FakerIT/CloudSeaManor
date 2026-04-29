#include "CloudSeamanor/engine/PixelMinimap.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"
#include "CloudSeamanor/engine/UiVertexHelpers.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::engine::uivx::AddQuad;
}  // namespace

// ============================================================================
// 【PixelMinimap::PixelMinimap】
// ============================================================================
PixelMinimap::PixelMinimap()
    : rect_{MinimapConfig::Position, MinimapConfig::Size},
      bg_vertices_(sf::PrimitiveType::Triangles),
      terrain_vertices_(sf::PrimitiveType::Triangles),
      marker_vertices_(sf::PrimitiveType::Triangles) {
    RebuildGeometry_();
}

// ============================================================================
// 【PixelMinimap::FadeIn】
// ============================================================================
void PixelMinimap::FadeIn(float duration_seconds) {
    anim_state_ = AnimationState::FadeIn;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
    visible_ = true;
}

// ============================================================================
// 【PixelMinimap::FadeOut】
// ============================================================================
void PixelMinimap::FadeOut(float duration_seconds) {
    anim_state_ = AnimationState::FadeOut;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
}

// ============================================================================
// 【PixelMinimap::UpdateAnimation】
// ============================================================================
void PixelMinimap::UpdateAnimation(float delta_seconds) {
    if (anim_state_ == AnimationState::Idle) return;
    anim_elapsed_ += delta_seconds;
    const float t = std::clamp(anim_elapsed_ / anim_duration_, 0.0f, 1.0f);
    if (anim_state_ == AnimationState::FadeIn) {
        alpha_ = t;
        if (t >= 1.0f) { alpha_ = 1.0f; anim_state_ = AnimationState::Idle; }
    } else if (anim_state_ == AnimationState::FadeOut) {
        alpha_ = 1.0f - t;
        if (t >= 1.0f) { alpha_ = 0.0f; anim_state_ = AnimationState::Idle; visible_ = false; }
    }
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelMinimap::SetPosition】
// ============================================================================
void PixelMinimap::SetPosition(const sf::Vector2f& pos) {
    rect_.position = pos;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelMinimap::UpdatePlayerPosition】
// ============================================================================
void PixelMinimap::UpdatePlayerPosition(const sf::Vector2f& world_pos) {
    if (player_world_pos_ == world_pos) return;
    player_world_pos_ = world_pos;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelMinimap::UpdateMarkers】
// ============================================================================
void PixelMinimap::UpdateMarkers(const std::vector<MapMarker>& markers) {
    markers_ = markers;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelMinimap::SetWorldBounds】
// ============================================================================
void PixelMinimap::SetWorldBounds(const sf::FloatRect& world_bounds) {
    world_bounds_ = world_bounds;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelMinimap::Render】
// ============================================================================
void PixelMinimap::Render(sf::RenderWindow& window) {
    if (!visible_) return;

    if (geometry_dirty_) {
        RebuildGeometry_();
        geometry_dirty_ = false;
    }

    sf::RenderStates alpha_states;
    alpha_states.blendMode = sf::BlendMode();
    window.draw(bg_vertices_, alpha_states);
    window.draw(terrain_vertices_, alpha_states);
    window.draw(marker_vertices_, alpha_states);

    if (font_renderer_ != nullptr && font_renderer_->IsLoaded() && !location_text_.empty()) {
        TextStyle s = TextStyle::Default();
        s.character_size = 12;
        s.fill_color = ColorPalette::TextBrown;
        font_renderer_->DrawText(window,
                                 location_text_,
                                 {rect_.position.x + 14.0f, rect_.position.y + rect_.size.y - 22.0f},
                                 s);
    }
}

// ============================================================================
// 【PixelMinimap::WorldToMap_】
// ============================================================================
sf::Vector2f PixelMinimap::WorldToMap_(const sf::Vector2f& world_pos) const {
    const float world_w = world_bounds_.size.x;
    const float world_h = world_bounds_.size.y;
    const float map_w = rect_.size.x - 16.0f;  // 留边距
    const float map_h = rect_.size.y - 16.0f;

    const float rel_x = (world_pos.x - world_bounds_.position.x) / world_w;
    const float rel_y = (world_pos.y - world_bounds_.position.y) / world_h;

    return {
        rect_.position.x + 8.0f + rel_x * map_w,
        rect_.position.y + 8.0f + rel_y * map_h
    };
}

// ============================================================================
// 【PixelMinimap::RebuildGeometry_】
// ============================================================================
void PixelMinimap::RebuildGeometry_() {
    bg_vertices_.clear();
    terrain_vertices_.clear();
    marker_vertices_.clear();

    const sf::FloatRect r = rect_;
    const float cs = 8.0f;
    const float t = 1.0f;
    const sf::Color& oc = border_color_;

    // 背景
    AddQuad(bg_vertices_,
            {r.position.x, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            panel_fill_color_, alpha_);

    // 像素边框
    AddQuad(bg_vertices_, {r.position.x, r.position.y}, {r.position.x + cs, r.position.y}, {r.position.x + cs, r.position.y + cs}, {r.position.x, r.position.y + cs}, oc, alpha_);
    AddQuad(bg_vertices_, {r.position.x + r.size.x - cs, r.position.y}, {r.position.x + r.size.x, r.position.y}, {r.position.x + r.size.x, r.position.y + cs}, {r.position.x + r.size.x - cs, r.position.y + cs}, oc, alpha_);
    AddQuad(bg_vertices_, {r.position.x, r.position.y + r.size.y - cs}, {r.position.x + cs, r.position.y + r.size.y - cs}, {r.position.x + cs, r.position.y + r.size.y}, {r.position.x, r.position.y + r.size.y}, oc, alpha_);
    AddQuad(bg_vertices_, {r.position.x + r.size.x - cs, r.position.y + r.size.y - cs}, {r.position.x + r.size.x, r.position.y + r.size.y - cs}, {r.position.x + r.size.x, r.position.y + r.size.y}, {r.position.x + r.size.x - cs, r.position.y + r.size.y}, oc, alpha_);

    AddQuad(bg_vertices_, {r.position.x + cs, r.position.y}, {r.position.x + r.size.x - cs, r.position.y}, {r.position.x + r.size.x - cs, r.position.y + t}, {r.position.x + cs, r.position.y + t}, oc, alpha_);
    AddQuad(bg_vertices_, {r.position.x + cs, r.position.y + r.size.y - t}, {r.position.x + r.size.x - cs, r.position.y + r.size.y - t}, {r.position.x + r.size.x - cs, r.position.y + r.size.y}, {r.position.x + cs, r.position.y + r.size.y}, oc, alpha_);
    AddQuad(bg_vertices_, {r.position.x, r.position.y + cs}, {r.position.x + t, r.position.y + cs}, {r.position.x + t, r.position.y + r.size.y - cs}, {r.position.x, r.position.y + r.size.y - cs}, oc, alpha_);
    AddQuad(bg_vertices_, {r.position.x + r.size.x - t, r.position.y + cs}, {r.position.x + r.size.x, r.position.y + cs}, {r.position.x + r.size.x, r.position.y + r.size.y - cs}, {r.position.x + r.size.x - t, r.position.y + r.size.y - cs}, oc, alpha_);

    // 地图区域（内部留白底）
    AddQuad(terrain_vertices_,
            {r.position.x + 8.0f, r.position.y + 8.0f},
            {r.position.x + r.size.x - 8.0f, r.position.y + 8.0f},
            {r.position.x + r.size.x - 8.0f, r.position.y + r.size.y - 8.0f},
            {r.position.x + 8.0f, r.position.y + r.size.y - 8.0f},
            inner_fill_color_, alpha_);

    // 玩家位置点（亮黄色）
    const sf::Vector2f player_map_pos = WorldToMap_(player_world_pos_);
    const float dot_r = MinimapConfig::PlayerDotRadius;
    const float clamped_x = std::clamp(player_map_pos.x,
                                       r.position.x + cs + dot_r,
                                       r.position.x + r.size.x - cs - dot_r);
    const float clamped_y = std::clamp(player_map_pos.y,
                                       r.position.y + cs + dot_r,
                                       r.position.y + r.size.y - cs - dot_r);
    AddQuad(marker_vertices_,
            {clamped_x - dot_r, clamped_y - dot_r},
            {clamped_x + dot_r, clamped_y - dot_r},
            {clamped_x + dot_r, clamped_y + dot_r},
            {clamped_x - dot_r, clamped_y + dot_r},
            sf::Color{255, 240, 80}, alpha_);  // 亮黄色

    // 地图标记
    for (const auto& marker : markers_) {
        const sf::Vector2f map_pos = WorldToMap_(marker.world_position);
        const float m_dot = marker.is_npc ? 3.0f : 4.0f;
        AddQuad(marker_vertices_,
                {map_pos.x - m_dot, map_pos.y - m_dot},
                {map_pos.x + m_dot, map_pos.y - m_dot},
                {map_pos.x + m_dot, map_pos.y + m_dot},
                {map_pos.x - m_dot, map_pos.y + m_dot},
                marker.color, alpha_);
    }
}

// ============================================================================
// 【PixelMinimap::draw】
// ============================================================================
void PixelMinimap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    (void)states;
    target.draw(bg_vertices_);
    target.draw(terrain_vertices_);
    target.draw(marker_vertices_);
}

}  // namespace CloudSeamanor::engine
