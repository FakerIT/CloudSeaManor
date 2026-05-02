#pragma once

// ============================================================================
// 【PixelMinimap】像素迷你地图
// ============================================================================
// Responsibilities:
// - 简化场景轮廓
// - 玩家位置点（亮黄色）
// - 重要地点标注
// - 可选：NPC 位置
// ============================================================================

#include "CloudSeamanor/engine/PixelUiConfig.hpp"
#include "CloudSeamanor/engine/PixelArtStyle.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

class PixelFontRenderer;

// ============================================================================
// 【MapMarker】地图标记
// ============================================================================
struct MapMarker {
    std::string name;
    sf::Vector2f world_position;
    sf::Color color = {255, 200, 80};
    bool is_npc = false;
    bool is_building = false;
};

// ============================================================================
// 【PixelMinimap】像素迷你地图
// ============================================================================
class PixelMinimap : public sf::Drawable, public sf::Transformable {
public:
    // ========================================================================
    // 【构造】
    // ========================================================================
    PixelMinimap();

    // ========================================================================
    // 【配置】
    // ========================================================================

    /**
     * @brief 设置位置（默认居中）
     */
    void SetPosition(const sf::Vector2f& pos);
    void SetRect(const sf::FloatRect& rect) { rect_ = rect; geometry_dirty_ = true; }
    void SetColors(const sf::Color& fill, const sf::Color& border, const sf::Color& inner_fill) {
        panel_fill_color_ = fill;
        border_color_ = border;
        inner_fill_color_ = inner_fill;
        geometry_dirty_ = true;
    }

    /**
     * @brief 更新玩家世界坐标
     */
    void UpdatePlayerPosition(const sf::Vector2f& world_pos);

    /**
     * @brief 更新地图标记（NPC、建筑等）
     */
    void UpdateMarkers(const std::vector<MapMarker>& markers);
    void SetFontRenderer(const PixelFontRenderer* renderer) { font_renderer_ = renderer; }
    void SetLocationText(const std::string& text) { location_text_ = text; }

    /**
     * @brief 更新世界边界
     */
    void SetWorldBounds(const sf::FloatRect& world_bounds);

    // ========================================================================
    // 【动画】
    // ========================================================================
    void FadeIn(float duration_seconds = 0.15f);
    void FadeOut(float duration_seconds = 0.10f);
    void UpdateAnimation(float delta_seconds);
    [[nodiscard]] bool IsAnimating() const { return anim_state_ != AnimationState::Idle; }
    void Open() { FadeIn(); }
    void Close() { FadeOut(); }

    // ========================================================================
    // 【状态】
    // ========================================================================
    [[nodiscard]] bool IsVisible() const { return visible_; }
    void SetVisible(bool visible) { visible_ = visible; }

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);
    void MouseWheel(float /*mx*/, float /*my*/, float /*delta*/) {}

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    [[nodiscard]] sf::Vector2f WorldToMap_(const sf::Vector2f& world_pos) const;
    void RebuildGeometry_();

    sf::FloatRect rect_;
    bool visible_ = true;
    sf::Vector2f player_world_pos_{640.0f, 360.0f};
    sf::FloatRect world_bounds_{{40.0f, 40.0f}, {1200.0f, 640.0f}};
    std::vector<MapMarker> markers_;
    const PixelFontRenderer* font_renderer_ = nullptr;
    std::string location_text_;

    mutable sf::VertexArray bg_vertices_;
    mutable sf::VertexArray terrain_vertices_;
    mutable sf::VertexArray marker_vertices_;
    mutable bool geometry_dirty_ = true;

    // 动画
    enum class AnimationState : std::uint8_t { Idle, FadeIn, FadeOut };
    AnimationState anim_state_ = AnimationState::Idle;
    float anim_duration_ = 0.15f;
    float anim_elapsed_ = 0.0f;
    float alpha_ = 1.0f;

    sf::Color panel_fill_color_ = ColorPalette::DeepCream;
    sf::Color border_color_ = ColorPalette::BrownOutline;
    sf::Color inner_fill_color_ = ColorPalette::LightCream;
};

}  // namespace CloudSeamanor::engine
