#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/PickupDrop.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <SFML/Graphics/Color.hpp>

#include <utility>

namespace CloudSeamanor::domain {

PickupDrop::PickupDrop(
    sf::Vector2f position,
    std::string item_id,
    int amount
) : item_id_(std::move(item_id)), amount_(amount) {
    shape_.setPosition(position);
    shape_.setSize({22.0f, 22.0f});
    shape_.setFillColor(sf::Color(255, 218, 110));
    shape_.setOutlineThickness(2.0f);
    shape_.setOutlineColor(sf::Color(128, 88, 24));
}

bool PickupDrop::IsCollectedBy(const RectF& player_bounds) const noexcept {
    return Intersection(adapter::ToDomain(shape_.getGlobalBounds()), player_bounds).has_value();
}

}  // namespace CloudSeamanor::domain

