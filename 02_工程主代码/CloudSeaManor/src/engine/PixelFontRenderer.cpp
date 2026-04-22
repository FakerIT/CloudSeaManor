#include "CloudSeamanor/PixelFontRenderer.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PixelFontRenderer::PixelFontRenderer】
// ============================================================================
PixelFontRenderer::PixelFontRenderer(const sf::Font& font)
    : font_(font), font_loaded_(font.getInfo().family != "") {
    if (font_loaded_) {
        // Pixel-perfect: disable glyph texture smoothing for common sizes.
        for (unsigned int size : {10u, 12u, 14u, 16u, 20u, 24u, 32u}) {
            const_cast<sf::Texture&>(font_.getTexture(size)).setSmooth(false);
        }
    }
}

PixelFontRenderer::PixelFontRenderer(const sf::Font& font, const sf::Font* fallback_font)
    : font_(font), fallback_font_(fallback_font), font_loaded_(font.getInfo().family != "") {
    if (font_loaded_) {
        for (unsigned int size : {10u, 12u, 14u, 16u, 20u, 24u, 32u}) {
            const_cast<sf::Texture&>(font_.getTexture(size)).setSmooth(false);
        }
    }
    if (fallback_font_ != nullptr && fallback_font_->getInfo().family != "") {
        for (unsigned int size : {10u, 12u, 14u, 16u, 20u, 24u, 32u}) {
            const_cast<sf::Texture&>(fallback_font_->getTexture(size)).setSmooth(false);
        }
    }
}

// ============================================================================
// 【PixelFontRenderer::DrawText】绘制带阴影的文本
// ============================================================================
void PixelFontRenderer::DrawText(sf::RenderWindow& window,
                                 const std::string& text,
                                 const sf::Vector2f& position,
                                 const TextStyle& style,
                                 TextAlignment alignment) const {
    DrawTextWithFallback(window, text, position, style, alignment);
}

// ============================================================================
// 【PixelFontRenderer::DrawText】绘制单行文本（简化版）
// ============================================================================
void PixelFontRenderer::DrawText(sf::RenderWindow& window,
                                 const std::string& text,
                                 float x, float y,
                                 const TextStyle& style) const {
    DrawText(window, text, {x, y}, style, TextAlignment::Left);
}

// ============================================================================
// 【PixelFontRenderer::MeasureText】计算文本边界
// ============================================================================
sf::Vector2f PixelFontRenderer::MeasureText(const std::string& text,
                                            const TextStyle& style) const {
    if (!font_loaded_ || text.empty()) return {0.0f, 0.0f};

    sf::Text probe(font_);
    probe.setCharacterSize(style.character_size);
    probe.setStyle(style.bold ? sf::Text::Bold : sf::Text::Regular);

    float width = 0.0f;
    float max_height = 0.0f;
    std::string segment;
    bool ascii_segment = true;

    auto FlushSegment = [&](bool is_ascii) {
        if (segment.empty()) return;
        const sf::Font& active_font = (is_ascii || fallback_font_ == nullptr) ? font_ : *fallback_font_;
        probe.setFont(active_font);
        probe.setString(sf::String::fromUtf8(segment.begin(), segment.end()));
        const auto b = probe.getLocalBounds();
        width += b.size.x;
        max_height = std::max(max_height, b.size.y);
        segment.clear();
    };

    for (std::size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        const bool is_ascii = c <= 0x7E;
        if (segment.empty()) {
            ascii_segment = is_ascii;
        } else if (ascii_segment != is_ascii) {
            FlushSegment(ascii_segment);
            ascii_segment = is_ascii;
        }
        segment.push_back(text[i]);
    }
    FlushSegment(ascii_segment);
    return {width, max_height};
}

// ============================================================================
// 【PixelFontRenderer::DrawWrappedText】绘制多行文本（自动换行）
// ============================================================================
void PixelFontRenderer::DrawWrappedText(sf::RenderWindow& window,
                                       const std::string& text,
                                       const sf::Vector2f& position,
                                       float max_width,
                                       const TextStyle& style,
                                       float line_height_multiplier) const {
    if (!font_loaded_ || text.empty()) return;

    const float line_height = static_cast<float>(style.character_size) * line_height_multiplier;
    std::string current_line;
    float y = position.y;

    for (char c : text) {
        current_line += c;
        const bool has_non_ascii = std::any_of(current_line.begin(), current_line.end(), [](unsigned char v) {
            return v >= 0x80;
        });
        const sf::Font& active_font = (has_non_ascii && fallback_font_ != nullptr) ? *fallback_font_ : font_;
        sf::Text probe(active_font);
        probe.setString(sf::String::fromUtf8(current_line.begin(), current_line.end()));
        probe.setCharacterSize(style.character_size);
        probe.setStyle(style.bold ? sf::Text::Bold : sf::Text::Regular);

        if (probe.getLocalBounds().size.x > max_width) {
            DrawText(window, current_line, {position.x, y}, style, TextAlignment::Left);
            y += line_height;
            current_line.clear();
            current_line += c;
        }
    }

    if (!current_line.empty()) {
        DrawText(window, current_line, {position.x, y}, style, TextAlignment::Left);
    }
}

// ============================================================================
// 【PixelFontRenderer::DrawCenteredText】绘制居中文本
// ============================================================================
void PixelFontRenderer::DrawCenteredText(sf::RenderWindow& window,
                                         const std::string& text,
                                         const sf::Vector2f& center,
                                         const TextStyle& style) const {
    DrawText(window, text, center, style, TextAlignment::Center);
}

void PixelFontRenderer::DrawTextWithFallback(sf::RenderWindow& window,
                                             const std::string& text,
                                             const sf::Vector2f& position,
                                             const TextStyle& style,
                                             TextAlignment alignment) const {
    if (!font_loaded_ || text.empty()) return;

    const sf::Vector2f measured = MeasureText(text, style);
    float start_x = position.x;
    if (alignment == TextAlignment::Center) {
        start_x -= measured.x / 2.0f;
    } else if (alignment == TextAlignment::Right) {
        start_x -= measured.x;
    }

    sf::Text txt(font_);
    txt.setCharacterSize(style.character_size);
    txt.setStyle(style.bold ? sf::Text::Bold : sf::Text::Regular);
    txt.setFillColor(style.fill_color);
    if (style.outline_thickness > 0.0f && style.outline_color != sf::Color::Transparent) {
        txt.setOutlineColor(style.outline_color);
        txt.setOutlineThickness(style.outline_thickness);
    }

    float cursor_x = start_x;
    const float snapped_y = std::round(position.y);
    const sf::String u32 = sf::String::fromUtf8(text.begin(), text.end());
    sf::Text shadow_txt(txt);
    shadow_txt.setFillColor(style.shadow_color);

    const auto DrawOne = [&](const sf::Font& active_font, const sf::String& s, const sf::Color& fill_override) {
        txt.setFont(active_font);
        txt.setString(s);
        txt.setFillColor(fill_override);
        if (style.shadow_color != sf::Color::Transparent) {
            shadow_txt.setFont(active_font);
            shadow_txt.setString(s);
            shadow_txt.setPosition({std::round(cursor_x + style.shadow_offset.x),
                                    std::round(snapped_y + style.shadow_offset.y)});
            window.draw(shadow_txt, sf::RenderStates::Default);
        }
        txt.setPosition({std::round(cursor_x), snapped_y});
        window.draw(txt, sf::RenderStates::Default);
        cursor_x += txt.getLocalBounds().size.x;
    };

    for (std::size_t i = 0; i < u32.getSize(); ++i) {
        const std::uint32_t cp = u32[i];
        const bool is_ascii = cp >= 0x20u && cp <= 0x7Eu;
        const sf::Font* preferred = (is_ascii || fallback_font_ == nullptr) ? &font_ : fallback_font_;

        const auto& g = preferred->getGlyph(cp, style.character_size, style.bold, style.outline_thickness);
        const bool glyph_ok = (g.advance > 0.0f) || (g.bounds.size.x > 0.0f);

        if (glyph_ok) {
            DrawOne(*preferred, sf::String(static_cast<char32_t>(cp)), style.fill_color);
            continue;
        }

        const auto& g2 = font_.getGlyph(cp, style.character_size, style.bold, style.outline_thickness);
        const bool glyph_ok2 = (g2.advance > 0.0f) || (g2.bounds.size.x > 0.0f);
        if (glyph_ok2) {
            DrawOne(font_, sf::String(static_cast<char32_t>(cp)), style.fill_color);
            continue;
        }

        // 缺失字形占位符
        const std::string ph = "[?]";
        const sf::String placeholder = sf::String::fromUtf8(ph.begin(), ph.end());
        DrawOne(font_, placeholder, ColorPalette::DarkGray);
    }
}

}  // namespace CloudSeamanor::engine
