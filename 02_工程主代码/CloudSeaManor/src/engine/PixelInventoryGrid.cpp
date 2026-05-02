#include "CloudSeamanor/engine/PixelInventoryGrid.hpp"

#include "CloudSeamanor/engine/PixelFontRenderer.hpp"
#include "CloudSeamanor/engine/UiVertexHelpers.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

namespace {
using CloudSeamanor::engine::uivx::AddQuad;

inline void AddDashedRect(sf::VertexArray& va,
                          const sf::FloatRect& r,
                          float dash_len,
                          float gap_len,
                          const sf::Color& color,
                          float alpha = 1.0f) {
    const float total = std::max(1.0f, dash_len + gap_len);
    const int top_n = std::max(1, static_cast<int>(std::floor(r.size.x / total)));
    const int side_n = std::max(1, static_cast<int>(std::floor(r.size.y / total)));

    for (int i = 0; i < top_n; ++i) {
        const float x0 = r.position.x + static_cast<float>(i) * total;
        const float x1 = std::min(r.position.x + r.size.x, x0 + dash_len);
        AddQuad(va, {x0, r.position.y}, {x1, r.position.y}, {x1, r.position.y + 1.0f}, {x0, r.position.y + 1.0f}, color, alpha);
        AddQuad(va, {x0, r.position.y + r.size.y - 1.0f}, {x1, r.position.y + r.size.y - 1.0f}, {x1, r.position.y + r.size.y}, {x0, r.position.y + r.size.y}, color, alpha);
    }
    for (int i = 0; i < side_n; ++i) {
        const float y0 = r.position.y + static_cast<float>(i) * total;
        const float y1 = std::min(r.position.y + r.size.y, y0 + dash_len);
        AddQuad(va, {r.position.x, y0}, {r.position.x + 1.0f, y0}, {r.position.x + 1.0f, y1}, {r.position.x, y1}, color, alpha);
        AddQuad(va, {r.position.x + r.size.x - 1.0f, y0}, {r.position.x + r.size.x, y0}, {r.position.x + r.size.x, y1}, {r.position.x + r.size.x - 1.0f, y1}, color, alpha);
    }
}
}  // namespace

// ============================================================================
// 【PixelInventoryGrid::PixelInventoryGrid】
// ============================================================================
PixelInventoryGrid::PixelInventoryGrid()
    : rect_{InventoryGridConfig::Position, InventoryGridConfig::Size},
      panel_vertices_(sf::PrimitiveType::Triangles),
      grid_vertices_(sf::PrimitiveType::Triangles),
      highlight_vertices_(sf::PrimitiveType::Triangles),
      tooltip_vertices_(sf::PrimitiveType::Triangles) {
    RebuildGeometry_();
}

// ============================================================================
// 【PixelInventoryGrid::SetPosition】
// ============================================================================
void PixelInventoryGrid::SetPosition(const sf::Vector2f& pos) {
    rect_.position = pos;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelInventoryGrid::UpdateItems】
// ============================================================================
void PixelInventoryGrid::UpdateItems(const std::vector<InventoryItem>& items) {
    items_ = items;
    geometry_dirty_ = true;
}

void PixelInventoryGrid::UpdateSocialEntries(const std::vector<SocialNpcEntry>& entries) {
    social_entries_ = entries;
    if (selected_social_index_ >= static_cast<int>(social_entries_.size())) {
        selected_social_index_ = social_entries_.empty() ? -1 : 0;
    }
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelInventoryGrid::FadeIn】
// ============================================================================
void PixelInventoryGrid::FadeIn(float duration_seconds) {
    anim_state_ = AnimationState::FadeIn;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
    visible_ = true;
}

// ============================================================================
// 【PixelInventoryGrid::FadeOut】
// ============================================================================
void PixelInventoryGrid::FadeOut(float duration_seconds) {
    anim_state_ = AnimationState::FadeOut;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
}

// ============================================================================
// 【PixelInventoryGrid::UpdateAnimation】
// ============================================================================
void PixelInventoryGrid::UpdateAnimation(float delta_seconds) {
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
// 【PixelInventoryGrid::SetActiveTab】
// ============================================================================
void PixelInventoryGrid::SetActiveTab(InventoryTab tab) {
    if (active_tab_ == tab) return;
    active_tab_ = tab;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelInventoryGrid::SetSelectedSlot】
// ============================================================================
void PixelInventoryGrid::SetSelectedSlot(int index) {
    if (selected_slot_ == index) return;
    selected_slot_ = index;
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelInventoryGrid::MouseHover】
// ============================================================================
void PixelInventoryGrid::MouseHover(float mx, float my) {
    tooltip_mouse_pos_ = {mx, my};
    hovered_tab_ = -1;
    for (int i = 0; i < static_cast<int>(InventoryTab::Count); ++i) {
        if (GetTabRect_(i).contains({mx, my})) {
            hovered_tab_ = i;
            break;
        }
    }

    const int idx = GetSlotIndexAt_(mx, my);
    if (hovered_slot_ != idx) {
        hovered_slot_ = idx;
        tooltip_dirty_ = true;
        tooltip_hover_elapsed_ = 0.0f;
    } else if (hovered_slot_ >= 0) {
        tooltip_hover_elapsed_ += 1.0f / 60.0f;
    }
    if (active_tab_ == InventoryTab::Social) {
        hovered_social_index_ = -1;
        constexpr int cols = InventoryGridConfig::SocialColumns;
        constexpr float card_h = InventoryGridConfig::SocialCardHeight;
        constexpr float spacing = InventoryGridConfig::SocialCardSpacing;
        const float p = InventoryGridConfig::TextPadding;
        const float start_x = rect_.position.x + p;
        const float start_y = rect_.position.y + InventoryGridConfig::TabBarHeight + p;
        const float total_w = rect_.size.x - p * 2.0f;
        const float card_w = (total_w - spacing) / static_cast<float>(cols);
        for (int i = 0; i < static_cast<int>(social_entries_.size()); ++i) {
            const int col = i % cols;
            const int row = i / cols;
            const sf::FloatRect card({
                start_x + col * (card_w + spacing),
                start_y + row * (card_h + spacing)
            }, {card_w, card_h});
            if (card.contains({mx, my})) {
                hovered_social_index_ = i;
                break;
            }
        }
    }
    geometry_dirty_ = true;
}

// ============================================================================
// 【PixelInventoryGrid::MouseClick】
// ============================================================================
void PixelInventoryGrid::MouseClick(float mx, float my) {
    for (int i = 0; i < static_cast<int>(InventoryTab::Count); ++i) {
        if (GetTabRect_(i).contains({mx, my})) {
            SetActiveTab(static_cast<InventoryTab>(i));
            return;
        }
    }

    if (active_tab_ == InventoryTab::Social) {
        constexpr int cols = InventoryGridConfig::SocialColumns;
        constexpr float card_h = InventoryGridConfig::SocialCardHeight;
        constexpr float spacing = InventoryGridConfig::SocialCardSpacing;
        const float p = InventoryGridConfig::TextPadding;
        const float start_x = rect_.position.x + p;
        const float start_y = rect_.position.y + InventoryGridConfig::TabBarHeight + p;
        const float total_w = rect_.size.x - p * 2.0f;
        const float card_w = (total_w - spacing) / static_cast<float>(cols);
        for (int i = 0; i < static_cast<int>(social_entries_.size()); ++i) {
            const int col = i % cols;
            const int row = i / cols;
            const sf::FloatRect card({
                start_x + col * (card_w + spacing),
                start_y + row * (card_h + spacing)
            }, {card_w, card_h});
            if (card.contains({mx, my})) {
                selected_social_index_ = i;
                geometry_dirty_ = true;
                return;
            }
        }
        return;
    }

    const int idx = GetSlotIndexAt_(mx, my);
    if (idx >= 0) {
        SetSelectedSlot(idx);
    }
}

// ============================================================================
// 【PixelInventoryGrid::GetHoveredItem】
// ============================================================================
std::optional<InventoryItem> PixelInventoryGrid::GetHoveredItem() const {
    if (hovered_slot_ >= 0 && hovered_slot_ < static_cast<int>(items_.size())) {
        return items_[static_cast<std::size_t>(hovered_slot_)];
    }
    return std::nullopt;
}

std::optional<SocialNpcEntry> PixelInventoryGrid::GetSelectedSocialNpc() const {
    if (selected_social_index_ >= 0
        && selected_social_index_ < static_cast<int>(social_entries_.size())) {
        return social_entries_[static_cast<std::size_t>(selected_social_index_)];
    }
    return std::nullopt;
}

// ============================================================================
// 【PixelInventoryGrid::Render】
// ============================================================================
void PixelInventoryGrid::Render(sf::RenderWindow& window) {
    if (!visible_) return;

    if (geometry_dirty_) {
        RebuildGeometry_();
        geometry_dirty_ = false;
    }

    sf::RenderStates alpha_states;
    alpha_states.blendMode = sf::BlendMode();
    window.draw(panel_vertices_, alpha_states);
    window.draw(grid_vertices_, alpha_states);
    window.draw(highlight_vertices_, alpha_states);

    if (font_renderer_ != nullptr && font_renderer_->IsLoaded()) {
        if (items_.empty() && active_tab_ != InventoryTab::Social) {
            font_renderer_->DrawCenteredText(window, "背包空空如也",
                                             {rect_.position.x + rect_.size.x * 0.5f, rect_.position.y + rect_.size.y * 0.5f},
                                             TextStyle::Default());
        }
    }
}

// ============================================================================
// 【PixelInventoryGrid::GetSlotRect_】
// ============================================================================
sf::FloatRect PixelInventoryGrid::GetSlotRect_(int col, int row) const {
    const float slot_s = InventoryGridConfig::SlotSize;
    const float spacing = InventoryGridConfig::SlotSpacing;
    const float tab_h = InventoryGridConfig::TabBarHeight;
    const float p = InventoryGridConfig::TextPadding;
    const float grid_start_x = rect_.position.x + p;
    const float grid_start_y = rect_.position.y + tab_h + p;

    return {
        {grid_start_x + static_cast<float>(col) * (slot_s + spacing),
         grid_start_y + static_cast<float>(row) * (slot_s + spacing)},
        {slot_s, slot_s}
    };
}

// ============================================================================
// 【PixelInventoryGrid::GetTabRect_】
// ============================================================================
sf::FloatRect PixelInventoryGrid::GetTabRect_(int tab_index) const {
    const float tab_w = rect_.size.x / static_cast<float>(InventoryTab::Count);
    const float tab_h = InventoryGridConfig::TabBarHeight;
    return {
        {rect_.position.x + static_cast<float>(tab_index) * tab_w,
         rect_.position.y},
        {tab_w, tab_h}
    };
}

// ============================================================================
// 【PixelInventoryGrid::GetSlotIndexAt_】
// ============================================================================
int PixelInventoryGrid::GetSlotIndexAt_(float mx, float my) const {
    const float slot_s = InventoryGridConfig::SlotSize;
    const float spacing = InventoryGridConfig::SlotSpacing;
    const float tab_h = InventoryGridConfig::TabBarHeight;
    const float p = InventoryGridConfig::TextPadding;
    const float grid_start_x = rect_.position.x + p;
    const float grid_start_y = rect_.position.y + tab_h + p;

    const float rel_x = mx - grid_start_x;
    const float rel_y = my - grid_start_y;

    if (rel_x < 0 || rel_y < 0) return -1;

    const int col = static_cast<int>(rel_x / (slot_s + spacing));
    const int row = static_cast<int>(rel_y / (slot_s + spacing));

    if (col >= InventoryGridConfig::Columns || row >= InventoryGridConfig::Rows) return -1;

    return row * InventoryGridConfig::Columns + col;
}

// ============================================================================
// 【PixelInventoryGrid::RebuildGeometry_】
// ============================================================================
void PixelInventoryGrid::RebuildGeometry_() {
    panel_vertices_.clear();
    grid_vertices_.clear();
    highlight_vertices_.clear();

    const sf::FloatRect r = rect_;
    const float p = InventoryGridConfig::TextPadding;

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
    const float th = InventoryGridConfig::TabBarHeight;
    const sf::Color title_bg = ColorPalette::TitleBarLight;
    AddQuad(panel_vertices_,
            {r.position.x + 1.0f, r.position.y + 1.0f},
            {r.position.x + r.size.x - 1.0f, r.position.y + 1.0f},
            {r.position.x + r.size.x - 1.0f, r.position.y + th},
            {r.position.x + 1.0f, r.position.y + th},
            title_bg, alpha_);

    for (int i = 0; i < static_cast<int>(InventoryTab::Count); ++i) {
        const sf::FloatRect tab = GetTabRect_(i);
        const bool active = active_tab_ == static_cast<InventoryTab>(i);
        const bool hovered = hovered_tab_ == i;
        AddQuad(panel_vertices_,
                {tab.position.x + 1.0f, tab.position.y + 1.0f},
                {tab.position.x + tab.size.x - 1.0f, tab.position.y + 1.0f},
                {tab.position.x + tab.size.x - 1.0f, tab.position.y + tab.size.y - 1.0f},
                {tab.position.x + 1.0f, tab.position.y + tab.size.y - 1.0f},
                active ? ColorPalette::ActiveGreen : (hovered ? ColorPalette::HighlightYellow : ColorPalette::TitleBarLight), alpha_);
        if (hovered && !active) {
            AddQuad(panel_vertices_,
                    {tab.position.x + 4.0f, tab.position.y + tab.size.y - 3.0f},
                    {tab.position.x + tab.size.x - 4.0f, tab.position.y + tab.size.y - 3.0f},
                    {tab.position.x + tab.size.x - 4.0f, tab.position.y + tab.size.y - 1.0f},
                    {tab.position.x + 4.0f, tab.position.y + tab.size.y - 1.0f},
                    ColorPalette::LightBrown, alpha_);
        }
    }

    if (active_tab_ == InventoryTab::Social) {
        constexpr int cols = InventoryGridConfig::SocialColumns;
        constexpr float card_h = InventoryGridConfig::SocialCardHeight;
        constexpr float spacing = InventoryGridConfig::SocialCardSpacing;
        const float start_x = r.position.x + p;
        const float start_y = r.position.y + InventoryGridConfig::TabBarHeight + p;
        const float total_w = r.size.x - p * 2.0f;
        const float card_w = (total_w - spacing) / static_cast<float>(cols);

        for (int i = 0; i < static_cast<int>(social_entries_.size()); ++i) {
            const auto& entry = social_entries_[static_cast<std::size_t>(i)];
            const int col = i % cols;
            const int row = i / cols;
            const sf::FloatRect card({
                start_x + col * (card_w + spacing),
                start_y + row * (card_h + spacing)
            }, {card_w, card_h});

            const bool hovered = hovered_social_index_ == i;
            AddQuad(grid_vertices_,
                    {card.position.x, card.position.y},
                    {card.position.x + card.size.x, card.position.y},
                    {card.position.x + card.size.x, card.position.y + card.size.y},
                    {card.position.x, card.position.y + card.size.y},
                    hovered ? ColorPalette::HighlightYellow : ColorPalette::BackgroundWhite, alpha_);

            // Avatar placeholder
            const sf::FloatRect avatar({
                card.position.x + InventoryGridConfig::SocialAvatarOffsetX,
                card.position.y + InventoryGridConfig::SocialAvatarOffsetY
            }, {InventoryGridConfig::SocialAvatarSize, InventoryGridConfig::SocialAvatarSize});
            AddQuad(grid_vertices_,
                    {avatar.position.x, avatar.position.y},
                    {avatar.position.x + avatar.size.x, avatar.position.y},
                    {avatar.position.x + avatar.size.x, avatar.position.y + avatar.size.y},
                    {avatar.position.x, avatar.position.y + avatar.size.y},
                    ColorPalette::LightGray, alpha_);

            // Favor bar
            const float favor_ratio = std::clamp(static_cast<float>(entry.favor) / 800.0f, 0.0f, 1.0f);
            const sf::FloatRect bar_bg({
                card.position.x + InventoryGridConfig::SocialFavorBarOffsetX,
                card.position.y + InventoryGridConfig::SocialFavorBarOffsetY
            }, {card.size.x - InventoryGridConfig::SocialBarRightPadding - InventoryGridConfig::SocialFavorBarOffsetX,
                InventoryGridConfig::SocialFavorBarHeight});
            AddQuad(grid_vertices_,
                    {bar_bg.position.x, bar_bg.position.y},
                    {bar_bg.position.x + bar_bg.size.x, bar_bg.position.y},
                    {bar_bg.position.x + bar_bg.size.x, bar_bg.position.y + bar_bg.size.y},
                    {bar_bg.position.x, bar_bg.position.y + bar_bg.size.y},
                    ColorPalette::LightGray, alpha_);
            AddQuad(grid_vertices_,
                    {bar_bg.position.x, bar_bg.position.y},
                    {bar_bg.position.x + bar_bg.size.x * favor_ratio, bar_bg.position.y},
                    {bar_bg.position.x + bar_bg.size.x * favor_ratio, bar_bg.position.y + bar_bg.size.y},
                    {bar_bg.position.x, bar_bg.position.y + bar_bg.size.y},
                    ColorPalette::ActiveGreen, alpha_);

            const sf::FloatRect event_bar({
                card.position.x + InventoryGridConfig::SocialEventBarOffsetX,
                card.position.y + InventoryGridConfig::SocialEventBarOffsetY
            }, {card.size.x - InventoryGridConfig::SocialBarRightPadding - InventoryGridConfig::SocialEventBarOffsetX,
                InventoryGridConfig::SocialEventBarHeight});
            const float event_ratio = std::clamp(static_cast<float>(entry.completed_heart_events) / 8.0f, 0.0f, 1.0f);
            AddQuad(grid_vertices_,
                    {event_bar.position.x, event_bar.position.y},
                    {event_bar.position.x + event_bar.size.x, event_bar.position.y},
                    {event_bar.position.x + event_bar.size.x, event_bar.position.y + event_bar.size.y},
                    {event_bar.position.x, event_bar.position.y + event_bar.size.y},
                    ColorPalette::LightGray, alpha_);
            AddQuad(grid_vertices_,
                    {event_bar.position.x, event_bar.position.y},
                    {event_bar.position.x + event_bar.size.x * event_ratio, event_bar.position.y},
                    {event_bar.position.x + event_bar.size.x * event_ratio, event_bar.position.y + event_bar.size.y},
                    {event_bar.position.x, event_bar.position.y + event_bar.size.y},
                    ColorPalette::WarningPink, alpha_);

            if (selected_social_index_ == i) {
                const float h = InventoryGridConfig::SelectionOutlineThickness;
                AddQuad(highlight_vertices_,
                        {card.position.x - h, card.position.y - h},
                        {card.position.x + card.size.x + h, card.position.y - h},
                        {card.position.x + card.size.x + h, card.position.y},
                        {card.position.x - h, card.position.y},
                        ColorPalette::ActiveGreen, alpha_);
            }
        }
        return;
    }

    // 物品格
    for (int row = 0; row < InventoryGridConfig::Rows; ++row) {
        for (int col = 0; col < InventoryGridConfig::Columns; ++col) {
            const sf::FloatRect slot_r = GetSlotRect_(col, row);
            const int slot_idx = row * InventoryGridConfig::Columns + col;
            const bool out_of_range = (slot_idx >= static_cast<int>(items_.size()));
            const bool is_empty = out_of_range || items_[static_cast<std::size_t>(slot_idx)].empty;
            const bool is_disabled = (!out_of_range && !is_empty && items_[static_cast<std::size_t>(slot_idx)].count <= 0);

            AddQuad(grid_vertices_,
                    {slot_r.position.x, slot_r.position.y},
                    {slot_r.position.x + slot_r.size.x, slot_r.position.y},
                    {slot_r.position.x + slot_r.size.x, slot_r.position.y + slot_r.size.y},
                    {slot_r.position.x, slot_r.position.y + slot_r.size.y},
                    (is_disabled ? ColorPalette::DarkGray : ColorPalette::BackgroundWhite), alpha_);

            // 格子描边
            if (is_empty) {
                AddDashedRect(grid_vertices_, slot_r, 5.0f, 3.0f, ColorPalette::LightGray, alpha_);
            } else {
                AddQuad(grid_vertices_,
                        {slot_r.position.x, slot_r.position.y},
                        {slot_r.position.x + slot_r.size.x, slot_r.position.y},
                        {slot_r.position.x + slot_r.size.x, slot_r.position.y + InventoryGridConfig::SlotBorderThickness},
                        {slot_r.position.x, slot_r.position.y + InventoryGridConfig::SlotBorderThickness},
                        ColorPalette::LightGray, alpha_);
                AddQuad(grid_vertices_,
                        {slot_r.position.x, slot_r.position.y + slot_r.size.y - InventoryGridConfig::SlotBorderThickness},
                        {slot_r.position.x + slot_r.size.x, slot_r.position.y + slot_r.size.y - InventoryGridConfig::SlotBorderThickness},
                        {slot_r.position.x + slot_r.size.x, slot_r.position.y + slot_r.size.y},
                        {slot_r.position.x, slot_r.position.y + slot_r.size.y},
                        ColorPalette::DarkGray, alpha_);
                AddQuad(grid_vertices_,
                        {slot_r.position.x, slot_r.position.y},
                        {slot_r.position.x + InventoryGridConfig::SlotBorderThickness, slot_r.position.y},
                        {slot_r.position.x + InventoryGridConfig::SlotBorderThickness, slot_r.position.y + slot_r.size.y},
                        {slot_r.position.x, slot_r.position.y + slot_r.size.y},
                        ColorPalette::LightGray, alpha_);
                AddQuad(grid_vertices_,
                        {slot_r.position.x + slot_r.size.x - InventoryGridConfig::SlotBorderThickness, slot_r.position.y},
                        {slot_r.position.x + slot_r.size.x, slot_r.position.y},
                        {slot_r.position.x + slot_r.size.x, slot_r.position.y + slot_r.size.y},
                        {slot_r.position.x + slot_r.size.x - InventoryGridConfig::SlotBorderThickness, slot_r.position.y + slot_r.size.y},
                        ColorPalette::DarkGray, alpha_);
            }

            if (is_disabled) {
                const float pad = 10.0f;
                const float thick = 2.0f;
                const sf::Color xcol(210, 210, 210, 255);
                // diagonal backslash
                AddQuad(highlight_vertices_,
                        {slot_r.position.x + pad, slot_r.position.y + pad},
                        {slot_r.position.x + pad + thick, slot_r.position.y + pad},
                        {slot_r.position.x + slot_r.size.x - pad, slot_r.position.y + slot_r.size.y - pad},
                        {slot_r.position.x + slot_r.size.x - pad - thick, slot_r.position.y + slot_r.size.y - pad},
                        xcol, alpha_);
                // diagonal slash
                AddQuad(highlight_vertices_,
                        {slot_r.position.x + slot_r.size.x - pad - thick, slot_r.position.y + pad},
                        {slot_r.position.x + slot_r.size.x - pad, slot_r.position.y + pad},
                        {slot_r.position.x + pad + thick, slot_r.position.y + slot_r.size.y - pad},
                        {slot_r.position.x + pad, slot_r.position.y + slot_r.size.y - pad},
                        xcol, alpha_);
            }
        }
    }

    // 选中格子高亮
    if (selected_slot_ >= 0) {
        const int col = selected_slot_ % InventoryGridConfig::Columns;
        const int row = selected_slot_ / InventoryGridConfig::Columns;
        const sf::FloatRect slot_r = GetSlotRect_(col, row);
        const float h = InventoryGridConfig::SelectionOutlineThickness;

        AddQuad(highlight_vertices_,
                {slot_r.position.x - h, slot_r.position.y - h},
                {slot_r.position.x + slot_r.size.x + h, slot_r.position.y - h},
                {slot_r.position.x + slot_r.size.x + h, slot_r.position.y},
                {slot_r.position.x - h, slot_r.position.y},
                ColorPalette::ActiveGreen, alpha_);
        AddQuad(highlight_vertices_,
                {slot_r.position.x - h, slot_r.position.y + slot_r.size.y},
                {slot_r.position.x + slot_r.size.x + h, slot_r.position.y + slot_r.size.y},
                {slot_r.position.x + slot_r.size.x + h, slot_r.position.y + slot_r.size.y + h},
                {slot_r.position.x - h, slot_r.position.y + slot_r.size.y + h},
                ColorPalette::ActiveGreen, alpha_);
        AddQuad(highlight_vertices_,
                {slot_r.position.x - h, slot_r.position.y},
                {slot_r.position.x, slot_r.position.y},
                {slot_r.position.x, slot_r.position.y + slot_r.size.y},
                {slot_r.position.x - h, slot_r.position.y + slot_r.size.y},
                ColorPalette::ActiveGreen, alpha_);
        AddQuad(highlight_vertices_,
                {slot_r.position.x + slot_r.size.x, slot_r.position.y},
                {slot_r.position.x + slot_r.size.x + h, slot_r.position.y},
                {slot_r.position.x + slot_r.size.x + h, slot_r.position.y + slot_r.size.y},
                {slot_r.position.x + slot_r.size.x, slot_r.position.y + slot_r.size.y},
                ColorPalette::ActiveGreen, alpha_);
    }
}

// ============================================================================
// 【PixelInventoryGrid::draw】
// ============================================================================
void PixelInventoryGrid::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    (void)states;
    target.draw(panel_vertices_);
    target.draw(grid_vertices_);
    target.draw(highlight_vertices_);
}

}  // namespace CloudSeamanor::engine
