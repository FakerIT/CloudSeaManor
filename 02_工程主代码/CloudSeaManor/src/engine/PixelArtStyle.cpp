#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/UiVertexHelpers.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

namespace {

// ============================================================================
// 辅助：向 VertexArray 添加一个 Quad（2 个三角形 = 4 个顶点）
// ============================================================================
using CloudSeamanor::engine::uivx::AddQuad;

}  // namespace

// ============================================================================
// 【PixelArtStyle::PixelArtStyle】
// ============================================================================
PixelArtStyle::PixelArtStyle(const PixelBorderStyle& style)
    : style_(style) {
}

// ============================================================================
// 【PixelArtStyle::DrawPixelBorder】
// ============================================================================
void PixelArtStyle::DrawPixelBorder(sf::VertexArray& target, const sf::FloatRect& rect) const {
    const float cs = style_.corner_size;   // 角块尺寸
    const float t = style_.thickness;     // 边框厚度
    const sf::Color oc = style_.outline_color;
    const sf::Color ic = style_.inner_shadow_color;

    const float l = rect.position.x;
    const float r = rect.position.x + rect.size.x;
    const float t_y = rect.position.y;
    const float b_y = rect.position.y + rect.size.y;

    // 四个角块（8×8 纯色块）
    // 左上角
    AddQuad(target,
            {l, t_y}, {l + cs, t_y},
            {l + cs, t_y + cs}, {l, t_y + cs},
            oc);
    // 右上角
    AddQuad(target,
            {r - cs, t_y}, {r, t_y},
            {r, t_y + cs}, {r - cs, t_y + cs},
            oc);
    // 左下角
    AddQuad(target,
            {l, b_y - cs}, {l + cs, b_y - cs},
            {l + cs, b_y}, {l, b_y},
            oc);
    // 右下角
    AddQuad(target,
            {r - cs, b_y - cs}, {r, b_y - cs},
            {r, b_y}, {r - cs, b_y},
            oc);

    // 四条边（厚度 1px）
    // 上边
    AddQuad(target,
            {l + cs, t_y}, {r - cs, t_y},
            {r - cs, t_y + t}, {l + cs, t_y + t},
            oc);
    // 下边
    AddQuad(target,
            {l + cs, b_y - t}, {r - cs, b_y - t},
            {r - cs, b_y}, {l + cs, b_y},
            oc);
    // 左边
    AddQuad(target,
            {l, t_y + cs}, {l + t, t_y + cs},
            {l + t, b_y - cs}, {l, b_y - cs},
            oc);
    // 右边
    AddQuad(target,
            {r - t, t_y + cs}, {r, t_y + cs},
            {r, b_y - cs}, {r - t, b_y - cs},
            oc);

    // 内阴影（同一颜色深 2 度，偏移 1px）
    if (style_.draw_inner_shadow) {
        const sf::Color shadow = ColorPalette::Darken(style_.fill_color, 15);
        // 顶部内阴影
        AddQuad(target,
                {l + cs, t_y + t}, {r - cs, t_y + t},
                {r - cs, t_y + t + 1.0f}, {l + cs, t_y + t + 1.0f},
                shadow);
        // 左侧内阴影
        AddQuad(target,
                {l + t, t_y + cs}, {l + t + 1.0f, t_y + cs},
                {l + t + 1.0f, b_y - cs}, {l + t, b_y - cs},
                shadow);
    }
}

void PixelArtStyle::DrawPixelBorder(sf::RenderTarget& target,
                                    const sf::FloatRect& rect,
                                    float border_width) const {
    const float effective_border = std::max(1.0f, border_width);
    const float corner_size = style_.corner_size;
    const sf::Color outline = style_.outline_color;

    const float left = rect.position.x;
    const float right = rect.position.x + rect.size.x;
    const float top = rect.position.y;
    const float bottom = rect.position.y + rect.size.y;

    sf::VertexArray triangles;

    // 四个固定 8x8 角块。
    AddQuad(triangles, {left, top}, {left + corner_size, top},
            {left + corner_size, top + corner_size}, {left, top + corner_size}, outline);
    AddQuad(triangles, {right - corner_size, top}, {right, top},
            {right, top + corner_size}, {right - corner_size, top + corner_size}, outline);
    AddQuad(triangles, {left, bottom - corner_size}, {left + corner_size, bottom - corner_size},
            {left + corner_size, bottom}, {left, bottom}, outline);
    AddQuad(triangles, {right - corner_size, bottom - corner_size}, {right, bottom - corner_size},
            {right, bottom}, {right - corner_size, bottom}, outline);

    // 四边可变宽度填充。
    AddQuad(triangles, {left + corner_size, top}, {right - corner_size, top},
            {right - corner_size, top + effective_border}, {left + corner_size, top + effective_border}, outline);
    AddQuad(triangles, {left + corner_size, bottom - effective_border}, {right - corner_size, bottom - effective_border},
            {right - corner_size, bottom}, {left + corner_size, bottom}, outline);
    AddQuad(triangles, {left, top + corner_size}, {left + effective_border, top + corner_size},
            {left + effective_border, bottom - corner_size}, {left, bottom - corner_size}, outline);
    AddQuad(triangles, {right - effective_border, top + corner_size}, {right, top + corner_size},
            {right, bottom - corner_size}, {right - effective_border, bottom - corner_size}, outline);

    if (style_.draw_inner_shadow) {
        const sf::Color shadow = ColorPalette::Darken(style_.fill_color, 2);
        AddQuad(triangles, {left + corner_size, top + effective_border}, {right - corner_size, top + effective_border},
                {right - corner_size, top + effective_border + 1.0f}, {left + corner_size, top + effective_border + 1.0f}, shadow);
        AddQuad(triangles, {left + effective_border, top + corner_size}, {left + effective_border + 1.0f, top + corner_size},
                {left + effective_border + 1.0f, bottom - corner_size}, {left + effective_border, bottom - corner_size}, shadow);
    }

    target.draw(triangles);
}

// ============================================================================
// 【PixelArtStyle::DrawPixelPanel】
// ============================================================================
void PixelArtStyle::DrawPixelPanel(sf::VertexArray& target, const sf::FloatRect& rect) const {
    // 先画填充背景
    AddQuad(target,
            {rect.position.x, rect.position.y},
            {rect.position.x + rect.size.x, rect.position.y},
            {rect.position.x + rect.size.x, rect.position.y + rect.size.y},
            {rect.position.x, rect.position.y + rect.size.y},
            style_.fill_color);

    // 再画边框
    DrawPixelBorder(target, rect);
}

// ============================================================================
// 【PixelArtStyle::DrawPixelTitleBar】
// ============================================================================
void PixelArtStyle::DrawPixelTitleBar(sf::VertexArray& target,
                                       const sf::FloatRect& rect,
                                       float title_height) const {
    const sf::Color title_bg = ColorPalette::TitleBarLight;

    // 标题栏背景
    AddQuad(target,
            {rect.position.x + 1.0f, rect.position.y + 1.0f},
            {rect.position.x + rect.size.x - 1.0f, rect.position.y + 1.0f},
            {rect.position.x + rect.size.x - 1.0f, rect.position.y + title_height},
            {rect.position.x + 1.0f, rect.position.y + title_height},
            title_bg);

    // 标题栏下边框（1px 深线）
    AddQuad(target,
            {rect.position.x + 1.0f, rect.position.y + title_height - 1.0f},
            {rect.position.x + rect.size.x - 1.0f, rect.position.y + title_height - 1.0f},
            {rect.position.x + rect.size.x - 1.0f, rect.position.y + title_height},
            {rect.position.x + 1.0f, rect.position.y + title_height},
            ColorPalette::Darken(title_bg, 20));
}

// ============================================================================
// 【PixelArtStyle::DrawPixelCorner】
// ============================================================================
void PixelArtStyle::DrawPixelCorner(sf::VertexArray& target,
                                    const sf::Vector2f& pos,
                                    int corner) const {
    const float cs = style_.corner_size;
    const sf::Vector2f p0 = [&]() -> sf::Vector2f {
        switch (corner) {
        case 0: return {pos.x, pos.y};          // 左上
        case 1: return {pos.x - cs, pos.y};     // 右上
        case 2: return {pos.x, pos.y - cs};     // 左下
        default: return {pos.x - cs, pos.y - cs}; // 右下
        }
    }();

    AddQuad(target,
            p0,
            {p0.x + cs, p0.y},
            {p0.x + cs, p0.y + cs},
            {p0.x, p0.y + cs},
            style_.outline_color);
}

// ============================================================================
// 【PixelArtStyle::DrawPixelLine】
// ============================================================================
void PixelArtStyle::DrawPixelLine(sf::VertexArray& target,
                                  const sf::Vector2f& start,
                                  const sf::Vector2f& end) const {
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 1e-5f) {
        target.append(sf::Vertex(start, style_.outline_color));
        return;
    }

    const sf::Vector2f dir(dx / dist, dy / dist);
    const int steps = static_cast<int>(std::ceil(dist));
    for (int i = 0; i <= steps; ++i) {
        const float x = start.x + dir.x * static_cast<float>(i);
        const float y = start.y + dir.y * static_cast<float>(i);
        target.append(sf::Vertex({x, y}, style_.outline_color));
    }
}

void PixelArtStyle::DrawCoinIcon(sf::RenderTarget& target,
                                 const sf::Vector2f& position,
                                 float size) const {
    sf::VertexArray icon(sf::PrimitiveType::Triangles);
    AddQuad(icon,
            position,
            {position.x + size, position.y},
            {position.x + size, position.y + size},
            {position.x, position.y + size},
            ColorPalette::CoinGold);
    AddQuad(icon,
            {position.x + 2.0f, position.y + 2.0f},
            {position.x + size - 2.0f, position.y + 2.0f},
            {position.x + size - 2.0f, position.y + size - 2.0f},
            {position.x + 2.0f, position.y + size - 2.0f},
            ColorPalette::Lighten(ColorPalette::CoinGold, 20));
    target.draw(icon);
}

}  // namespace CloudSeamanor::engine
