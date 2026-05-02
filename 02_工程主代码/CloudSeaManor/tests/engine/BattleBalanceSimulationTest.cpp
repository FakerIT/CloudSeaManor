#include "TestFramework.hpp"

#include <algorithm>
#include <random>
#include <vector>

namespace {

struct SimConfig {
    float energy_max = 100.0f;
    float energy_regen = 5.0f;
    float skill_cost = 15.0f;
    float skill_cd = 8.0f;
    float skill_base = 30.0f;
    float weapon_attack = 22.0f;
    float atk_weight = 0.35f;
    float purify_rate = 1.10f;
    float weather_mult = 1.20f;
    float enemy_pollution = 120.0f;
    float enemy_resist = 0.05f;
    float enemy_attack = 8.0f;
    float enemy_attack_cd = 4.0f;
    float crit_chance = 0.08f;
    float crit_mult = 1.5f;
};

float SimulateOneBattle_(std::mt19937& rng, const SimConfig& cfg) {
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    std::uniform_real_distribution<float> jitter(0.95f, 1.05f);

    float t = 0.0f;
    float energy = cfg.energy_max;
    float pollution = cfg.enemy_pollution;
    float next_skill = 0.0f;
    float next_enemy = cfg.enemy_attack_cd;
    constexpr float dt = 0.1f;

    while (t < 120.0f && pollution > 0.0f) {
        energy = std::min(cfg.energy_max, energy + cfg.energy_regen * dt);

        if (t >= next_enemy) {
            const float energy_cost = cfg.enemy_attack * 0.5f;
            energy = std::max(0.0f, energy - energy_cost);
            next_enemy += cfg.enemy_attack_cd;
        }

        if (t >= next_skill && energy >= cfg.skill_cost) {
            float damage = (cfg.skill_base + cfg.atk_weight * cfg.weapon_attack)
                * cfg.purify_rate
                * cfg.weather_mult
                * (1.0f - cfg.enemy_resist)
                * jitter(rng);
            if (u(rng) < cfg.crit_chance) {
                damage *= cfg.crit_mult;
            }
            pollution = std::max(0.0f, pollution - damage);
            energy -= cfg.skill_cost;
            next_skill += cfg.skill_cd;
        }

        t += dt;
    }
    return t;
}

} // namespace

namespace CloudSeamanor::engine {

TEST_CASE(BattleBalanceSimulation_Lv5CommonSpirit_AverageDurationInTargetRange) {
    std::mt19937 rng(42);
    SimConfig cfg;
    std::vector<float> durations;
    durations.reserve(500);

    for (int i = 0; i < 500; ++i) {
        durations.push_back(SimulateOneBattle_(rng, cfg));
    }

    float sum = 0.0f;
    for (float d : durations) {
        sum += d;
    }
    const float average = sum / static_cast<float>(durations.size());

    ASSERT_TRUE(average >= 15.0f);
    ASSERT_TRUE(average <= 30.0f);
    return true;
}

} // namespace CloudSeamanor::engine

