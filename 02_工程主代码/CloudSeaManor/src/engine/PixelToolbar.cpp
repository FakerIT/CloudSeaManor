#include "CloudSeamanor/PixelToolbar.hpp"

#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <functional>

namespace CloudSeamanor::engine {

namespace {
inline void AddQuad(sf::VertexArray& va,
                    const sf::Vector2f& p0,
                    const sf::Vector2f& p1,
                    const sf::Vector2f& p2,
                    const sf::Vector2f& p3,
                    const sf::Color& color,
                    float alpha = 1.0f) {
    sf::Color c = color;
    c.a = static_cast<std::uint8_t>(std::clamp(static_cast<float>(c.a) * alpha, 0.0f, 255.0f));
    const auto snap = [](const sf::Vector2f& p) { return sf::Vector2f(std::round(p.x), std::round(p.y)); };
    const sf::Vector2f s0 = snap(p0);
    const sf::Vector2f s1 = snap(p1);
    const sf::Vector2f s2 = snap(p2);
    const sf::Vector2f s3 = snap(p3);
    va.append(sf::Vertex(s0, c));
    va.append(sf::Vertex(s1, c));
    va.append(sf::Vertex(s2, c));
    va.append(sf::Vertex(s0, c));
    va.append(sf::Vertex(s2, c));
    va.append(sf::Vertex(s3, c));
}
}  // namespace

PixelToolbar::PixelToolbar()
    : rect_{ToolbarConfig::Position,
            {ToolbarConfig::TotalWidth, ToolbarConfig::Height}},
      bg_vertices_(sf::PrimitiveType::Triangles),
      slot_vertices_(sf::PrimitiveType::Triangles),
      select_vertices_(sf::PrimitiveType::Triangles) {
    RebuildGeometry_();
}

void PixelToolbar::SetPosition(const sf::Vector2f& pos) {
    rect_.position = pos;
    geometry_dirty_ = true;
}

void PixelToolbar::UpdateSlots(const std::array<ToolbarSlot, ToolbarConfig::SlotCount>& slots) {
    slots_ = slots;
    geometry_dirty_ = true;
}

void PixelToolbar::SetSelectedSlot(int index) {
    index = std::clamp(index, 0, ToolbarConfig::SlotCount - 1);
    if (selected_slot_ == index) return;
    selected_slot_ = index;
    geometry_dirty_ = true;
}

void PixelToolbar::MoveSelection(int delta) {
    if (delta == 0) return;
    const int next = (selected_slot_ + delta + ToolbarConfig::SlotCount) % ToolbarConfig::SlotCount;
    SetSelectedSlot(next);
}

void PixelToolbar::FadeIn(float duration_seconds) {
    anim_state_ = AnimationState::FadeIn;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
    visible_ = true;
}

void PixelToolbar::FadeOut(float duration_seconds) {
    anim_state_ = AnimationState::FadeOut;
    anim_duration_ = duration_seconds;
    anim_elapsed_ = 0.0f;
}

void PixelToolbar::UpdateAnimation(float delta_seconds) {
    highlight_timer_ += std::max(0.0f, delta_seconds);
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

void PixelToolbar::Render(sf::RenderWindow& window) {
    if (!visible_) return;

    if (geometry_dirty_) {
        RebuildGeometry_();
        geometry_dirty_ = false;
    }

    sf::RenderStates alpha_states;
    alpha_states.blendMode = sf::BlendMode();
    window.draw(bg_vertices_, alpha_states);
    window.draw(slot_vertices_, alpha_states);
    window.draw(select_vertices_, alpha_states);
}

void PixelToolbar::RebuildGeometry_() {
    bg_vertices_.clear();
    slot_vertices_.clear();
    select_vertices_.clear();

    const float sx = rect_.position.x;
    const float sy = rect_.position.y;
    const float slot_s = ToolbarConfig::SlotSize;
    const float spacing = ToolbarConfig::SlotSpacing;

    const float bg_l = sx - 4.0f;
    const float bg_t = sy - 2.0f;
    const float bg_r = sx + rect_.size.x + 4.0f;
    const float bg_b = sy + rect_.size.y + 2.0f;

    AddQuad(bg_vertices_,
            {bg_l, bg_t}, {bg_r, bg_t}, {bg_r, bg_b}, {bg_l, bg_b},
            bg_fill_color_, alpha_);
    AddQuad(bg_vertices_,
            {bg_l, bg_t}, {bg_r, bg_t}, {bg_r, bg_t + 1.0f}, {bg_l, bg_t + 1.0f},
            bg_border_color_, alpha_);
    AddQuad(bg_vertices_,
            {bg_l, bg_b - 1.0f}, {bg_r, bg_b - 1.0f}, {bg_r, bg_b}, {bg_l, bg_b},
            bg_border_color_, alpha_);
    AddQuad(bg_vertices_,
            {bg_l, bg_t}, {bg_l + 1.0f, bg_t}, {bg_l + 1.0f, bg_b}, {bg_l, bg_b},
            bg_border_color_, alpha_);
    AddQuad(bg_vertices_,
            {bg_r - 1.0f, bg_t}, {bg_r, bg_t}, {bg_r, bg_b}, {bg_r - 1.0f, bg_b},
            bg_border_color_, alpha_);

    for (int i = 0; i < ToolbarConfig::SlotCount; ++i) {
        const float gx = sx + static_cast<float>(i) * (slot_s + spacing);
        const float gy = sy;

        const sf::Color slot_bg = slots_[i].empty
            ? ColorPalette::LightGray
            : ColorPalette::BackgroundWhite;
        AddQuad(slot_vertices_,
                {gx, gy}, {gx + slot_s, gy}, {gx + slot_s, gy + slot_s}, {gx, gy + slot_s},
                slot_bg, alpha_);

        AddQuad(slot_vertices_,
                {gx, gy}, {gx + slot_s, gy}, {gx + slot_s, gy + 1.0f}, {gx, gy + 1.0f},
                ColorPalette::LightGray, alpha_);
        AddQuad(slot_vertices_,
                {gx, gy + slot_s - 1.0f}, {gx + slot_s, gy + slot_s - 1.0f}, {gx + slot_s, gy + slot_s}, {gx, gy + slot_s},
                ColorPalette::DarkGray, alpha_);
        AddQuad(slot_vertices_,
                {gx, gy}, {gx + 1.0f, gy}, {gx + 1.0f, gy + slot_s}, {gx, gy + slot_s},
                ColorPalette::LightGray, alpha_);
        AddQuad(slot_vertices_,
                {gx + slot_s - 1.0f, gy}, {gx + slot_s, gy}, {gx + slot_s, gy + slot_s}, {gx + slot_s - 1.0f, gy + slot_s},
                ColorPalette::DarkGray, alpha_);

        if (slots_[i].highlighted) {
            const float pulse = 0.55f + 0.45f * std::sin(highlight_timer_ * ToolbarConfig::HighlightPulseFrequency);
            sf::Color glow = ColorPalette::WarningPink;
            glow.a = static_cast<std::uint8_t>(
                static_cast<float>(ToolbarConfig::HighlightAlphaBase) + pulse * ToolbarConfig::HighlightAlphaRange);
            const float h = ToolbarConfig::HighlightOutlineThickness;
            AddQuad(select_vertices_,
                    {gx - h, gy - h}, {gx + slot_s + h, gy - h},
                    {gx + slot_s + h, gy}, {gx - h, gy},
                    glow, alpha_);
            AddQuad(select_vertices_,
                    {gx - h, gy + slot_s}, {gx + slot_s + h, gy + slot_s},
                    {gx + slot_s + h, gy + slot_s + h}, {gx - h, gy + slot_s + h},
                    glow, alpha_);
            AddQuad(select_vertices_,
                    {gx - h, gy}, {gx, gy}, {gx, gy + slot_s}, {gx - h, gy + slot_s},
                    glow, alpha_);
            AddQuad(select_vertices_,
                    {gx + slot_s, gy}, {gx + slot_s + h, gy},
                    {gx + slot_s + h, gy + slot_s}, {gx + slot_s, gy + slot_s},
                    glow, alpha_);
        }
    }

    {
        const float sel = static_cast<float>(selected_slot_);
        const float gx = sx + sel * (slot_s + spacing);
        const float gy = sy;
        const float h = ToolbarConfig::HighlightOutlineThickness;

        AddQuad(select_vertices_,
                {gx - h, gy - h}, {gx + slot_s + h, gy - h},
                {gx + slot_s + h, gy}, {gx - h, gy},
                ColorPalette::ActiveGreen, alpha_);
        AddQuad(select_vertices_,
                {gx - h, gy + slot_s}, {gx + slot_s + h, gy + slot_s},
                {gx + slot_s + h, gy + slot_s + h}, {gx - h, gy + slot_s + h},
                ColorPalette::ActiveGreen, alpha_);
        AddQuad(select_vertices_,
                {gx - h, gy}, {gx, gy}, {gx, gy + slot_s}, {gx - h, gy + slot_s},
                ColorPalette::ActiveGreen, alpha_);
        AddQuad(select_vertices_,
                {gx + slot_s, gy}, {gx + slot_s + h, gy},
                {gx + slot_s + h, gy + slot_s}, {gx + slot_s, gy + slot_s},
                ColorPalette::ActiveGreen, alpha_);
    }
}

sf::FloatRect PixelToolbar::GetSlotRect_(int slot_index) const {
    const float sx = rect_.position.x;
    const float sy = rect_.position.y;
    const float slot_s = ToolbarConfig::SlotSize;
    const float spacing = ToolbarConfig::SlotSpacing;
    return {{sx + static_cast<float>(slot_index) * (slot_s + spacing), sy}, {slot_s, slot_s}};
}

void PixelToolbar::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    (void)states;
    target.draw(bg_vertices_);
    target.draw(slot_vertices_);
    target.draw(select_vertices_);
}

}  // namespace CloudSeamanor::engine
