#include "CloudSeamanor/engine/PixelQuestMenu.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"
#include "CloudSeamanor/engine/UiVertexHelpers.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::engine::uivx::AddQuad;
}  // namespace

// ============================================================================
// 【PixelQuestMenu::PixelQuestMenu】
// ============================================================================
PixelQuestMenu::PixelQuestMenu()
    : rect_{QuestMenuConfig::Position, QuestMenuConfig::Size},
      panel_vertices_(sf::PrimitiveType::Triangles),
      list_vertices_(sf::PrimitiveType::Triangles) {
    progress_bar_.SetContractStyle();
    RebuildGeometry_();
}

// ============================================================================
// 【PixelQuestMenu::SetPosition】
// ============================================================================
void PixelQuestMenu::SetPosition(const sf::Vector2f& pos) {
    rect_.position = pos;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelQuestMenu::UpdateQuests】
// ============================================================================
void PixelQuestMenu::UpdateQuests(const std::vector<Quest>& quests) {
    quests_ = quests;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelQuestMenu::FadeIn】
// ============================================================================
void PixelQuestMenu::FadeIn(float duration_seconds) {
    anim_state_ = AnimationState::FadeIn;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
    visible_ = true;
}

// ============================================================================
// 【PixelQuestMenu::FadeOut】
// ============================================================================
void PixelQuestMenu::FadeOut(float duration_seconds) {
    anim_state_ = AnimationState::FadeOut;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
}

// ============================================================================
// 【PixelQuestMenu::UpdateAnimation】
// ============================================================================
void PixelQuestMenu::UpdateAnimation(float delta_seconds) {
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
// 【PixelQuestMenu::MouseHover】
// ============================================================================
void PixelQuestMenu::MouseHover(float mx, float my) {
    const sf::FloatRect r = rect_;
    if (mx < r.position.x || mx > r.position.x + r.size.x) {
        if (hovered_row_ != -1) {
            hovered_row_ = -1;
            geometry_dirty_ = true;
        }
        return;
    }
    const int tab = GetTabAt_(mx, my);
    if (hovered_tab_ != tab) {
        hovered_tab_ = tab;
        geometry_dirty_ = true;
    }
    const int row = GetRowAt_(my);
    if (hovered_row_ != row) {
        hovered_row_ = row;
        geometry_dirty_ = true;
    }
}

// ============================================================================
// 【PixelQuestMenu::MouseClick】
// ============================================================================
void PixelQuestMenu::MouseClick(float mx, float my) {
    const sf::FloatRect r = rect_;
    if (mx < r.position.x || mx > r.position.x + r.size.x) return;
    if (const int tab = GetTabAt_(mx, my); tab >= 0) {
        active_tab_ = static_cast<QuestTab>(tab);
        scroll_offset_ = 0;
        hovered_row_ = -1;
        geometry_dirty_ = true;
        return;
    }
    const int row = GetRowAt_(my);
    if (row >= 0 && row < static_cast<int>(quests_.size())) {
        quests_[static_cast<std::size_t>(row)].expanded = !quests_[static_cast<std::size_t>(row)].expanded;
        geometry_dirty_ = true;
    }
}

void PixelQuestMenu::MouseWheel(float mx, float my, float delta) {
    if (!visible_) return;
    if (!rect_.contains({mx, my})) return;
    const int step = (delta > 0.0f) ? -1 : 1;
    Scroll(step);
}

// ============================================================================
// 【PixelQuestMenu::Scroll】
// ============================================================================
void PixelQuestMenu::Scroll(int delta) {
    const auto in_tab = [this](const Quest& q) -> bool {
        if (active_tab_ == QuestTab::Commissions) {
            return q.id.find("contract") == std::string::npos && q.id.find("cloud") == std::string::npos;
        }
        if (active_tab_ == QuestTab::Contracts) {
            return q.id.find("contract") != std::string::npos || q.id.find("cloud") != std::string::npos;
        }
        return q.id.find("main") != std::string::npos || q.id.find("plot") != std::string::npos;
    };
    int visible_count = 0;
    for (const auto& q : quests_) {
        if (in_tab(q)) ++visible_count;
    }
    const int max_offset = std::max(0, visible_count - kVisibleRows_);
    scroll_offset_ = std::clamp(scroll_offset_ + delta, 0, max_offset);
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelQuestMenu::Render】
// ============================================================================
void PixelQuestMenu::Render(sf::RenderWindow& window) {
    if (!visible_) return;

    if (geometry_dirty_) {
        RebuildGeometry_();
        geometry_dirty_ = false;
    }

    sf::RenderStates alpha_states;
    alpha_states.blendMode = sf::BlendMode();
    window.draw(panel_vertices_, alpha_states);
    window.draw(list_vertices_, alpha_states);

    if (font_renderer_ == nullptr || !font_renderer_->IsLoaded()) return;

    const sf::FloatRect r = rect_;
    const float p = QuestMenuConfig::TextPadding;
    const float th = QuestMenuConfig::TitleBarHeight;
    const float tab_h = 40.0f;
    float cursor_y = r.position.y + th + tab_h + QuestMenuConfig::ListTopMargin;
    const float max_y = r.position.y + r.size.y - p;

    // Tabs
    {
        constexpr const char* kTabs[] = {"委托", "契约", "主线"};
        const float tab_w = 72.0f;
        for (int i = 0; i < static_cast<int>(QuestTab::Count); ++i) {
            const float tx = r.position.x + p + static_cast<float>(i) * tab_w;
            TextStyle t = TextStyle::Default();
            t.character_size = 14;
            t.bold = (active_tab_ == static_cast<QuestTab>(i));
            t.fill_color = (hovered_tab_ == i) ? ColorPalette::DeepBrown : ColorPalette::TextBrown;
            font_renderer_->DrawText(window, kTabs[i], {tx + 8.0f, r.position.y + th + 10.0f}, t);
            if (active_tab_ == static_cast<QuestTab>(i)) {
                sf::VertexArray underline(sf::PrimitiveType::Triangles);
                AddQuad(underline,
                        {tx, r.position.y + th + tab_h - 3.0f},
                        {tx + tab_w - 8.0f, r.position.y + th + tab_h - 3.0f},
                        {tx + tab_w - 8.0f, r.position.y + th + tab_h},
                        {tx, r.position.y + th + tab_h},
                        ColorPalette::ActiveGreen, alpha_);
                window.draw(underline, alpha_states);
            }
        }
    }

    const auto in_tab = [this](const Quest& q) -> bool {
        if (active_tab_ == QuestTab::Commissions) {
            return q.id.find("contract") == std::string::npos && q.id.find("cloud") == std::string::npos;
        }
        if (active_tab_ == QuestTab::Contracts) {
            return q.id.find("contract") != std::string::npos || q.id.find("cloud") != std::string::npos;
        }
        return q.id.find("main") != std::string::npos || q.id.find("plot") != std::string::npos;
    };

    std::vector<int> visible_indices;
    visible_indices.reserve(quests_.size());
    for (int i = 0; i < static_cast<int>(quests_.size()); ++i) {
        if (in_tab(quests_[static_cast<std::size_t>(i)])) {
            visible_indices.push_back(i);
        }
    }

    for (int vi = scroll_offset_; vi < static_cast<int>(visible_indices.size()); ++vi) {
        const int i = visible_indices[static_cast<std::size_t>(vi)];
        const auto& q = quests_[static_cast<std::size_t>(i)];
        const bool hovered = hovered_row_ == i;

        TextStyle title_style = TextStyle::Default();
        title_style.character_size = 14;
        title_style.bold = true;
        title_style.fill_color = hovered ? ColorPalette::DeepBrown : ColorPalette::TextBrown;

        // Status icon
        std::string status_icon = "○";
        sf::Color status_color = status_default_color_;
        if (q.status == QuestStatus::Completed) {
            status_icon = "✓";
            status_color = status_completed_color_;
        } else if (q.status == QuestStatus::Active) {
            status_icon = "●";
            status_color = status_active_color_;
        }
        TextStyle icon_style = TextStyle::Default();
        icon_style.character_size = 14;
        icon_style.bold = true;
        icon_style.fill_color = status_color;
        font_renderer_->DrawText(window, status_icon, {r.position.x + p + 4.0f, cursor_y + 9.0f}, icon_style);

        font_renderer_->DrawText(window, q.title,
                                 {r.position.x + p + 16.0f, cursor_y + 9.0f},
                                 title_style);

        // Reward preview
        TextStyle reward_style = TextStyle::HotkeyHint();
        reward_style.character_size = 11;
        reward_style.fill_color = ColorPalette::LightBrown;
        font_renderer_->DrawText(window, q.reward_text,
                                 {r.position.x + r.size.x - p - 160.0f, cursor_y + 11.0f},
                                 reward_style);

        // Progress bar
        if (q.progress_max > 0) {
            const float ratio = std::clamp(static_cast<float>(q.progress_current) / static_cast<float>(q.progress_max), 0.0f, 1.0f);
            progress_bar_.SetPosition({r.position.x + p + 16.0f, cursor_y + 26.0f});
            progress_bar_.SetProgress(ratio);
            progress_bar_.Render(window);
        }

        cursor_y += QuestMenuConfig::RowHeight;
        if (q.expanded) {
            TextStyle body = TextStyle::Default();
            body.character_size = 12;
            body.fill_color = ColorPalette::TextBrown;

            const float x = r.position.x + p + 10.0f;
            const float y = cursor_y + 8.0f;
            font_renderer_->DrawText(window, "目标: " + q.objective, {x, y}, body);
            font_renderer_->DrawText(window, "奖励: " + q.reward_text, {x, y + 16.0f}, body);
            font_renderer_->DrawWrappedText(window, q.description, {x, y + 32.0f},
                                            r.size.x - p * 2.0f - 20.0f, body, 1.2f);
            cursor_y += QuestMenuConfig::ExpandedHeight;
        }
        if (cursor_y > max_y) break;
    }
}

// ============================================================================
// 【PixelQuestMenu::GetRowAt_】
// ============================================================================
int PixelQuestMenu::GetRowAt_(float my) const {
    const float title_h = QuestMenuConfig::TitleBarHeight;
    const float list_top = rect_.position.y + title_h + 40.0f + QuestMenuConfig::ListTopMargin;
    float y = list_top;
    if (my < y) return -1;

    const auto in_tab = [this](const Quest& q) -> bool {
        if (active_tab_ == QuestTab::Commissions) {
            return q.id.find("contract") == std::string::npos && q.id.find("cloud") == std::string::npos;
        }
        if (active_tab_ == QuestTab::Contracts) {
            return q.id.find("contract") != std::string::npos || q.id.find("cloud") != std::string::npos;
        }
        return q.id.find("main") != std::string::npos || q.id.find("plot") != std::string::npos;
    };

    std::vector<int> visible_indices;
    visible_indices.reserve(quests_.size());
    for (int i = 0; i < static_cast<int>(quests_.size()); ++i) {
        if (in_tab(quests_[static_cast<std::size_t>(i)])) {
            visible_indices.push_back(i);
        }
    }

    const float max_y = rect_.position.y + rect_.size.y - QuestMenuConfig::TextPadding;
    for (int vi = scroll_offset_; vi < static_cast<int>(visible_indices.size()); ++vi) {
        const int i = visible_indices[static_cast<std::size_t>(vi)];
        const float row_h = QuestMenuConfig::RowHeight;
        if (my >= y && my <= y + row_h) {
            return i;
        }
        y += row_h;
        if (quests_[static_cast<std::size_t>(i)].expanded) {
            y += QuestMenuConfig::ExpandedHeight;
        }
        if (y > max_y) break;
    }
    return -1;
}

int PixelQuestMenu::GetTabAt_(float mx, float my) const {
    const float title_h = QuestMenuConfig::TitleBarHeight;
    const float tab_y = rect_.position.y + title_h;
    constexpr float tab_h = 40.0f;
    if (my < tab_y || my > tab_y + tab_h) return -1;
    const float p = QuestMenuConfig::TextPadding;
    const float tab_w = 72.0f;
    for (int i = 0; i < static_cast<int>(QuestTab::Count); ++i) {
        const sf::FloatRect tab_rect(
            {rect_.position.x + p + static_cast<float>(i) * tab_w, tab_y},
            {tab_w, tab_h});
        if (tab_rect.contains({mx, my})) return i;
    }
    return -1;
}

// ============================================================================
// 【PixelQuestMenu::RebuildGeometry_】
// ============================================================================
void PixelQuestMenu::RebuildGeometry_() {
    panel_vertices_.clear();
    list_vertices_.clear();

    const sf::FloatRect r = rect_;
    const float p = QuestMenuConfig::TextPadding;

    // 面板背景
    AddQuad(panel_vertices_,
            {r.position.x, r.position.y},
            {r.position.x + r.size.x, r.position.y},
            {r.position.x + r.size.x, r.position.y + r.size.y},
            {r.position.x, r.position.y + r.size.y},
            panel_fill_color_, alpha_);

    // 像素边框
    const float cs = PixelBorderConfig::CornerBlockSize;
    const float t = PixelBorderConfig::BorderThickness;
    const sf::Color& oc = border_color_;

    AddQuad(panel_vertices_, {r.position.x, r.position.y}, {r.position.x + cs, r.position.y}, {r.position.x + cs, r.position.y + cs}, {r.position.x, r.position.y + cs}, oc, alpha_);
    AddQuad(panel_vertices_, {r.position.x + r.size.x - cs, r.position.y}, {r.position.x + r.size.x, r.position.y}, {r.position.x + r.size.x, r.position.y + cs}, {r.position.x + r.size.x - cs, r.position.y + cs}, oc, alpha_);
    AddQuad(panel_vertices_, {r.position.x, r.position.y + r.size.y - cs}, {r.position.x + cs, r.position.y + r.size.y - cs}, {r.position.x + cs, r.position.y + r.size.y}, {r.position.x, r.position.y + r.size.y}, oc, alpha_);
    AddQuad(panel_vertices_, {r.position.x + r.size.x - cs, r.position.y + r.size.y - cs}, {r.position.x + r.size.x, r.position.y + r.size.y - cs}, {r.position.x + r.size.x, r.position.y + r.size.y}, {r.position.x + r.size.x - cs, r.position.y + r.size.y}, oc, alpha_);

    AddQuad(panel_vertices_, {r.position.x + cs, r.position.y}, {r.position.x + r.size.x - cs, r.position.y}, {r.position.x + r.size.x - cs, r.position.y + t}, {r.position.x + cs, r.position.y + t}, oc, alpha_);
    AddQuad(panel_vertices_, {r.position.x + cs, r.position.y + r.size.y - t}, {r.position.x + r.size.x - cs, r.position.y + r.size.y - t}, {r.position.x + r.size.x - cs, r.position.y + r.size.y}, {r.position.x + cs, r.position.y + r.size.y}, oc, alpha_);
    AddQuad(panel_vertices_, {r.position.x, r.position.y + cs}, {r.position.x + t, r.position.y + cs}, {r.position.x + t, r.position.y + r.size.y - cs}, {r.position.x, r.position.y + r.size.y - cs}, oc, alpha_);
    AddQuad(panel_vertices_, {r.position.x + r.size.x - t, r.position.y + cs}, {r.position.x + r.size.x, r.position.y + cs}, {r.position.x + r.size.x, r.position.y + r.size.y - cs}, {r.position.x + r.size.x - t, r.position.y + r.size.y - cs}, oc, alpha_);

    // 标题栏
    const float th = QuestMenuConfig::TitleBarHeight;
    const sf::Color title_bg = ColorPalette::TitleBarLight;
    AddQuad(panel_vertices_,
            {r.position.x + 1.0f, r.position.y + 1.0f},
            {r.position.x + r.size.x - 1.0f, r.position.y + 1.0f},
            {r.position.x + r.size.x - 1.0f, r.position.y + th},
            {r.position.x + 1.0f, r.position.y + th},
            title_bg, alpha_);

    // Tab bar background
    const float tab_h = 40.0f;
    AddQuad(panel_vertices_,
            {r.position.x + 1.0f, r.position.y + th},
            {r.position.x + r.size.x - 1.0f, r.position.y + th},
            {r.position.x + r.size.x - 1.0f, r.position.y + th + tab_h},
            {r.position.x + 1.0f, r.position.y + th + tab_h},
            ColorPalette::LightCream, alpha_);

    // 任务列表
    float cursor_y = r.position.y + th + tab_h + QuestMenuConfig::ListTopMargin;
    const float max_y = r.position.y + r.size.y - p;

    const auto in_tab = [this](const Quest& q) -> bool {
        if (active_tab_ == QuestTab::Commissions) {
            return q.id.find("contract") == std::string::npos && q.id.find("cloud") == std::string::npos;
        }
        if (active_tab_ == QuestTab::Contracts) {
            return q.id.find("contract") != std::string::npos || q.id.find("cloud") != std::string::npos;
        }
        return q.id.find("main") != std::string::npos || q.id.find("plot") != std::string::npos;
    };
    std::vector<int> visible_indices;
    visible_indices.reserve(quests_.size());
    for (int i = 0; i < static_cast<int>(quests_.size()); ++i) {
        if (in_tab(quests_[static_cast<std::size_t>(i)])) {
            visible_indices.push_back(i);
        }
    }

    for (int vi = scroll_offset_; vi < static_cast<int>(visible_indices.size()); ++vi) {
        const int i = visible_indices[static_cast<std::size_t>(vi)];
        const auto& quest = quests_[static_cast<std::size_t>(i)];
        const float row_y = cursor_y;
        const bool is_hovered = (hovered_row_ == i);

        // 行背景
        sf::Color row_bg = button_default_color_;
        if (quest.status == QuestStatus::Completed) {
            row_bg = button_selected_color_;
        } else if (is_hovered) {
            row_bg = button_hover_color_;
        }

        AddQuad(list_vertices_,
                {r.position.x + p, row_y},
                {r.position.x + r.size.x - p, row_y},
                {r.position.x + r.size.x - p, row_y + QuestMenuConfig::RowHeight},
                {r.position.x + p, row_y + QuestMenuConfig::RowHeight},
                row_bg, alpha_);

        // 状态标记
        sf::Color status_color;
        if (quest.status == QuestStatus::Completed) {
            status_color = status_completed_color_;
        } else if (quest.status == QuestStatus::Active) {
            status_color = status_active_color_;
        } else {
            status_color = status_default_color_;
        }

        // 展开/折叠三角
        if (quest.expanded) {
            AddQuad(list_vertices_,
                    {r.position.x + p + 2.0f, row_y + 6.0f},
                    {r.position.x + p + 8.0f, row_y + 6.0f},
                    {r.position.x + p + 5.0f, row_y + 12.0f},
                    {r.position.x + p + 2.0f, row_y + 12.0f},
                    ColorPalette::DarkGray, alpha_);
        } else {
            AddQuad(list_vertices_,
                    {r.position.x + p + 2.0f, row_y + 6.0f},
                    {r.position.x + p + 8.0f, row_y + 9.0f},
                    {r.position.x + p + 2.0f, row_y + 12.0f},
                    {r.position.x + p + 2.0f, row_y + 12.0f},
                    ColorPalette::DarkGray, alpha_);
        }

        // 展开区域（浅黄色背景）
        cursor_y += QuestMenuConfig::RowHeight;
        if (quest.expanded) {
            AddQuad(list_vertices_,
                    {r.position.x + p, cursor_y},
                    {r.position.x + r.size.x - p, cursor_y},
                    {r.position.x + r.size.x - p, cursor_y + QuestMenuConfig::ExpandedHeight},
                    {r.position.x + p, cursor_y + QuestMenuConfig::ExpandedHeight},
                    ColorPalette::HighlightYellow, alpha_);
            cursor_y += QuestMenuConfig::ExpandedHeight;
        }

        if (cursor_y > max_y) break;
    }
}

// ============================================================================
// 【PixelQuestMenu::draw】
// ============================================================================
void PixelQuestMenu::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    (void)states;
    target.draw(panel_vertices_);
    target.draw(list_vertices_);
}

}  // namespace CloudSeamanor::engine
