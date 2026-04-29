#pragma once

// ============================================================================
// 【PixelQuestMenu】像素任务面板
// ============================================================================
// Responsibilities:
// - 任务列表
// - 每条任务标题 + 状态（完成/进行中）
// - 奖励显示
// - 折叠/展开详情
// ============================================================================

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelProgressBar.hpp"
#include "CloudSeamanor/engine/PixelUiConfig.hpp"
#include "CloudSeamanor/engine/PixelUiPanel.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>

namespace CloudSeamanor::engine {

class PixelFontRenderer;

// ============================================================================
// 【QuestStatus】任务状态
// ============================================================================
enum class QuestStatus : std::uint8_t {
    Available,   // 可接取
    Active,      // 进行中
    Completed,   // 已完成
    Failed       // 已失败
};

enum class QuestTab : std::uint8_t {
    Commissions = 0,
    Contracts = 1,
    Story = 2,
    Count
};

// ============================================================================
// 【Quest】任务数据
// ============================================================================
struct Quest {
    std::string id;
    std::string title;
    std::string description;
    std::string objective;
    std::string reward_text;
    QuestStatus status = QuestStatus::Available;
    int progress_current = 0;
    int progress_max = 0;
    bool expanded = false;
};

// ============================================================================
// 【PixelQuestMenu】像素任务面板
// ============================================================================
class PixelQuestMenu : public sf::Drawable, public sf::Transformable {
public:
    // ========================================================================
    // 【构造】
    // ========================================================================
    PixelQuestMenu();

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
    void SetSemanticColors(const sf::Color& button_hover,
                           const sf::Color& button_selected,
                           const sf::Color& button_default,
                           const sf::Color& status_completed,
                           const sf::Color& status_active,
                           const sf::Color& status_default) {
        button_hover_color_ = button_hover;
        button_selected_color_ = button_selected;
        button_default_color_ = button_default;
        status_completed_color_ = status_completed;
        status_active_color_ = status_active;
        status_default_color_ = status_default;
        geometry_dirty_ = true;
    }

    /**
     * @brief 更新任务列表
     */
    void UpdateQuests(const std::vector<Quest>& quests);
    void SetFontRenderer(const PixelFontRenderer* renderer) { font_renderer_ = renderer; }

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
     * @brief 鼠标点击（展开/折叠任务）
     */
    void MouseClick(float mx, float my);

    void MouseWheel(float mx, float my, float delta);

    /**
     * @brief 键盘滚动
     */
    void Scroll(int delta);

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
    [[nodiscard]] int GetRowAt_(float my) const;
    [[nodiscard]] int GetTabAt_(float mx, float my) const;
    void RebuildGeometry_();

    sf::FloatRect rect_;
    bool visible_ = true;
    std::vector<Quest> quests_;
    int hovered_row_ = -1;
    int scroll_offset_ = 0;
    QuestTab active_tab_ = QuestTab::Commissions;
    int hovered_tab_ = -1;

    static constexpr int kVisibleRows_ = 10;

    mutable sf::VertexArray panel_vertices_;
    mutable sf::VertexArray list_vertices_;
    mutable bool geometry_dirty_ = true;
    const PixelFontRenderer* font_renderer_ = nullptr;
    PixelProgressBar progress_bar_;

    // 动画
    enum class AnimationState : std::uint8_t { Idle, FadeIn, FadeOut };
    AnimationState anim_state_ = AnimationState::Idle;
    float anim_duration_ = 0.15f;
    float anim_elapsed_ = 0.0f;
    float alpha_ = 1.0f;

    sf::Color panel_fill_color_ = ColorPalette::Cream;
    sf::Color border_color_ = ColorPalette::BrownOutline;
    sf::Color button_hover_color_ = ColorPalette::HighlightYellow;
    sf::Color button_selected_color_ = ColorPalette::ActiveGreen;
    sf::Color button_default_color_ = ColorPalette::BackgroundWhite;
    sf::Color status_completed_color_ = ColorPalette::SuccessGreen;
    sf::Color status_active_color_ = ColorPalette::ActiveGreen;
    sf::Color status_default_color_ = ColorPalette::LightGray;
};

}  // namespace CloudSeamanor::engine
