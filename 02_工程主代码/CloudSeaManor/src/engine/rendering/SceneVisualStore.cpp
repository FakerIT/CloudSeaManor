#include "CloudSeamanor/engine/rendering/SceneVisualStore.hpp"

namespace CloudSeamanor::engine {

void SceneVisualStore::SyncTeaPlots(const std::vector<TeaPlot>& tea_plots) {
    tea_plot_shapes_.clear();
    tea_plot_shapes_.reserve(tea_plots.size());
    for (const auto& plot : tea_plots) {
        sf::RectangleShape shape;
        shape.setSize(CloudSeamanor::adapter::ToSf(plot.size));
        shape.setPosition(CloudSeamanor::adapter::ToSf(plot.position));
        shape.setFillColor(CloudSeamanor::adapter::PackedRgbaToColor(plot.fill_rgba));
        shape.setOutlineColor(CloudSeamanor::adapter::PackedRgbaToColor(plot.outline_rgba));
        shape.setOutlineThickness(plot.outline_thickness);
        tea_plot_shapes_.push_back(shape);
    }
}

void SceneVisualStore::SyncNpcs(const std::vector<NpcActor>& npcs) {
    npc_shapes_.clear();
    npc_shapes_.reserve(npcs.size());
    for (const auto& npc : npcs) {
        sf::RectangleShape shape;
        shape.setSize(CloudSeamanor::adapter::ToSf(npc.size));
        shape.setPosition(CloudSeamanor::adapter::ToSf(npc.position));
        shape.setFillColor(CloudSeamanor::adapter::PackedRgbaToColor(npc.fill_rgba));
        shape.setOutlineColor(CloudSeamanor::adapter::PackedRgbaToColor(npc.outline_rgba));
        shape.setOutlineThickness(npc.outline_thickness);
        npc_shapes_.push_back(shape);
    }
}

void SceneVisualStore::SyncSpiritBeast(const SpiritBeast& beast) {
    sf::CircleShape shape(beast.radius, beast.point_count);
    shape.setOrigin({beast.radius, beast.radius});
    shape.setPosition(CloudSeamanor::adapter::ToSf(beast.position));
    shape.setFillColor(CloudSeamanor::adapter::PackedRgbaToColor(beast.fill_rgba));
    shape.setOutlineColor(CloudSeamanor::adapter::PackedRgbaToColor(beast.outline_rgba));
    shape.setOutlineThickness(beast.outline_thickness);
    spirit_beast_shape_ = shape;
}

}  // namespace CloudSeamanor::engine
