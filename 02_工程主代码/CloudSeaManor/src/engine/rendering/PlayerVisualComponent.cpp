#include "CloudSeamanor/engine/rendering/PlayerVisualComponent.hpp"

#include <algorithm>
#include <cstdint>

namespace CloudSeamanor::engine {

void PlayerVisualComponent::SyncFromDomainPlayer(const CloudSeamanor::domain::Player& player) {
    const auto pos = player.GetPosition();
    const auto size = player.GetSize();
    shape_.setSize({size.x, size.y});
    shape_.setPosition({pos.x, pos.y});
    shape_.setFillColor(ResolveColor_(player));
}

sf::Color PlayerVisualComponent::ResolveColor_(const CloudSeamanor::domain::Player& player) const {
    using CloudSeamanor::domain::FacingDirection;
    sf::Color color(89, 195, 255);

    switch (player.Facing()) {
    case FacingDirection::Up:
    case FacingDirection::UpLeft:
    case FacingDirection::UpRight:
        color = sf::Color(126, 184, 255);
        break;
    case FacingDirection::Left:
    case FacingDirection::DownLeft:
        color = sf::Color(118, 220, 196);
        break;
    case FacingDirection::Right:
    case FacingDirection::DownRight:
        color = sf::Color(255, 190, 96);
        break;
    case FacingDirection::Down:
        color = sf::Color(89, 195, 255);
        break;
    }

    if (player.IsMoving()) {
        color.r = static_cast<std::uint8_t>(std::min(255, static_cast<int>(color.r) + 18));
        color.g = static_cast<std::uint8_t>(std::min(255, static_cast<int>(color.g) + 18));
        color.b = static_cast<std::uint8_t>(std::min(255, static_cast<int>(color.b) + 18));
    }
    return color;
}

} // namespace CloudSeamanor::engine
