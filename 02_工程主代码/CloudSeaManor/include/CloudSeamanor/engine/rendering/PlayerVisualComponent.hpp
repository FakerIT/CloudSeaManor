#pragma once

#include "CloudSeamanor/domain/Player.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

namespace CloudSeamanor::engine {

class PlayerVisualComponent {
public:
    void SyncFromDomainPlayer(const CloudSeamanor::domain::Player& player);
    [[nodiscard]] const sf::RectangleShape& Shape() const noexcept { return shape_; }

private:
    [[nodiscard]] sf::Color ResolveColor_(const CloudSeamanor::domain::Player& player) const;

    sf::RectangleShape shape_;
};

} // namespace CloudSeamanor::engine
