// ============================================================================
// 【PixelArtStyleTest.cpp】ColorPalette 季节主题色测试
// ============================================================================

#include "../catch2Compat.hpp"
#include "CloudSeamanor/GameClock.hpp"
#include "CloudSeamanor/PixelArtStyle.hpp"

using CloudSeamanor::domain::Season;
using CloudSeamanor::engine::ColorPalette;

TEST_CASE("ColorPalette::GetSeasonTheme - returns 4x4 season colors")
{
    const auto spring = ColorPalette::GetSeasonTheme(Season::Spring);
    CHECK_EQ(spring.background, ColorPalette::Season::SpringYellow);
    CHECK_EQ(spring.accent, ColorPalette::Season::SpringGreen);
    CHECK_EQ(spring.text, ColorPalette::TextBrown);
    CHECK_EQ(spring.highlight, ColorPalette::Season::SpringPink);

    const auto summer = ColorPalette::GetSeasonTheme(Season::Summer);
    CHECK_EQ(summer.background, ColorPalette::Season::SummerBlue);
    CHECK_EQ(summer.accent, ColorPalette::Season::SummerGreen);
    CHECK_EQ(summer.text, ColorPalette::TextBrown);
    CHECK_EQ(summer.highlight, ColorPalette::Season::SummerOrange);

    const auto autumn = ColorPalette::GetSeasonTheme(Season::Autumn);
    CHECK_EQ(autumn.background, ColorPalette::Season::AutumnGold);
    CHECK_EQ(autumn.accent, ColorPalette::Season::AutumnSienna);
    CHECK_EQ(autumn.text, ColorPalette::TextBrown);
    CHECK_EQ(autumn.highlight, ColorPalette::Season::AutumnRosy);

    const auto winter = ColorPalette::GetSeasonTheme(Season::Winter);
    CHECK_EQ(winter.background, ColorPalette::Season::WinterSnow);
    CHECK_EQ(winter.accent, ColorPalette::Season::WinterBlue);
    CHECK_EQ(winter.text, ColorPalette::TextBrown);
    CHECK_EQ(winter.highlight, ColorPalette::Season::WinterLavender);
}

TEST_CASE("ColorPalette::GetSeasonTheme - string overload maps correctly")
{
    CHECK_EQ(ColorPalette::GetSeasonTheme("spring").accent, ColorPalette::Season::SpringGreen);
    CHECK_EQ(ColorPalette::GetSeasonTheme("summer").accent, ColorPalette::Season::SummerGreen);
    CHECK_EQ(ColorPalette::GetSeasonTheme("autumn").accent, ColorPalette::Season::AutumnSienna);
    CHECK_EQ(ColorPalette::GetSeasonTheme("winter").accent, ColorPalette::Season::WinterBlue);
    CHECK_EQ(ColorPalette::GetSeasonTheme("unknown").accent, ColorPalette::Season::WinterBlue);
}
