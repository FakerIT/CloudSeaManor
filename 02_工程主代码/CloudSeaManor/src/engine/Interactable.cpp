#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <SFML/Graphics/Color.hpp>

#include <utility>

namespace CloudSeamanor::engine {

Interactable::Interactable(
    sf::Vector2f position,
    sf::Vector2f size,
    InteractableType type,
    std::string label,
    std::string reward_item,
    int reward_amount,
    std::string enemy_id
) : type_(type),
    label_(std::move(label)),
    reward_item_(std::move(reward_item)),
    reward_amount_(reward_amount),
    enemy_id_(std::move(enemy_id)) {
    shape_.setPosition(position);
    shape_.setSize(size);
    RefreshVisualState(false);
}

void Interactable::SetHighlighted(bool highlighted) {
    RefreshVisualState(highlighted);
}

bool Interactable::IsPlayerInRange(
    const CloudSeamanor::domain::RectF& player_bounds,
    float extra_range
) const noexcept {
    CloudSeamanor::domain::RectF expanded = adapter::ToDomain(Bounds());
    expanded.position.x -= extra_range;
    expanded.position.y -= extra_range;
    expanded.size.x += extra_range * 2.0f;
    expanded.size.y += extra_range * 2.0f;
    return CloudSeamanor::domain::Intersection(expanded, player_bounds).has_value();
}

sf::FloatRect Interactable::Bounds() const noexcept {
    return shape_.getGlobalBounds();
}

std::string Interactable::TypeText() const {
    switch (type_) {
    case InteractableType::GatheringNode: return "采集点";
    case InteractableType::Workstation:   return "工作台";
    case InteractableType::Storage:       return "储物点";
    }
    return "未知";
}

void Interactable::RefreshVisualState(bool highlighted) {
    sf::Color base_fill;
    sf::Color outline;
    switch (type_) {
    case InteractableType::GatheringNode:
        base_fill = sf::Color(98, 168, 88);
        outline = sf::Color(46, 96, 40);
        break;
    case InteractableType::Workstation:
        base_fill = sf::Color(176, 136, 72);
        outline = sf::Color(94, 68, 34);
        break;
    case InteractableType::Storage:
        base_fill = sf::Color(118, 128, 156);
        outline = sf::Color(64, 72, 88);
        break;
    }

    shape_.setFillColor(base_fill);
    shape_.setOutlineColor(outline);
    shape_.setOutlineThickness(2.0f);
    if (highlighted) {
        shape_.setOutlineColor(sf::Color::White);
        shape_.setOutlineThickness(4.0f);
    }
}

}  // namespace CloudSeamanor::engine

