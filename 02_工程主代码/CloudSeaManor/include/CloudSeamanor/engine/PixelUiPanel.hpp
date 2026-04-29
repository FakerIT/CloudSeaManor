#pragma once

// ============================================================================
// 【PixelUiPanel】像素面板基类
// ============================================================================
// Responsibilities:
// - 像素边框 + 米色填充背景
// - 标题栏（可选）
// - 阴影效果
// - 打开/关闭动画（淡入淡出）
// - 可见性控制
// ============================================================================

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"
#include "CloudSeamanor/engine/PixelUiConfig.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PixelUiPanel】像素风格面板基类
// ============================================================================
class PixelUiPanel : public sf::Drawable, public sf::Transformable {
public:
    // ========================================================================
    // 【构造/析构】
    // ========================================================================
    PixelUiPanel();
    explicit PixelUiPanel(const sf::FloatRect& rect,
                          const std::string& title = {},
                          bool has_title_bar = false);

    // ========================================================================
    // 【配置】
    // ========================================================================

    /**
     * @brief 设置面板矩形
     */
    void SetRect(const sf::FloatRect& rect);

    /**
     * @brief 设置标题文本
     */
    void SetTitle(const std::string& title);

    /**
     * @brief 是否显示标题栏
     */
    void SetHasTitleBar(bool value) { has_title_bar_ = value; }
    void SetColors(const sf::Color& fill, const sf::Color& border) {
        auto& s = art_style_.MutableStyle();
        s.fill_color = fill;
        s.outline_color = border;
        geometry_dirty_ = true;
    }

    /**
     * @brief 设置是否显示
     */
    void SetVisible(bool visible) { visible_ = visible; }
    [[nodiscard]] bool IsVisible() const { return visible_; }
    void SetAnimationDuration(float fade_in_seconds, float fade_out_seconds);

    // ========================================================================
    // 【动画】
    // ========================================================================

    /**
     * @brief 淡入动画（打开面板时调用）
     */
    void FadeIn();
    void FadeIn(float duration_seconds);

    /**
     * @brief 淡出动画（关闭面板时调用）
     */
    void FadeOut();
    void FadeOut(float duration_seconds);

    /**
     * @brief 每帧更新动画
     * @param delta_seconds 帧时间
     */
    void UpdateAnimation(float delta_seconds);

    /**
     * @brief 是否正在动画中
     */
    [[nodiscard]] bool IsAnimating() const {
        return anim_state_ != AnimationState::Idle;
    }

    // ========================================================================
    // 【状态】
    // ========================================================================
    [[nodiscard]] bool IsOpen() const { return is_open_; }
    void Open();
    void Close();
    void Toggle();

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【拖动（UI-129）】
    // ========================================================================
    // 仅在标题栏区域按下左键时开始拖动（has_title_bar_ == true）
    // Mouse coords are expected to be in the same view space as rendering (1280x720 logical).
    [[nodiscard]] bool OnMousePressed(float mx, float my);
    void OnMouseMoved(float mx, float my);
    void OnMouseReleased();
    [[nodiscard]] bool IsDragging() const { return is_dragging_; }

    // ========================================================================
    // 【属性】
    // ========================================================================
    [[nodiscard]] const sf::FloatRect& GetRect() const { return rect_; }
    [[nodiscard]] sf::FloatRect GetWorldRect() const {
        const auto t = getTransform();
        return sf::FloatRect(t.transformPoint(rect_.position), rect_.size);
    }
    [[nodiscard]] float GetAlpha() const { return alpha_; }
    [[nodiscard]] const PixelArtStyle& GetArtStyle() const { return art_style_; }

protected:
    // ========================================================================
    // 【子类重写】
    // ========================================================================

    /**
     * @brief 子类实现自定义渲染内容（在 Panel Draw 时调用）
     * @param window 渲染目标
     * @param inner_rect 去除边框后的内容区域
     */
    virtual void RenderContent(sf::RenderWindow& window, const sf::FloatRect& inner_rect) {
        (void)window;
        (void)inner_rect;
    }

    // ========================================================================
    // 【内部绘制（受保护，子类可调用）】
    // ========================================================================
    void DrawPixelBorder(sf::VertexArray& va, const sf::FloatRect& r) const;
    void DrawPixelPanel(sf::VertexArray& va, const sf::FloatRect& r) const;

    // ========================================================================
    // 【sf::Drawable 实现】
    // ========================================================================
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    void RebuildGeometry_();

    sf::FloatRect rect_;
    std::string title_;
    bool has_title_bar_ = false;
    bool visible_ = true;
    bool is_open_ = false;
    PixelArtStyle art_style_;

    // 拖动状态（标题栏拖动）
    bool is_dragging_ = false;
    sf::Vector2f drag_offset_{0.0f, 0.0f};

    // 动画
    enum class AnimationState : std::uint8_t { Idle, FadeIn, FadeOut };
    AnimationState anim_state_ = AnimationState::Idle;
    float anim_duration_ = 0.15f;
    float anim_elapsed_ = 0.0f;
    float alpha_ = 1.0f;
    float fade_in_duration_ = AnimationConfig::FadeInDuration;
    float fade_out_duration_ = AnimationConfig::FadeOutDuration;

    // 几何缓存
    mutable sf::VertexArray vertices_;
    mutable bool geometry_dirty_ = true;
};

}  // namespace CloudSeamanor::engine
