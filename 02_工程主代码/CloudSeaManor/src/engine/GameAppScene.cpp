#include "CloudSeamanor/GameAppScene.hpp"

#include "CloudSeamanor/GameAppFarming.hpp"
#include "CloudSeamanor/GameAppSpiritBeast.hpp"
#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/PickupDrop.hpp"
#include "CloudSeamanor/Player.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/TmxMap.hpp"

#include <filesystem>
#include <fstream>

namespace CloudSeamanor::engine {

namespace {

void UpdateInteractableHighlightState(
    const CloudSeamanor::domain::Player& player,
    std::vector<CloudSeamanor::domain::Interactable>& interactables,
    int& highlighted_index) {
    highlighted_index = -1;
    for (std::size_t i = 0; i < interactables.size(); ++i) {
        const bool highlighted = interactables[i].IsPlayerInRange(player.Bounds());
        interactables[i].SetHighlighted(highlighted);
        if (highlighted && highlighted_index == -1) {
            highlighted_index = static_cast<int>(i);
        }
    }
}

void UpdatePlotHighlightState(
    const CloudSeamanor::domain::Player& player,
    std::vector<TeaPlot>& tea_plots,
    int& highlighted_plot_index,
    const std::function<void(TeaPlot&, bool)>& refresh_tea_plot_visual) {
    highlighted_plot_index = -1;
    for (std::size_t i = 0; i < tea_plots.size(); ++i) {
        const sf::FloatRect plot_bounds(
            CloudSeamanor::adapter::ToSf(tea_plots[i].position),
            CloudSeamanor::adapter::ToSf(tea_plots[i].size));
        const bool highlighted = CloudSeamanor::domain::Intersection(
            CloudSeamanor::adapter::ToDomain(plot_bounds),
            player.Bounds()).has_value();
        refresh_tea_plot_visual(tea_plots[i], highlighted);
        if (highlighted && highlighted_plot_index == -1) {
            highlighted_plot_index = static_cast<int>(i);
        }
    }
}

void UpdateNpcHighlightState(
    const CloudSeamanor::domain::Player& player,
    std::vector<NpcActor>& npcs,
    int& highlighted_npc_index) {
    highlighted_npc_index = -1;
    for (std::size_t i = 0; i < npcs.size(); ++i) {
        if (!npcs[i].visible) {
            npcs[i].outline_rgba = npcs[i].base_outline_rgba;
            npcs[i].outline_thickness = 2.0f;
            continue;
        }
        const sf::FloatRect npc_bounds(
            CloudSeamanor::adapter::ToSf(npcs[i].position),
            CloudSeamanor::adapter::ToSf(npcs[i].size));
        const bool highlighted = CloudSeamanor::domain::Intersection(
            CloudSeamanor::adapter::ToDomain(npc_bounds),
            player.Bounds()).has_value();
        npcs[i].outline_rgba = highlighted ? PackRgba(255, 255, 255) : npcs[i].base_outline_rgba;
        npcs[i].outline_thickness = highlighted ? 4.0f : 2.0f;
        if (highlighted && highlighted_npc_index == -1) {
            highlighted_npc_index = static_cast<int>(i);
        }
    }
}

void UpdateSpiritBeastHighlightState(
    const CloudSeamanor::domain::Player& player,
    SpiritBeast& spirit_beast,
    bool& spirit_beast_highlighted,
    const std::function<void(SpiritBeast&, bool)>& refresh_spirit_beast_visual) {
    const sf::FloatRect beast_bounds(
        {spirit_beast.position.x - spirit_beast.radius,
         spirit_beast.position.y - spirit_beast.radius},
        {spirit_beast.radius * 2.0f,
         spirit_beast.radius * 2.0f});
    spirit_beast_highlighted = CloudSeamanor::domain::Intersection(
        CloudSeamanor::adapter::ToDomain(beast_bounds),
        player.Bounds()).has_value();
    refresh_spirit_beast_visual(spirit_beast, spirit_beast_highlighted);
}

} // namespace

bool ValidateDataAsset(const std::filesystem::path& path,
                       std::string_view asset_label,
                       const std::function<void(std::string_view)>& log_error,
                       const std::function<void(std::string_view)>& log_info) {
    if (!std::filesystem::exists(path)) {
        log_error(std::string(asset_label) + " not found: " + path.string());
        return false;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        log_error(std::string(asset_label) + " could not be opened: " + path.string());
        return false;
    }

    stream.seekg(0, std::ios::end);
    const auto size = stream.tellg();
    if (size <= 0) {
        log_error(std::string(asset_label) + " is empty: " + path.string());
        return false;
    }

    log_info(std::string(asset_label) + " loaded successfully: " + path.string());
    return true;
}

void BuildSceneFallback(std::vector<sf::RectangleShape>& ground_tiles,
                        std::vector<sf::RectangleShape>& obstacle_shapes,
                        std::vector<sf::FloatRect>& obstacle_bounds,
                        std::vector<CloudSeamanor::domain::Interactable>& interactables,
                        std::vector<CloudSeamanor::domain::PickupDrop>& pickups) {
    ground_tiles.clear();
    obstacle_shapes.clear();
    obstacle_bounds.clear();
    interactables.clear();
    pickups.clear();

    // 8*12 的后备地面瓦片数量固定，预分配避免 push_back 扩容。
    ground_tiles.reserve(8 * 12);

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 12; ++x) {
            sf::RectangleShape tile({64.0f, 64.0f});
            tile.setPosition({40.0f + x * 64.0f, 40.0f + y * 64.0f});
            tile.setFillColor(((x + y) % 3 == 0) ? sf::Color(92, 144, 96) : sf::Color(74, 124, 74));
            tile.setOutlineThickness(-1.0f);
            tile.setOutlineColor(sf::Color(0, 0, 0, 20));
            ground_tiles.push_back(tile);
        }
    }

    sf::RectangleShape house({192.0f, 128.0f});
    house.setPosition({168.0f, 168.0f});
    house.setFillColor(sf::Color(112, 88, 72));
    house.setOutlineThickness(3.0f);
    house.setOutlineColor(sf::Color(64, 47, 35));
    obstacle_bounds.push_back(house.getGlobalBounds());
    obstacle_shapes.push_back(house);

    interactables.emplace_back(sf::Vector2f(200.0f, 360.0f), sf::Vector2f(48.0f, 48.0f), CloudSeamanor::domain::InteractableType::GatheringNode, "茶树丛", "TeaLeaf", 2);
    interactables.emplace_back(sf::Vector2f(520.0f, 360.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Workstation, "工作台", "TeaPack", 1);
    interactables.emplace_back(sf::Vector2f(720.0f, 456.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "仓库", "Wood", 1);
    interactables.emplace_back(sf::Vector2f(620.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Workstation, "Shop Stall", "", 1);
    interactables.emplace_back(sf::Vector2f(680.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "Purchaser", "", 1);
    interactables.emplace_back(sf::Vector2f(740.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "Mailbox", "", 1);
    interactables.emplace_back(sf::Vector2f(800.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::GatheringNode, "Spirit Gateway", "", 1);
    interactables.emplace_back(sf::Vector2f(860.0f, 180.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::GatheringNode, "Spirit Gateway Return", "", 1);
    interactables.emplace_back(sf::Vector2f(860.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Workstation, "General Store", "", 1);
    interactables.emplace_back(sf::Vector2f(920.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Workstation, "Tide Shop", "", 1);
    interactables.emplace_back(sf::Vector2f(980.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::GatheringNode, "Spirit Beast Zone", "spirit_dust", 1);
    interactables.emplace_back(sf::Vector2f(860.0f, 300.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "Greenhouse Gate", "", 1);
    interactables.emplace_back(sf::Vector2f(920.0f, 300.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "Inn Desk", "", 1);
    interactables.emplace_back(sf::Vector2f(980.0f, 300.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "Coop Barn", "", 1);
    interactables.emplace_back(sf::Vector2f(1040.0f, 300.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "Decoration Bench", "", 1);
    interactables.emplace_back(sf::Vector2f(1100.0f, 300.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::Storage, "Pet House", "", 1);
    interactables.emplace_back(sf::Vector2f(1040.0f, 120.0f), sf::Vector2f(56.0f, 56.0f), CloudSeamanor::domain::InteractableType::GatheringNode, "Spirit Beast Zone", "", 1);
}

void BuildSceneFromMap(const CloudSeamanor::infrastructure::TmxMap& tmx_map,
                       std::vector<sf::RectangleShape>& ground_tiles,
                       std::vector<sf::RectangleShape>& obstacle_shapes,
                       std::vector<sf::FloatRect>& obstacle_bounds,
                       std::vector<CloudSeamanor::domain::Interactable>& interactables,
                       std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
                       sf::FloatRect& world_bounds) {
    obstacle_shapes.clear();
    obstacle_bounds.clear();
    interactables.clear();
    pickups.clear();
    ground_tiles.clear();

    const sf::Vector2f world_size = tmx_map.WorldSize();
    world_bounds = sf::FloatRect({40.0f, 40.0f}, world_size);

    const int width = tmx_map.MapWidth();
    const int height = tmx_map.MapHeight();
    const int tile_width = tmx_map.TileWidth();
    const int tile_height = tmx_map.TileHeight();
    const auto& tiles = tmx_map.GroundTiles();

    if (width > 0 && height > 0) {
        ground_tiles.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int index = y * width + x;
            if (index < 0 || index >= static_cast<int>(tiles.size())) {
                continue;
            }

            sf::RectangleShape tile(sf::Vector2f(static_cast<float>(tile_width), static_cast<float>(tile_height)));
            tile.setPosition({40.0f + static_cast<float>(x * tile_width), 40.0f + static_cast<float>(y * tile_height)});
            const int tile_id = tiles[static_cast<std::size_t>(index)];
            tile.setFillColor(tile_id == 2 ? sf::Color(74, 124, 74) : (tile_id == 3 ? sf::Color(120, 98, 70) : sf::Color(92, 144, 96)));
            tile.setOutlineThickness(-1.0f);
            tile.setOutlineColor(sf::Color(0, 0, 0, 20));
            ground_tiles.push_back(tile);
        }
    }

    for (const auto& rect : tmx_map.Obstacles()) {
        sf::RectangleShape obstacle({rect.width, rect.height});
        obstacle.setPosition({40.0f + rect.x, 40.0f + rect.y});
        obstacle.setFillColor(sf::Color(96, 89, 81));
        obstacle.setOutlineThickness(2.0f);
        obstacle.setOutlineColor(sf::Color(56, 49, 44));
        obstacle_bounds.push_back(obstacle.getGlobalBounds());
        obstacle_shapes.push_back(obstacle);
    }

    for (const auto& entry : tmx_map.Interactables()) {
        const bool is_workstation = (entry.type == "Workstation");
        const bool is_storage = (entry.type == "Storage");
        const auto type = is_workstation
            ? CloudSeamanor::domain::InteractableType::Workstation
            : (is_storage ? CloudSeamanor::domain::InteractableType::Storage : CloudSeamanor::domain::InteractableType::GatheringNode);
        std::string label = entry.name.empty() ? "节点" : entry.name;
        if (entry.type == "spirit_gateway") {
            label = "Spirit Gateway";
        } else if (entry.type == "spirit_gateway_return") {
            label = "Spirit Gateway Return";
        } else if (entry.type == "spirit_plant") {
            label = "Spirit Plant";
        } else if (entry.type == "SpiritRoamArea" || entry.type == "spirit_beast_zone" || entry.type == "spirit_beast") {
            label = "Spirit Beast Zone";
        } else if (entry.type == "BossSpawn") {
            label = "Spirit Beast Zone";
        } else if (entry.type == "festival_booth") {
            label = "Festival Booth";
        }
        interactables.emplace_back(
            sf::Vector2f(40.0f + entry.rect.x, 40.0f + entry.rect.y),
            sf::Vector2f(entry.rect.width, entry.rect.height),
            type,
            label,
            entry.item,
            entry.count,
            entry.enemy_id);
    }
}

void BuildScene(CloudSeamanor::infrastructure::TmxMap& tmx_map,
                std::vector<sf::RectangleShape>& ground_tiles,
                std::vector<sf::RectangleShape>& obstacle_shapes,
                std::vector<sf::FloatRect>& obstacle_bounds,
                std::vector<CloudSeamanor::domain::Interactable>& interactables,
                std::vector<CloudSeamanor::domain::PickupDrop>& pickups,
                sf::FloatRect& world_bounds,
                const std::string& map_path,
                const std::function<void(std::string_view)>& log_info,
                const std::function<void(std::string_view)>& log_warning) {
    if (tmx_map.LoadFromFile(map_path)) {
        BuildSceneFromMap(tmx_map, ground_tiles, obstacle_shapes, obstacle_bounds, interactables, pickups, world_bounds);
        log_info("TMX map loaded successfully.");
        return;
    }

    BuildSceneFallback(ground_tiles, obstacle_shapes, obstacle_bounds, interactables, pickups);
    log_warning("TMX 地图加载失败，已使用后备场景。");
}

void UpdateHighlightedInteractable(const CloudSeamanor::domain::Player& player,
                                   std::vector<CloudSeamanor::domain::Interactable>& interactables,
                                   std::vector<TeaPlot>& tea_plots,
                                   std::vector<NpcActor>& npcs,
                                   SpiritBeast& spirit_beast,
                                   int& highlighted_index,
                                   int& highlighted_plot_index,
                                   int& highlighted_npc_index,
                                   bool& spirit_beast_highlighted,
                                   const std::function<void(TeaPlot&, bool)>& refresh_tea_plot_visual,
                                   const std::function<void(SpiritBeast&, bool)>& refresh_spirit_beast_visual) {
    UpdateInteractableHighlightState(player, interactables, highlighted_index);
    UpdatePlotHighlightState(player, tea_plots, highlighted_plot_index, refresh_tea_plot_visual);
    UpdateNpcHighlightState(player, npcs, highlighted_npc_index);
    UpdateSpiritBeastHighlightState(player, spirit_beast, spirit_beast_highlighted, refresh_spirit_beast_visual);
}

} // namespace CloudSeamanor::engine
