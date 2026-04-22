#pragma once

// ============================================================================
// 【PixelInventoryGrid】像素背包
// ============================================================================
// Responsibilities:
// - 标签栏（物品/工具/装备/收集）
// - 物品格网格（8×4 或可滚动）
// - tooltip（鼠标悬停显示名称/描述）
// - 数量标注
// - 空格子占位符
// ============================================================================

#include "CloudSeamanor/PixelArtStyle.hpp"
#include "CloudSeamanor/PixelUiConfig.hpp"
#include "CloudSeamanor/PixelUiPanel.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【InventoryItem】物品数据
// ============================================================================
struct InventoryItem {
    std::string item_id;
    std::string item_name;
    std::string description;
    int count = 0;
    int price = 0;
    bool empty = true;
};

struct SocialNpcEntry {
    std::string npc_id;
    std::string npc_name;
    int favor = 0;
    int heart_level = 0;
    int last_gift_day = -1;
    int completed_heart_events = 0;
};

// ============================================================================
// 【InventoryTab】背包标签
// ============================================================================
enum class InventoryTab : std::uint8_t {
    Items = 0,
    Tools = 1,
    Equipment = 2,
    Collection = 3,
    Social = 4,
    Count
};

// ============================================================================
// 【PixelInventoryGrid】像素背包面板
// ============================================================================
class PixelInventoryGrid : public sf::Drawable, public sf::Transformable {
public:
    // ========================================================================
    // 【构造】
    // ========================================================================
    PixelInventoryGrid();

    // ========================================================================
    // 【动画】
    // ========================================================================
    void FadeIn(float duration_seconds = AnimationConfig::FadeInDuration);
    void FadeOut(float duration_seconds = AnimationConfig::FadeOutDuration);
    void UpdateAnimation(float delta_seconds);
    [[nodiscard]] bool IsAnimating() const { return anim_state_ != AnimationState::Idle; }
    void Open() { FadeIn(); }
    void Close() { FadeOut(); }

    // ========================================================================
    // 【配置】
    // ========================================================================

    /**
     * @brief 设置位置
     */
    void SetPosition(const sf::Vector2f& pos);
    void SetRect(const sf::FloatRect& rect) { rect_ = rect; geometry_dirty_ = true; }
    void SetColors(const sf::Color& fill, const sf::Color& border) { panel_fill_color_ = fill; border_color_ = border; geometry_dirty_ = true; }

    /**
     * @brief 更新物品数据
     * @param items 物品列表
     */
    void UpdateItems(const std::vector<InventoryItem>& items);
    void UpdateSocialEntries(const std::vector<SocialNpcEntry>& entries);
    void SetFontRenderer(const PixelFontRenderer* renderer) { font_renderer_ = renderer; }

    /**
     * @brief 设置当前标签
     */
    void SetActiveTab(InventoryTab tab);
    [[nodiscard]] InventoryTab GetActiveTab() const { return active_tab_; }

    /**
     * @brief 设置选中的格子索引
     */
    void SetSelectedSlot(int index);
    [[nodiscard]] int GetSelectedSlot() const { return selected_slot_; }

    // ========================================================================
    // 【状态】
    // ========================================================================
    [[nodiscard]] bool IsVisible() const { return visible_; }
    void SetVisible(bool visible) { visible_ = visible; }

    // ========================================================================
    // 【交互】
    // ========================================================================

    /**
     * @brief 鼠标悬停
     */
    void MouseHover(float mx, float my);

    /**
     * @brief 鼠标点击
     */
    void MouseClick(float mx, float my);

    /**
     * @brief 鼠标滚轮（预留：列表滚动等）
     */
    void MouseWheel(float /*mx*/, float /*my*/, float /*delta*/) {}

    /**
     * @brief 获取悬停的物品信息
     */
    [[nodiscard]] std::optional<InventoryItem> GetHoveredItem() const;
    [[nodiscard]] std::optional<SocialNpcEntry> GetSelectedSocialNpc() const;

    // ========================================================================
    // 【渲染】
    // ========================================================================
    void Render(sf::RenderWindow& window);

    // ========================================================================
    // 【属性】
    // ========================================================================
    [[nodiscard]] const sf::FloatRect& GetRect() const { return rect_; }

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    [[nodiscard]] sf::FloatRect GetSlotRect_(int col, int row) const;
    [[nodiscard]] sf::FloatRect GetTabRect_(int tab_index) const;
    [[nodiscard]] int GetSlotIndexAt_(float mx, float my) const;
    void RebuildGeometry_();

    sf::FloatRect rect_;
    bool visible_ = true;
    InventoryTab active_tab_ = InventoryTab::Items;
    int selected_slot_ = -1;
    int hovered_slot_ = -1;
    int hovered_social_index_ = -1;
    int hovered_tab_ = -1;
    std::vector<InventoryItem> items_;
    int selected_social_index_ = -1;
    std::vector<SocialNpcEntry> social_entries_;

    static constexpr const char* kTabLabels_[static_cast<int>(InventoryTab::Count)] = {
        "物品", "工具", "装备", "收集", "社交"
    };

    mutable sf::VertexArray panel_vertices_;
    mutable sf::VertexArray grid_vertices_;
    mutable sf::VertexArray highlight_vertices_;
    mutable sf::VertexArray tooltip_vertices_;
    mutable bool geometry_dirty_ = true;
    mutable bool tooltip_dirty_ = true;
    const PixelFontRenderer* font_renderer_ = nullptr;
    sf::Vector2f tooltip_mouse_pos_{0.0f, 0.0f};
    float tooltip_hover_elapsed_ = 0.0f;

    // 动画
    enum class AnimationState : std::uint8_t { Idle, FadeIn, FadeOut };
    AnimationState anim_state_ = AnimationState::Idle;
    float anim_duration_ = 0.15f;
    float anim_elapsed_ = 0.0f;
    float alpha_ = 1.0f;

    sf::Color panel_fill_color_ = ColorPalette::Cream;
    sf::Color border_color_ = ColorPalette::BrownOutline;
};

}  // namespace CloudSeamanor::engine
