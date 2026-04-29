#include "../catch2Compat.hpp"

#include "CloudSeamanor/engine/PixelUiPanel.hpp"

using CloudSeamanor::engine::PixelUiPanel;

TEST_CASE("PixelUiPanel drag starts only on title bar") {
    PixelUiPanel panel({{100.0f, 100.0f}, {240.0f, 160.0f}}, "Test", true);
    panel.SetVisible(true);
    panel.Open();

    CHECK(panel.OnMousePressed(120.0f, 110.0f));
    panel.OnMouseMoved(200.0f, 180.0f);
    panel.OnMouseReleased();

    const auto rect = panel.GetRect();
    CHECK(rect.position.x != 100.0f);
    CHECK(rect.position.y != 100.0f);
}

TEST_CASE("PixelUiPanel drag is ignored outside title bar") {
    PixelUiPanel panel({{80.0f, 80.0f}, {220.0f, 140.0f}}, "Test", true);
    panel.SetVisible(true);
    panel.Open();

    CHECK_FALSE(panel.OnMousePressed(100.0f, 140.0f));  // body area
    panel.OnMouseMoved(180.0f, 220.0f);

    const auto rect = panel.GetRect();
    CHECK_FLOAT_EQ(rect.position.x, 80.0f, 0.001f);
    CHECK_FLOAT_EQ(rect.position.y, 80.0f, 0.001f);
}

TEST_CASE("PixelUiPanel color setter updates style") {
    PixelUiPanel panel({{0.0f, 0.0f}, {120.0f, 80.0f}}, "Color", true);
    panel.SetColors(sf::Color(10, 20, 30), sf::Color(200, 180, 160));

    const auto& style = panel.GetArtStyle().GetStyle();
    CHECK_EQ(style.fill_color, sf::Color(10, 20, 30));
    CHECK_EQ(style.outline_color, sf::Color(200, 180, 160));
}
