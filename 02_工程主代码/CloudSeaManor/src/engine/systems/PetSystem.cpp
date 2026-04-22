#include "CloudSeamanor/engine/systems/PetSystem.hpp"

#include <cmath>
#include <string>

namespace CloudSeamanor::engine {

void PetSystem::Update(GameWorldState& world_state, float delta_seconds) const {
    if (!world_state.GetPetAdopted()) {
        return;
    }

    auto& beast = world_state.GetSpiritBeast();
    const auto player_pos_d = world_state.GetPlayer().GetPosition();
    const sf::Vector2f player_pos(player_pos_d.x, player_pos_d.y);
    sf::Vector2f pet_pos = beast.shape.getPosition();

    sf::Vector2f offset(32.0f, 18.0f);
    float speed = 70.0f;
    const std::string pet_type = world_state.GetPetType();
    if (pet_type == "cat") {
        offset = sf::Vector2f(26.0f, 20.0f);
        speed = 78.0f;
    } else if (pet_type == "dog") {
        offset = sf::Vector2f(34.0f, 24.0f);
        speed = 84.0f;
    } else if (pet_type == "bird") {
        offset = sf::Vector2f(20.0f, -12.0f);
        speed = 96.0f;
        const float t = world_state.GetSessionTime() * 6.0f;
        offset.y += std::sin(t) * 6.0f;
    }
    const sf::Vector2f target = player_pos + offset;
    const sf::Vector2f delta = target - pet_pos;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist > 2.0f) {
        pet_pos += (delta / dist) * (speed * delta_seconds);
        beast.shape.setPosition(pet_pos);
    }
}

} // namespace CloudSeamanor::engine
