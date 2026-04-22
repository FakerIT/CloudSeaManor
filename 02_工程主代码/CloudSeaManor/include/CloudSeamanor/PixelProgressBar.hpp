#pragma once

// ============================================================================
// 【PixelProgressBar】像素进度条
// ============================================================================
// Responsibilities:
// - 背景条 + 填充条
// - 颜色状态（满/正常/低）
// - 像素边框包裹
// ============================================================================

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelUiConfig.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PixelProgressBar】像素风格进度条
// ============================================================================
class PixelProgressBar : public sf::Drawable,
                          public sf::Transformable {
public:
    // ========================================================================
    // 【构造】
    // ========================================================================
    PixelProgressBar();
    PixelProgressBar(const sf::Vector2f& position,
                    const sf::Vector2f& size);
    void SetPosition(const sf::Vector2f& position) { rect_.position = position; geometry_dirty_ = true; }
    void SetSize(const sf::Vector2f& size) { rect_.size = size; geometry_dirty_ = true; }
    void SetColors(const sf::Color& bg,
                   const sf::Color& outline,
                   const sf::Color& normal,
                   const sf::Color& low,
                   const sf::Color& full) {
        bg_color_ = bg;
        outline_color_ = outline;
        normal_color_ = normal;
        low_color_ = low;
        full_color_ = full;
        geometry_dirty_ = true;
    }

    // ========================================================================
    // 【配置】
    // ========================================================================

    /**
     * @brief 设置进度值
     * @param ratio 0.0 ~ 1.0
     */
    void SetProgress(float ratio);

    /**
     * @brief 设置是否低体力警告模式
     * @param low true=低体力（红色闪烁）
     */
    void SetLowState(bool low);

    /**
     * @brief 设置条的方向（横向或纵向）
     */
    void SetVertical(bool vertical) { vertical_ = vertical; }

    /**
     * @brief 设置进度条标签（右侧显示）
     */
    void SetLabel(const std::string& label);
    void SetStaminaStyle();
    void SetFavorStyle();
    void SetCropStyle();
    void SetWorkshopStyle();
    void SetContractStyle();

    // ========================================================================
    // 【动画】
    // ========================================================================

    /**
     * @brief 更新低体力闪烁动画
     */
    void UpdateLowStateAnimation(float delta_seconds);

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【属性】
    // ========================================================================
    [[nodiscard]] float GetProgress() const { return progress_; }
    [[nodiscard]] bool IsLowState() const { return low_state_; }
    [[nodiscard]] const sf::FloatRect& GetRect() const { return rect_; }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    void RebuildGeometry_();
    void UpdateFillColor_();
    [[nodiscard]] sf::Color GetFillColor_() const;

    sf::FloatRect rect_;
    float progress_ = 1.0f;
    bool low_state_ = false;
    bool vertical_ = false;
    std::string label_;

    // 闪烁
    float blink_timer_ = 0.0f;
    bool blink_on_ = true;

    sf::Color bg_color_ = ColorPalette::StaminaBarBg;
    sf::Color outline_color_ = ColorPalette::StaminaBarOutline;
    sf::Color normal_color_ = ColorPalette::StaminaNormal;
    sf::Color low_color_ = ColorPalette::StaminaLow;
    sf::Color full_color_ = ColorPalette::StaminaFull;

    mutable sf::VertexArray bg_vertices_;
    mutable sf::VertexArray fill_vertices_;
    mutable bool geometry_dirty_ = true;
};

}  // namespace CloudSeamanor::engine
