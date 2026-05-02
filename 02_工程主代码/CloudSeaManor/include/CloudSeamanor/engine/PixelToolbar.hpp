#pragma once

// ============================================================================
// 【PixelToolbar】像素工具栏
// ============================================================================
// Responsibilities:
// - 12 格物品格
// - 选中高亮（亮绿色 2px 描边）
// - 快捷键标注（1-0, -, =）
// - 物品数量渲染
// - 滚动支持（如果物品 > 12）
// ============================================================================

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelUiConfig.hpp"
#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <array>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【ToolbarSlot】工具栏格子数据
// ============================================================================
struct ToolbarSlot {
    std::string item_id;
    std::string item_name;
    int count = 0;
    bool empty = true;
    bool highlighted = false;
};

// ============================================================================
// 【PixelToolbar】像素工具栏
// ============================================================================
class PixelToolbar : public sf::Drawable, public sf::Transformable {
public:
    // ========================================================================
    // 【构造】
    // ========================================================================
    PixelToolbar();

    // ========================================================================
    // 【配置】
    // ========================================================================

    /**
     * @brief 设置位置
     */
    void SetPosition(const sf::Vector2f& pos);
    void SetRect(const sf::FloatRect& rect) { rect_ = rect; geometry_dirty_ = true; }
    void SetColors(const sf::Color& fill, const sf::Color& border) { bg_fill_color_ = fill; bg_border_color_ = border; geometry_dirty_ = true; }

    /**
     * @brief 更新格子数据
     * @param slots 12 个格子数据
     */
    void UpdateSlots(const std::vector<ToolbarSlot>& slots);

    /**
     * @brief 设置当前选中的格子索引（0-11）
     */
    void SetSelectedSlot(int index);
    void MoveSelection(int delta);

    /**
     * @brief 获取当前选中的格子索引
     */
    [[nodiscard]] int GetSelectedSlot() const { return selected_slot_; }

    // ========================================================================
    // 【动画】
    // ========================================================================
    void FadeIn(float duration_seconds = 0.15f);
    void FadeOut(float duration_seconds = 0.10f);
    void UpdateAnimation(float delta_seconds);
    [[nodiscard]] bool IsAnimating() const { return anim_state_ != AnimationState::Idle; }

    // ========================================================================
    // 【状态】
    // ========================================================================
    [[nodiscard]] bool IsVisible() const { return visible_; }
    void SetVisible(bool visible) { visible_ = visible; }

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【属性】
    // ========================================================================
    [[nodiscard]] const sf::FloatRect& GetRect() const { return rect_; }
    [[nodiscard]] const ToolbarSlot& GetSlot(int index) const {
        return slots_[index];
    }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    void RebuildGeometry_();
    [[nodiscard]] sf::FloatRect GetSlotRect_(int slot_index) const;

    static constexpr int kSlotCount = 12;

    sf::FloatRect rect_;
    std::array<ToolbarSlot, kSlotCount> slots_;
    int selected_slot_ = 0;
    bool visible_ = true;

    // 动画
    enum class AnimationState : std::uint8_t { Idle, FadeIn, FadeOut };
    AnimationState anim_state_ = AnimationState::Idle;
    float anim_duration_ = 0.15f;
    float anim_elapsed_ = 0.0f;
    float alpha_ = 1.0f;
    float highlight_timer_ = 0.0f;

    mutable sf::VertexArray bg_vertices_;
    mutable sf::VertexArray slot_vertices_;
    mutable sf::VertexArray select_vertices_;
    mutable bool geometry_dirty_ = true;

    static constexpr std::uint8_t kCreamR = 255, kCreamG = 250, kCreamB = 240;
    static constexpr std::uint8_t kBrownR = 139, kBrownG = 90, kBrownB = 43;

    sf::Color bg_fill_color_ = sf::Color(kCreamR, kCreamG, kCreamB);
    sf::Color bg_border_color_ = sf::Color(kBrownR, kBrownG, kBrownB);

    // 快捷键标签
    static constexpr const char* kKeyLabels_[kSlotCount] = {
        "1", "2", "3", "4", "5", "6",
        "7", "8", "9", "0", "-", "="
    };
};

}  // namespace CloudSeamanor::engine
