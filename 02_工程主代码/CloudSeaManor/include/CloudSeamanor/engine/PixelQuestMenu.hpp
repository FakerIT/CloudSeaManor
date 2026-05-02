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
#include "CloudSeamanor/engine/PixelFontRenderer.hpp"
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

enum class QuestStatus : std::uint8_t {
    Available,
    Active,
    Completed,
    Failed
};

enum class QuestTab : std::uint8_t {
    Commissions = 0,
    Contracts = 1,
    Story = 2,
    Count
};

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

class PixelQuestMenu : public sf::Drawable, public sf::Transformable {
public:
    PixelQuestMenu();

    void FadeIn(float duration_seconds = 0.15f);
    void FadeOut(float duration_seconds = 0.10f);
    void UpdateAnimation(float delta_seconds);
    [[nodiscard]] bool IsAnimating() const { return anim_state_ != AnimationState::Idle; }
    void Open() { FadeIn(); }
    void Close() { FadeOut(); }

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

    void UpdateQuests(const std::vector<Quest>& quests);
    void SetFontRenderer(const PixelFontRenderer* renderer) { font_renderer_ = renderer; }

    [[nodiscard]] bool IsVisible() const { return visible_; }
    void SetVisible(bool visible) { visible_ = visible; }

    void MouseHover(float mx, float my);
    void MouseClick(float mx, float my);
    void MouseWheel(float mx, float my, float delta);
    void Scroll(int delta);

    void Render(sf::RenderWindow& window);

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
    ::CloudSeamanor::engine::PixelProgressBar progress_bar_;

    enum class AnimationState : std::uint8_t { Idle, FadeIn, FadeOut };
    AnimationState anim_state_ = AnimationState::Idle;
    float anim_duration_ = 0.15f;
    float anim_elapsed_ = 0.0f;
    float alpha_ = 1.0f;

    static constexpr std::uint8_t kCreamR = 255, kCreamG = 250, kCreamB = 240;
    static constexpr std::uint8_t kBrownR = 139, kBrownG = 90, kBrownB = 43;
    static constexpr std::uint8_t kYellowR = 255, kYellowG = 255, kYellowB = 0;
    static constexpr std::uint8_t kGreenR = 50, kGreenG = 205, kGreenB = 50;
    static constexpr std::uint8_t kWhiteR = 255, kWhiteG = 255, kWhiteB = 255;
    static constexpr std::uint8_t kGrayR = 192, kGrayG = 192, kGrayB = 192;

    sf::Color panel_fill_color_ = sf::Color(kCreamR, kCreamG, kCreamB);
    sf::Color border_color_ = sf::Color(kBrownR, kBrownG, kBrownB);
    sf::Color button_hover_color_ = sf::Color(kYellowR, kYellowG, kYellowB);
    sf::Color button_selected_color_ = sf::Color(kGreenR, kGreenG, kGreenB);
    sf::Color button_default_color_ = sf::Color(kWhiteR, kWhiteG, kWhiteB);
    sf::Color status_completed_color_ = sf::Color(kGreenR, kGreenG, kGreenB);
    sf::Color status_active_color_ = sf::Color(kGreenR, kGreenG, kGreenB);
    sf::Color status_default_color_ = sf::Color(kGrayR, kGrayG, kGrayB);
};

}  // namespace CloudSeamanor::engine
