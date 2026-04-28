#include "CloudSeamanor/engine/systems/SpiritBeastSystem.hpp"
#include "CloudSeamanor/GameConstants.hpp"
#include "CloudSeamanor/GameAppSpiritBeast.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

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

sf::Vector2f NormalizedOrZero(const sf::Vector2f& v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len <= 0.0001f) {
        return {0.0f, 0.0f};
    }
    return v / len;
}
} // namespace

// ============================================================================
// 【SpiritBeastSystem::UpdateFollow_】灵兽跟随状态更新
// ============================================================================
void SpiritBeastSystem::UpdateFollow_(
    GameWorldState& world_state,
    float delta_seconds
) {
    auto& beast = world_state.MutableSpiritBeast();
    const auto player_pos_d = world_state.GetPlayer().GetPosition();
    const sf::Vector2f player_pos(player_pos_d.x, player_pos_d.y);
    sf::Vector2f beast_pos = CloudSeamanor::adapter::ToSf(beast.position);
    const sf::Vector2f to_player = player_pos - beast_pos;
    const float distance = std::sqrt(to_player.x * to_player.x + to_player.y * to_player.y);

    float player_speed = 0.0f;
    if (has_last_player_position_ && delta_seconds > 0.0f) {
        const sf::Vector2f delta = player_pos - last_player_position_;
        player_speed = std::sqrt(delta.x * delta.x + delta.y * delta.y) / delta_seconds;
    }
    last_player_position_ = player_pos;
    has_last_player_position_ = true;

    const float elastic_keep_distance = GameConstants::SpiritBeast::FollowKeepDistance
        + std::min(92.0f, player_speed * 0.16f);
    if (distance > elastic_keep_distance) {
        // BE-078: 多灵兽分散策略（槽位编队）
        // 当前运行时主灵兽仍是单实例，但会依据地图内 Spirit Beast 节点数量
        // 选择不同跟随槽位，避免多个灵兽节点叠在玩家同一侧。
        int beast_slot_count = 1;
        for (const auto& obj : world_state.GetInteractables()) {
            if (obj.Label() == "Spirit Beast") {
                ++beast_slot_count;
            }
        }
        beast_slot_count = std::clamp(beast_slot_count, 1, 4);
        const int preferred_slot =
            std::clamp(static_cast<int>(beast.personality), 0, beast_slot_count - 1);
        const std::array<sf::Vector2f, 4> slot_offsets{
            sf::Vector2f(-48.0f, 18.0f),
            sf::Vector2f(42.0f, 20.0f),
            sf::Vector2f(-30.0f, -26.0f),
            sf::Vector2f(26.0f, -28.0f),
        };
        const sf::Vector2f slot_target = player_pos + slot_offsets[static_cast<std::size_t>(preferred_slot)];
        const sf::Vector2f to_slot = slot_target - beast_pos;
        const sf::Vector2f dir = NormalizedOrZero(to_slot);
        const float speed_scale = PersonalitySpeedScale(beast.personality);
        sf::Vector2f next_pos =
            beast_pos + dir * (delta_seconds * GameConstants::SpiritBeast::FollowSpeed * speed_scale);

        // 轻量障碍绕行：若目标点落入障碍，尝试切向偏移。
        for (const auto& obstacle : world_state.GetObstacleBounds()) {
            if (obstacle.contains(next_pos)) {
                const sf::Vector2f tangent(-dir.y, dir.x);
                next_pos = beast_pos + tangent * (delta_seconds * GameConstants::SpiritBeast::FollowSpeed * 0.85f);
                break;
            }
        }

        // 避免与 NPC 贴脸重叠。
        for (const auto& npc : world_state.GetNpcs()) {
            const sf::Vector2f npc_delta = next_pos - CloudSeamanor::adapter::ToSf(npc.position);
            const float npc_dist = std::sqrt(npc_delta.x * npc_delta.x + npc_delta.y * npc_delta.y);
            if (npc_dist < 24.0f && npc_dist > 0.001f) {
                const sf::Vector2f push = npc_delta / npc_dist;
                next_pos += push * (24.0f - npc_dist);
            }
        }

        // 防重叠：与“其他灵兽节点”做分离力，避免视觉叠在一起。
        sf::Vector2f separate_force{0.0f, 0.0f};
        for (const auto& obj : world_state.GetInteractables()) {
            if (obj.Label() != "Spirit Beast") {
                continue;
            }
            const sf::Vector2f other = obj.Shape().getPosition();
            const sf::Vector2f delta = next_pos - other;
            const float d = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (d > 0.001f && d < 46.0f) {
                separate_force += (delta / d) * ((46.0f - d) / 46.0f);
            }
        }
        if (separate_force.x != 0.0f || separate_force.y != 0.0f) {
            next_pos += NormalizedOrZero(separate_force)
                * (delta_seconds * GameConstants::SpiritBeast::FollowSpeed * 0.75f);
        }
        beast_pos = next_pos;
        beast.position = CloudSeamanor::adapter::ToDomain(beast_pos);
    }
}

// ============================================================================
// 【SpiritBeastSystem::UpdateWander_】灵兽游荡状态更新
// ============================================================================
void SpiritBeastSystem::UpdateWander_(
    GameWorldState& world_state,
    float delta_seconds
) {
    auto& beast = world_state.MutableSpiritBeast();
    if (beast.patrol_points.empty()) {
        current_state_ = SpiritBeastState::Idle;
        idle_timer_ = GameConstants::SpiritBeast::IdleDuration;
        return;
    }

    sf::Vector2f beast_pos = CloudSeamanor::adapter::ToSf(beast.position);
    const sf::Vector2f target_pt = CloudSeamanor::adapter::ToSf(beast.patrol_points[beast.patrol_index]);
    const sf::Vector2f to_target = target_pt - beast_pos;
    const float target_dist = std::sqrt(to_target.x * to_target.x + to_target.y * to_target.y);

    if (target_dist > GameConstants::SpiritBeast::WanderArrivalDistance) {
        const sf::Vector2f dir = to_target / target_dist;
        const float speed_scale = PersonalitySpeedScale(beast.personality);
        beast_pos += dir * (delta_seconds * GameConstants::SpiritBeast::WanderSpeed * speed_scale);
        beast.position = CloudSeamanor::adapter::ToDomain(beast_pos);
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
    auto& beast = world_state.MutableSpiritBeast();

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

    RefreshSpiritBeastVisual(beast, world_state.MutableInteraction().spirit_beast_highlighted);
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
