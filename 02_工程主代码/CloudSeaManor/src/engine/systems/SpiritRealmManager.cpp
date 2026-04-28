#include "CloudSeamanor/engine/systems/SpiritRealmManager.hpp"

#include "CloudSeamanor/Interactable.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

void SpiritRealmManager::RefreshDailyNodes(
    GameWorldState& world_state,
    const CloudSeamanor::domain::CloudSystem& cloud_system) const {
    auto& interactables = world_state.MutableInteractables();
    const auto remove_begin = std::remove_if(
        interactables.begin(),
        interactables.end(),
        [](const CloudSeamanor::domain::Interactable& obj) {
            return obj.Label() == "Spirit Plant Rare";
        });
    if (remove_begin != interactables.end()) {
        interactables.erase(remove_begin, interactables.end());
    }
    if (cloud_system.CurrentState() != CloudSeamanor::domain::CloudState::Tide) {
        return;
    }
    interactables.emplace_back(
        sf::Vector2f(1010.0f, 180.0f),
        sf::Vector2f(48.0f, 48.0f),
        CloudSeamanor::domain::InteractableType::GatheringNode,
        "Spirit Plant Rare",
        "star_fragment",
        1);
}

} // namespace CloudSeamanor::engine

