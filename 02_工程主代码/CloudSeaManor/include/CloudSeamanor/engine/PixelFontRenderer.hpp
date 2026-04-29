#pragma once

// ============================================================================
// 【PixelFontRenderer】像素字体渲染
// ============================================================================
// Responsibilities:
// - 位图字形生成 TextureAtlas（从 TTF 模拟像素字体）
// - 字符串绘制（支持中英文混合）
// - 字形缓存避免重复生成
// - 支持描边和阴影
//
// 当前实现策略：
// 使用 SFML 内置 TTF 字体但通过缩放模拟像素感，
// 未来可替换为真实位图字形 atlas。
// ============================================================================

#include "CloudSeamanor/engine/PixelArtStyle.hpp"
#include "CloudSeamanor/engine/PixelUiConfig.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <string>
#include <unordered_map>

namespace CloudSeamanor::engine {

enum class TextAlignment {
    Left,
    Center,
    Right
};

// ============================================================================
// 【TextStyle】文本样式
// ============================================================================
struct TextStyle {
    unsigned int character_size = 14;
    sf::Color fill_color = ColorPalette::TextBrown;
    sf::Color outline_color = sf::Color::Transparent;
    sf::Color shadow_color = sf::Color::Transparent;
    sf::Vector2f shadow_offset{1.0f, 1.0f};
    float outline_thickness = 0.0f;
    bool bold = false;

    static TextStyle Default() {
        return {
            .character_size = 14,
            .fill_color = ColorPalette::TextBrown,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = false
        };
    }

    static TextStyle Dialogue() {
        return {
            .character_size = 14,
            .fill_color = ColorPalette::TextBrown,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = false
        };
    }

    static TextStyle NpcName() {
        return {
            .character_size = 16,
            .fill_color = ColorPalette::DeepBrown,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = true
        };
    }

    static TextStyle PanelTitle() {
        return {
            .character_size = 16,
            .fill_color = ColorPalette::TextBrown,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = true
        };
    }

    static TextStyle TopRightInfo() {
        return {
            .character_size = 12,
            .fill_color = ColorPalette::Black,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = false
        };
    }

    static TextStyle HotkeyHint() {
        return {
            .character_size = 10,
            .fill_color = ColorPalette::DarkGray,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = false
        };
    }

    static TextStyle ItemCount() {
        return {
            .character_size = 12,
            .fill_color = ColorPalette::TextBrown,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = true
        };
    }

    static TextStyle Tooltip() {
        return {
            .character_size = 11,
            .fill_color = ColorPalette::DeepBrown,
            .outline_color = sf::Color::Transparent,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.0f,
            .bold = false
        };
    }

    static TextStyle CoinText() {
        return {
            .character_size = 14,
            .fill_color = ColorPalette::CoinGold,
            .outline_color = ColorPalette::BrownOutline,
            .shadow_color = sf::Color::Transparent,
            .shadow_offset = {1.0f, 1.0f},
            .outline_thickness = 0.5f,
            .bold = true
        };
    }
};

// ============================================================================
// 【PixelFontRenderer】像素字体渲染器
// ============================================================================
class PixelFontRenderer {
public:
    explicit PixelFontRenderer(const sf::Font& font);
    PixelFontRenderer(const sf::Font& font, const sf::Font* fallback_font);
    void SetFallbackFont(const sf::Font* fallback_font) { fallback_font_ = fallback_font; }
    void SetUiScale(float scale) { ui_scale_ = std::max(0.75f, scale); }
    [[nodiscard]] float GetUiScale() const { return ui_scale_; }

    // ========================================================================
    // 【绘制方法】
    // ========================================================================

    /**
     * @brief 绘制带阴影的文本
     */
    void DrawText(sf::RenderWindow& window,
                  const std::string& text,
                  const sf::Vector2f& position,
                  const TextStyle& style = TextStyle::Default(),
                  TextAlignment alignment = TextAlignment::Left) const;

    /**
     * @brief 绘制单行文本（简化版）
     */
    void DrawText(sf::RenderWindow& window,
                  const std::string& text,
                  float x, float y,
                  const TextStyle& style = TextStyle::Default()) const;

    /**
     * @brief 计算文本边界
     */
    [[nodiscard]] sf::Vector2f MeasureText(const std::string& text,
                                           const TextStyle& style = TextStyle::Default()) const;

    /**
     * @brief 绘制多行文本（自动换行）
     * @param window 渲染目标
     * @param text 文本内容
     * @param position 左上角位置
     * @param max_width 最大宽度
     * @param style 样式
     * @param line_height 行高倍数
     */
    void DrawWrappedText(sf::RenderWindow& window,
                          const std::string& text,
                          const sf::Vector2f& position,
                          float max_width,
                          const TextStyle& style = TextStyle::Default(),
                          float line_height_multiplier = 1.5f) const;

    /**
     * @brief 绘制居中文本
     */
    void DrawCenteredText(sf::RenderWindow& window,
                          const std::string& text,
                          const sf::Vector2f& center,
                          const TextStyle& style = TextStyle::Default()) const;
    void DrawTextWithFallback(sf::RenderWindow& window,
                              const std::string& text,
                              const sf::Vector2f& position,
                              const TextStyle& style = TextStyle::Default(),
                              TextAlignment alignment = TextAlignment::Left) const;

    // ========================================================================
    // 【属性】
    // ========================================================================
    [[nodiscard]] const sf::Font& GetFont() const { return font_; }
    [[nodiscard]] bool IsLoaded() const { return font_loaded_; }

private:
    [[nodiscard]] unsigned int ScaledCharacterSize_(unsigned int base) const;
    [[nodiscard]] float ScaledOutlineThickness_(float base) const;
    [[nodiscard]] sf::Vector2f ScaledShadowOffset_(const sf::Vector2f& base) const;

    const sf::Font& font_;
    const sf::Font* fallback_font_ = nullptr;
    bool font_loaded_ = false;
    float ui_scale_ = 1.0f;
};

}  // namespace CloudSeamanor::engine
