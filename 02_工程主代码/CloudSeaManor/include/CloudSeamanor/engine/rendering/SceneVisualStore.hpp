#pragma once

#include "CloudSeamanor/engine/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <vector>

namespace CloudSeamanor::engine {

class SceneVisualStore {
public:
    void SyncTeaPlots(const std::vector<TeaPlot>& tea_plots);
    void SyncNpcs(const std::vector<NpcActor>& npcs);
    void SyncSpiritBeast(const SpiritBeast& beast);

    [[nodiscard]] const std::vector<sf::RectangleShape>& TeaPlotShapes() const { return tea_plot_shapes_; }
    [[nodiscard]] const std::vector<sf::RectangleShape>& NpcShapes() const { return npc_shapes_; }
    [[nodiscard]] const sf::CircleShape& SpiritBeastShape() const { return spirit_beast_shape_; }

private:
    std::vector<sf::RectangleShape> tea_plot_shapes_;
    std::vector<sf::RectangleShape> npc_shapes_;
    sf::CircleShape spirit_beast_shape_;
};

}  // namespace CloudSeamanor::engine
