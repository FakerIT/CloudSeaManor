#include "CloudSeamanor/app/GameAppSpiritBeast.hpp"

#include "CloudSeamanor/domain/GameClock.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"

#include <cmath>
#include <cstdint>

namespace CloudSeamanor::engine {

void RefreshSpiritBeastVisual(SpiritBeast& spirit_beast, bool highlighted) {
    std::uint32_t fill_rgba = PackRgba(196, 235, 255);
    switch (spirit_beast.state) {
    case SpiritBeastState::Idle:
        fill_rgba = PackRgba(196, 235, 255);
        break;
    case SpiritBeastState::Wander:
        fill_rgba = PackRgba(168, 220, 255);
        break;
    case SpiritBeastState::Follow:
        fill_rgba = PackRgba(255, 214, 168);
        break;
    case SpiritBeastState::Interact:
        fill_rgba = PackRgba(255, 174, 206);
        break;
    }
    spirit_beast.fill_rgba = fill_rgba;
    spirit_beast.outline_thickness = highlighted ? 4.0f : 2.0f;
    spirit_beast.outline_rgba = highlighted ? PackRgba(255, 255, 255) : PackRgba(86, 114, 148);
}

void BuildSpiritBeast(SpiritBeast& spirit_beast,
                      const CloudSeamanor::domain::GameClock& clock,
                      bool& spirit_beast_watered_today,
                      bool& spirit_beast_highlighted) {
    spirit_beast.custom_name = "灵团";
    spirit_beast.personality = SpiritBeastPersonality::Lively;
    spirit_beast.favor = 0;
    spirit_beast.dispatched_for_pest_control = false;
    spirit_beast.radius = GameConstants::SpiritBeast::BodyRadius;
    spirit_beast.point_count = GameConstants::SpiritBeast::BodyPointCount;
    spirit_beast.home_position = {620.0f, 250.0f};
    spirit_beast.position = spirit_beast.home_position;
    spirit_beast.patrol_points = {
        {560.0f, 240.0f},
        {650.0f, 250.0f},
        {700.0f, 330.0f},
        {560.0f, 340.0f}
    };
    spirit_beast.patrol_index = 0;
    spirit_beast.state = SpiritBeastState::Wander;
    spirit_beast.idle_timer = 0.0f;
    spirit_beast.interact_timer = 0.0f;
    spirit_beast.daily_interacted = false;
    spirit_beast.last_interaction_day = clock.Day();
    spirit_beast_watered_today = false;
    spirit_beast_highlighted = false;
    RefreshSpiritBeastVisual(spirit_beast, spirit_beast_highlighted);
}

void SpawnHeartParticles(CloudSeamanor::domain::Vec2f center, std::vector<HeartParticle>& heart_particles) {
    constexpr float offsets[5] = {-18.0f, -9.0f, 0.0f, 9.0f, 18.0f};
    for (float offset : offsets) {
        HeartParticle particle;
        particle.position = {center.x + offset, center.y - 6.0f};
        particle.radius = GameConstants::SpiritBeast::ParticleRadius;
        particle.color_rgba = PackRgba(255, 120, 170, 220);
        particle.velocity = {offset * 0.2f, -GameConstants::SpiritBeast::ParticleRiseSpeed - std::abs(offset)};
        particle.lifetime = GameConstants::SpiritBeast::ParticleLifetime;
        heart_particles.push_back(particle);
    }
}

void TrySpiritBeastWateringAid(SpiritBeast& spirit_beast,
                               bool& spirit_beast_watered_today,
                               std::vector<TeaPlot>& tea_plots,
                               CloudSeamanor::domain::StaminaSystem& stamina,
                               const std::function<void(TeaPlot&, bool)>& refresh_tea_plot_visual,
                               const std::function<void(const std::string&, float)>& push_hint,
                               const std::function<void(const std::string&)>& log_info) {
    if (spirit_beast.trait != "Watering Aid" || spirit_beast_watered_today) {
        return;
    }

    for (auto& plot : tea_plots) {
        if (!plot.seeded || plot.watered || plot.ready) {
            continue;
        }
        plot.watered = true;
        refresh_tea_plot_visual(plot, false);
        spirit_beast_watered_today = true;
        push_hint("Spirit beast used Watering Aid on a crop.", 3.0f);
        log_info("Spirit beast auto-watered a plot.");
        return;
    }

    if (stamina.Ratio() < GameConstants::SpiritBeast::StaminaRecoverRatioThreshold) {
        stamina.Recover(GameConstants::SpiritBeast::StaminaRecoverAmount);
        spirit_beast_watered_today = true;
        push_hint("Spirit beast shared calming energy. Stamina restored a little.", 3.0f);
        log_info("灵兽为玩家恢复了少量体力。");
    }
}

} // namespace CloudSeamanor::engine
