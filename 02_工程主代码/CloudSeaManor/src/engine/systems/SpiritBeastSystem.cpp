#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/engine/systems/SpiritBeastSystem.hpp"
#include "CloudSeamanor/GameConstants.hpp"

#include <cmath>

namespace CloudSeamanor::engine {

namespace {
float PersonalitySpeedScale(SpiritBeastPersonality personality) {
    switch (personality) {
    case SpiritBeastPersonality::Lively: return 1.15f;
    case SpiritBeastPersonality::Lazy: return 0.85f;
    case SpiritBeastPersonality::Curious: return 1.0f;
    }
    return 1.0f;
}

float PersonalityIdleScale(SpiritBeastPersonality personality) {
    switch (personality) {
    case SpiritBeastPersonality::Lively: return 0.7f;
    case SpiritBeastPersonality::Lazy: return 1.4f;
    case SpiritBeastPersonality::Curious: return 1.0f;
    }
    return 1.0f;
}
} // namespace

// ============================================================================
// 【SpiritBeastSystem::UpdateFollow_】灵兽跟随状态更新
// ============================================================================
void SpiritBeastSystem::UpdateFollow_(
    GameWorldState& world_state,
    float delta_seconds
) {
    auto& beast = world_state.GetSpiritBeast();
    const auto player_pos_d = world_state.GetPlayer().GetPosition();
    const sf::Vector2f player_pos(player_pos_d.x, player_pos_d.y);
    sf::Vector2f beast_pos = beast.shape.getPosition();
    const sf::Vector2f to_player = player_pos - beast_pos;
    const float distance = std::sqrt(to_player.x * to_player.x + to_player.y * to_player.y);

    if (distance > GameConstants::SpiritBeast::FollowKeepDistance) {
        const sf::Vector2f dir = to_player / distance;
        const float speed_scale = PersonalitySpeedScale(beast.personality);
        beast_pos += dir * (delta_seconds * GameConstants::SpiritBeast::FollowSpeed * speed_scale);
        beast.shape.setPosition(beast_pos);
    }
}

// ============================================================================
// 【SpiritBeastSystem::UpdateWander_】灵兽游荡状态更新
// ============================================================================
void SpiritBeastSystem::UpdateWander_(
    GameWorldState& world_state,
    float delta_seconds
) {
    auto& beast = world_state.GetSpiritBeast();
    if (beast.patrol_points.empty()) {
        current_state_ = SpiritBeastState::Idle;
        idle_timer_ = GameConstants::SpiritBeast::IdleDuration;
        return;
    }

    sf::Vector2f beast_pos = beast.shape.getPosition();
    const sf::Vector2f target_pt = beast.patrol_points[beast.patrol_index];
    const sf::Vector2f to_target = target_pt - beast_pos;
    const float target_dist = std::sqrt(to_target.x * to_target.x + to_target.y * to_target.y);

    if (target_dist > GameConstants::SpiritBeast::WanderArrivalDistance) {
        const sf::Vector2f dir = to_target / target_dist;
        const float speed_scale = PersonalitySpeedScale(beast.personality);
        beast_pos += dir * (delta_seconds * GameConstants::SpiritBeast::WanderSpeed * speed_scale);
        beast.shape.setPosition(beast_pos);
    } else {
        beast.patrol_index =
            (beast.patrol_index + 1) % beast.patrol_points.size();
        current_state_ = SpiritBeastState::Idle;
        idle_timer_ = GameConstants::SpiritBeast::IdleDuration * PersonalityIdleScale(beast.personality);
    }
}

// ============================================================================
// 【SpiritBeastSystem::UpdateIdle_】灵兽待机状态更新
// ============================================================================
void SpiritBeastSystem::UpdateIdle_(
    GameWorldState& /*world_state*/,
    float delta_seconds
) {
    idle_timer_ -= delta_seconds;
    if (idle_timer_ <= 0.0f) {
        current_state_ = SpiritBeastState::Wander;
    }
}

// ============================================================================
// 【SpiritBeastSystem::Update】每帧更新灵兽
// ============================================================================
void SpiritBeastSystem::Update(
    GameWorldState& world_state,
    float delta_seconds
) {
    auto& beast = world_state.GetSpiritBeast();

    if (current_state_ == SpiritBeastState::Interact) {
        beast.interact_timer -= delta_seconds;
        if (beast.interact_timer <= 0.0f) {
            current_state_ = SpiritBeastState::Follow;
        }
        return;
    }

    if (current_state_ == SpiritBeastState::Follow) {
        UpdateFollow_(world_state, delta_seconds);
        return;
    }
    if (current_state_ == SpiritBeastState::Wander) {
        UpdateWander_(world_state, delta_seconds);
        return;
    }
    if (current_state_ == SpiritBeastState::Idle) {
        UpdateIdle_(world_state, delta_seconds);
        return;
    }

    RefreshSpiritBeastVisual(beast, world_state.GetInteraction().spirit_beast_highlighted);
}

// ============================================================================
// 【SpiritBeastSystem::SetState】设置灵兽状态
// ============================================================================
void SpiritBeastSystem::SetState(SpiritBeastState new_state) {
    current_state_ = new_state;
    if (new_state == SpiritBeastState::Idle) {
        idle_timer_ = GameConstants::SpiritBeast::IdleDuration;
    }
}

}  // namespace CloudSeamanor::engine
