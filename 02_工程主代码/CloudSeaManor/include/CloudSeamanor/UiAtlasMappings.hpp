#pragma once

#include <SFML/Graphics/Rect.hpp>

#include <string_view>

namespace CloudSeamanor::engine::atlas {

inline constexpr const char* kTinyTownTilemapPath =
    "assets/textures/third_party/kenney_tiny-town/Tilemap/tilemap_packed.png";

// ui_main.atlas.json -> frames
inline constexpr sf::IntRect kIconStamina({0, 0}, {16, 16});
inline constexpr sf::IntRect kIconCoin({16, 0}, {16, 16});
inline constexpr sf::IntRect kIconSpirit({32, 0}, {16, 16});
inline constexpr sf::IntRect kIconCloudClear({48, 0}, {16, 16});
inline constexpr sf::IntRect kIconCloudMist({64, 0}, {16, 16});
inline constexpr sf::IntRect kIconCloudDense({80, 0}, {16, 16});
inline constexpr sf::IntRect kIconCloudTide({96, 0}, {16, 16});
inline constexpr sf::IntRect kIconQuest({112, 0}, {16, 16});

inline constexpr sf::IntRect kBtnDefault0({0, 96}, {48, 32});
inline constexpr sf::IntRect kBtnDefault1({48, 96}, {48, 32});
inline constexpr sf::IntRect kBtnDefault2({96, 96}, {48, 32});
inline constexpr sf::IntRect kBtnDefault3({144, 96}, {48, 32});

inline constexpr sf::IntRect kPanelCornerTl({0, 160}, {8, 8});
inline constexpr sf::IntRect kPanelCornerTr({8, 160}, {8, 8});
inline constexpr sf::IntRect kPanelCornerBl({16, 160}, {8, 8});
inline constexpr sf::IntRect kPanelCornerBr({24, 160}, {8, 8});
inline constexpr sf::IntRect kPanelEdgeH({32, 160}, {8, 8});
inline constexpr sf::IntRect kPanelEdgeV({40, 160}, {8, 8});

// world placeholder mapping in kenney_tiny-town tilemap
inline constexpr sf::IntRect kWorldTileGrassA({0, 32}, {16, 16});
inline constexpr sf::IntRect kWorldTileGrassB({16, 32}, {16, 16});
inline constexpr sf::IntRect kWorldTileGrassC({32, 32}, {16, 16});
inline constexpr sf::IntRect kWorldPlayer({0, 48}, {16, 16});
inline constexpr sf::IntRect kWorldNpc({16, 48}, {16, 16});
inline constexpr sf::IntRect kWorldSpiritBeast({32, 48}, {16, 16});

inline sf::IntRect WeatherIconForText(std::string_view weather_text) {
    if (weather_text.find("雾") != std::string_view::npos
        || weather_text.find("mist") != std::string_view::npos
        || weather_text.find("Mist") != std::string_view::npos) {
        return kIconCloudMist;
    }
    if (weather_text.find("密") != std::string_view::npos
        || weather_text.find("浓") != std::string_view::npos
        || weather_text.find("dense") != std::string_view::npos
        || weather_text.find("Dense") != std::string_view::npos) {
        return kIconCloudDense;
    }
    if (weather_text.find("潮") != std::string_view::npos
        || weather_text.find("tide") != std::string_view::npos
        || weather_text.find("Tide") != std::string_view::npos) {
        return kIconCloudTide;
    }
    return kIconCloudClear;
}

}  // namespace CloudSeamanor::engine::atlas
