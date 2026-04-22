#pragma once

// ============================================================================
// 【PixelArtStyle】像素美术工具 + 调色板
// ============================================================================
// Responsibilities:
// - 像素风格调色板常量定义（ColorPalette）
// - 使用 VertexArray 绘制 1px 像素风格边框和装饰
// - 整数倍缩放辅助（保持像素锐利）
// ============================================================================

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

#include "CloudSeamanor/GameClock.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace CloudSeamanor::engine {

// ============================================================================
// 【ColorPalette】像素 UI 调色板
// ============================================================================
struct ColorPalette {
    struct SeasonTheme {
        sf::Color background;
        sf::Color accent;
        sf::Color text;
        sf::Color highlight;
    };
    using Theme = SeasonTheme;
    // 基础色
    static constexpr sf::Color BackgroundWhite{253, 254, 248};   // #FEFEF8
    static constexpr sf::Color Cream{251, 244, 224};              // #FBF4E0
    static constexpr sf::Color LightCream{247, 239, 210};        // #F7EFD2
    static constexpr sf::Color DeepCream{232, 216, 168};         // #E8D8A8
    static constexpr sf::Color LightBrown{212, 168, 75};         // #D4A84B
    static constexpr sf::Color DeepBrown{139, 90, 43};           // #8B5A2B
    static constexpr sf::Color BrownOutline{92, 58, 30};         // #5C3A1E
    static constexpr sf::Color Black{45, 27, 14};                // #2D1B0E
    static constexpr sf::Color TextBrown{61, 37, 23};            // #3D2517
    static constexpr sf::Color LightGray{200, 192, 168};         // #C8C0A8
    static constexpr sf::Color DarkGray{110, 100, 84};           // #6E6454

    // 功能色
    static constexpr sf::Color HighlightYellow{255, 250, 205}; // LemonChiffon #FFFACD，hover 高亮
    static constexpr sf::Color ActiveGreen{144, 238, 144};      // LightGreen #90EE90，选中/活跃
    static constexpr sf::Color WarningPink{255, 182, 193};       // LightPink #FFB6C1，体力低
    static constexpr sf::Color DisabledGray{169, 169, 169};       // DarkGray
    static constexpr sf::Color SuccessGreen{144, 238, 144};       // 同 ActiveGreen
    static constexpr sf::Color TextWhite{255, 255, 240};         // Ivory #FFFFF0
    static constexpr sf::Color TitleBarLight{230, 215, 180};     // 标题栏浅棕底

    // 体力条状态色
    static constexpr sf::Color StaminaFull{100, 180, 220};       // 深蓝色，满体力
    static constexpr sf::Color StaminaNormal{120, 214, 138};     // 绿色，正常
    static constexpr sf::Color StaminaLow{255, 130, 130};         // 淡红色，低体力
    static constexpr sf::Color StaminaBarBg{42, 52, 64};         // 体力条背景
    static constexpr sf::Color StaminaBarOutline{120, 140, 156}; // 体力条描边

    // 金币色
    static constexpr sf::Color CoinGold{255, 215, 0};            // 金色
    static constexpr sf::Color CoinBg{40, 32, 24};               // 背景

    // 四季主题色
    struct Season {
        // 春 Spring
        static constexpr sf::Color SpringGreen{143, 188, 143};
        static constexpr sf::Color SpringPink{255, 182, 193};
        static constexpr sf::Color SpringYellow{255, 248, 220};
        static constexpr sf::Color SpringPurple{230, 230, 250};
        // 夏 Summer
        static constexpr sf::Color SummerGreen{46, 139, 87};
        static constexpr sf::Color SummerBlue{135, 206, 235};
        static constexpr sf::Color SummerYellow{255, 215, 0};
        static constexpr sf::Color SummerOrange{255, 165, 0};
        // 秋 Autumn
        static constexpr sf::Color AutumnGold{218, 165, 32};
        static constexpr sf::Color AutumnSienna{160, 82, 45};
        static constexpr sf::Color AutumnRosy{188, 143, 143};
        // 冬 Winter
        static constexpr sf::Color WinterGray{220, 220, 220};
        static constexpr sf::Color WinterBlue{173, 216, 230};
        static constexpr sf::Color WinterLavender{216, 191, 216};
        static constexpr sf::Color WinterSnow{255, 250, 250};
    };

    // 云海天气色
    struct Weather {
        static constexpr sf::Color ClearSky{76, 122, 168};
        static constexpr sf::Color ClearGround{143, 188, 143};
        static constexpr sf::Color MistSky{88, 104, 122};
        static constexpr sf::Color MistGround{160, 184, 168};
        static constexpr sf::Color DenseSky{52, 64, 79};
        static constexpr sf::Color DenseGround{96, 112, 96};
        static constexpr sf::Color TideSky{42, 34, 68};
        static constexpr sf::Color TideGround{155, 143, 188};
        static constexpr sf::Color TideGlow{255, 148, 200};      // 大潮特殊光晕
    };

    // 内阴影（比基准色深 2 度）
    [[nodiscard]] static sf::Color Darken(const sf::Color& c, std::uint8_t amount = 2) noexcept {
        return sf::Color(
            amount > c.r ? 0 : c.r - amount,
            amount > c.g ? 0 : c.g - amount,
            amount > c.b ? 0 : c.b - amount,
            c.a);
    }

    // 变亮
    [[nodiscard]] static sf::Color Lighten(const sf::Color& c, std::uint8_t amount = 20) noexcept {
        return sf::Color(
            c.r + amount > 255 ? 255 : c.r + amount,
            c.g + amount > 255 ? 255 : c.g + amount,
            c.b + amount > 255 ? 255 : c.b + amount,
            c.a);
    }

    [[nodiscard]] static SeasonTheme GetSeasonTheme(CloudSeamanor::domain::Season season) noexcept {
        using DomainSeason = CloudSeamanor::domain::Season;
        switch (season) {
        case DomainSeason::Spring:
            return {ColorPalette::Season::SpringYellow, ColorPalette::Season::SpringGreen, TextBrown, ColorPalette::Season::SpringPink};
        case DomainSeason::Summer:
            return {ColorPalette::Season::SummerBlue, ColorPalette::Season::SummerGreen, TextBrown, ColorPalette::Season::SummerOrange};
        case DomainSeason::Autumn:
            return {ColorPalette::Season::AutumnGold, ColorPalette::Season::AutumnSienna, TextBrown, ColorPalette::Season::AutumnRosy};
        case DomainSeason::Winter:
        default:
            return {ColorPalette::Season::WinterSnow, ColorPalette::Season::WinterBlue, TextBrown, ColorPalette::Season::WinterLavender};
        }
    }

    [[nodiscard]] static SeasonTheme GetSeasonTheme(std::string_view season) noexcept {
        if (season == "spring") {
            return GetSeasonTheme(CloudSeamanor::domain::Season::Spring);
        }
        if (season == "summer") {
            return GetSeasonTheme(CloudSeamanor::domain::Season::Summer);
        }
        if (season == "autumn") {
            return GetSeasonTheme(CloudSeamanor::domain::Season::Autumn);
        }
        return GetSeasonTheme(CloudSeamanor::domain::Season::Winter);
    }
};

// ============================================================================
// 【PixelBorderStyle】像素边框风格
// ============================================================================
struct PixelBorderStyle {
    sf::Color fill_color = ColorPalette::Cream;
    sf::Color outline_color = ColorPalette::BrownOutline;
    sf::Color inner_shadow_color = ColorPalette::DeepCream;
    float corner_size = 8.0f;
    float thickness = 1.0f;
    bool draw_inner_shadow = true;
};

// ============================================================================
// 【PixelArtStyle】像素风格绘制工具
// ============================================================================
class PixelArtStyle {
public:
    explicit PixelArtStyle(const PixelBorderStyle& style = {});

    // ========================================================================
    // 【边框绘制】
    // ========================================================================

    /**
     * @brief 绘制像素风格矩形边框（4 个角块 + 4 条边）
     * @param target 绘制到的 VertexArray（应传入 VertexArray 引用）
     * @param rect 矩形位置和大小
     * @note 不会清空 target，调用方负责创建新的 VertexArray
     */
    void DrawPixelBorder(sf::VertexArray& target, const sf::FloatRect& rect) const;
    void DrawPixelBorder(sf::RenderTarget& target,
                         const sf::FloatRect& rect,
                         float border_width = 1.0f) const;

    /**
     * @brief 绘制带填充的像素风格矩形面板
     * @param target 绘制到的 VertexArray
     * @param rect 矩形位置和大小
     */
    void DrawPixelPanel(sf::VertexArray& target, const sf::FloatRect& rect) const;

    /**
     * @brief 绘制像素风格标题栏
     * @param target 绘制到的 VertexArray
     * @param rect 面板矩形
     * @param title_height 标题栏高度
     */
    void DrawPixelTitleBar(sf::VertexArray& target,
                            const sf::FloatRect& rect,
                            float title_height) const;

    /**
     * @brief 绘制像素风格角装饰（用于面板四角装饰）
     * @param target 绘制到的 VertexArray
     * @param pos 角位置
     * @param corner 方位（0=左上,1=右上,2=左下,3=右下）
     */
    void DrawPixelCorner(sf::VertexArray& target,
                          const sf::Vector2f& pos,
                          int corner) const;

    /**
     * @brief 绘制像素风格分割线
     * @param target 绘制到的 VertexArray
     * @param start 起点
     * @param end 终点
     */
    void DrawPixelLine(sf::VertexArray& target,
                       const sf::Vector2f& start,
                       const sf::Vector2f& end) const;
    void DrawCoinIcon(sf::RenderTarget& target,
                      const sf::Vector2f& position,
                      float size = 12.0f) const;

    // ========================================================================
    // 【工具方法】
    // ========================================================================

    /**
     * @brief 获取整数倍缩放值（保持像素锐利）
     * @param base 原始尺寸
     * @param scale 缩放倍数
     * @return 缩放后的整数尺寸
     */
    [[nodiscard]] static int ScaleInt(int base, int scale) noexcept {
        return base * scale;
    }

    /**
     * @brief 计算面板内可放置内容的区域（去除边框）
     * @param rect 面板矩形
     * @return 内容区域
     */
    [[nodiscard]] static sf::FloatRect GetInnerRect(const sf::FloatRect& rect) noexcept {
        const float t = 1.0f;
        return sf::FloatRect{
            {rect.position.x + t, rect.position.y + t},
            {rect.size.x - t * 2.0f, rect.size.y - t * 2.0f}
        };
    }

    // ========================================================================
    // 【属性】
    // ========================================================================
    [[nodiscard]] const PixelBorderStyle& GetStyle() const { return style_; }
    [[nodiscard]] PixelBorderStyle& GetStyle() { return style_; }

private:
    PixelBorderStyle style_;
};

}  // namespace CloudSeamanor::engine
