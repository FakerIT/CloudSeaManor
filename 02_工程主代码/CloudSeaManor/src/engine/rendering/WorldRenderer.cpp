#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/engine/rendering/WorldRenderer.hpp"

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/CloudGuardianContract.hpp"
#include "CloudSeamanor/Profiling.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

WorldRenderer::WorldRenderer() = default;

namespace {
float StageWaveOffset_(const int stage, const float phase_seed) {
    if (stage < 3) {
        return 0.0f;
    }
    const float amplitude = (stage >= 4) ? 2.0f : 1.0f;
    return std::sin(phase_seed) * amplitude;
}

sf::Color StageCropColor_(const int stage) {
    switch (stage) {
    case 0: return sf::Color(126, 90, 56, 240);   // 种子：棕色
    case 1: return sf::Color(158, 216, 132, 245); // 幼苗：浅绿
    case 2: return sf::Color(82, 174, 92, 250);   // 成长期：深绿
    default: return sf::Color(106, 198, 124, 255);
    }
}
}  // namespace

// ============================================================================
// 【WorldRenderer::Render】渲染世界场景
// ============================================================================
void WorldRenderer::Render(
    sf::RenderWindow& window,
    const GameWorldState& world_state
) {
    CSM_ZONE_SCOPED;
    RenderGroundTiles_(window, world_state);
    RenderObstacles_(window, world_state);
    RenderInteractables_(window, world_state);
    RenderTeaPlots_(window, world_state);
    RenderNpcs_(window, world_state);
    RenderSpiritBeast_(window, world_state);
    RenderParticles_(window, world_state);
    RenderPickups_(window, world_state);
    RenderPlayer_(window, world_state);
}

// ============================================================================
// 【WorldRenderer::RenderGroundTiles_】渲染地面瓦片
// ============================================================================
void WorldRenderer::RenderGroundTiles_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    if (ws.GetInSpiritRealm()) {
        sf::RectangleShape aura({1280.0f, 720.0f});
        const int completed = CloudSeamanor::domain::GetGlobalContract().CompletedPactCount();
        sf::Color c(90, 85, 110, 70); // 荒废
        if (completed >= 1) c = sf::Color(120, 95, 145, 75);  // 苏醒
        if (completed >= 2) c = sf::Color(170, 170, 210, 85); // 兴盛
        if (completed >= 3) c = sf::Color(210, 180, 255, 95); // 繁华
        if (completed >= 4) c = sf::Color(235, 210, 120, 100); // 太初
        aura.setFillColor(c);
        window.draw(aura);
    }
    for (const auto& t : ws.GetGroundTiles()) {
        window.draw(t);
    }
}

// ============================================================================
// 【WorldRenderer::RenderObstacles_】渲染障碍物
// ============================================================================
void WorldRenderer::RenderObstacles_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    for (const auto& o : ws.GetObstacleShapes()) {
        window.draw(o);
    }
}

// ============================================================================
// 【WorldRenderer::RenderInteractables_】渲染可交互对象
// ============================================================================
void WorldRenderer::RenderInteractables_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    for (const auto& i : ws.GetInteractables()) {
        window.draw(i.Shape());
    }
}

// ============================================================================
// 【WorldRenderer::RenderTeaPlots_】渲染茶田地块
// ============================================================================
void WorldRenderer::RenderTeaPlots_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    const float pulse = ws.GetSessionTime();
    for (const auto& plot : ws.GetTeaPlots()) {
        sf::RectangleShape soil = plot.shape;
        if (plot.watered) {
            soil.setFillColor(sf::Color(114, 142, 176, 220));  // 蓝色调
        } else {
            soil.setFillColor(sf::Color(176, 142, 98, 220));   // 土黄色调
        }
        soil.setOutlineThickness(1.5f);
        soil.setOutlineColor(sf::Color(88, 60, 38, 180));
        window.draw(soil);

        if (!plot.seeded) {
            continue;
        }

        const sf::Vector2f size = plot.shape.getSize();
        const sf::Vector2f base = plot.shape.getPosition();
        const float wave = StageWaveOffset_(plot.stage, pulse + base.x * 0.03f + base.y * 0.02f);
        sf::RectangleShape crop;

        if (plot.stage <= 0) {
            crop.setSize({4.0f, 4.0f});
            crop.setFillColor(StageCropColor_(0));
            crop.setPosition({base.x + size.x * 0.5f - 2.0f, base.y + size.y * 0.5f - 2.0f});
            window.draw(crop);
            continue;
        }

        if (plot.stage == 1) {
            crop.setSize({8.0f, 8.0f});
            crop.setFillColor(StageCropColor_(1));
            crop.setPosition({base.x + size.x * 0.5f - 4.0f, base.y + size.y * 0.5f - 6.0f});
            window.draw(crop);
            continue;
        }

        if (plot.stage == 2) {
            crop.setSize({12.0f, 12.0f});
            crop.setFillColor(StageCropColor_(2));
            crop.setPosition({base.x + size.x * 0.5f - 6.0f, base.y + size.y * 0.5f - 10.0f});
            window.draw(crop);
            continue;
        }

        // stage 3/4：成熟视觉 + 轻微摇摆
        crop.setSize({16.0f, 16.0f});
        crop.setFillColor(plot.shape.getFillColor());
        crop.setPosition({base.x + size.x * 0.5f - 8.0f + wave, base.y + size.y * 0.5f - 14.0f});
        window.draw(crop);

        if (plot.ready || plot.stage >= 4) {
            sf::RectangleShape harvest_glow({20.0f, 20.0f});
            harvest_glow.setPosition({base.x + size.x * 0.5f - 10.0f + wave, base.y + size.y * 0.5f - 16.0f});
            harvest_glow.setFillColor(sf::Color(0, 0, 0, 0));
            harvest_glow.setOutlineThickness(2.0f);
            harvest_glow.setOutlineColor(sf::Color(255, 215, 0, 225));  // 金色边框
            window.draw(harvest_glow);

            sf::CircleShape particle(2.5f);
            particle.setFillColor(sf::Color(255, 236, 128, 220));
            particle.setPosition({base.x + size.x * 0.5f + 9.0f + std::cos(pulse * 3.0f) * 4.0f,
                                  base.y + size.y * 0.5f - 18.0f + std::sin(pulse * 2.5f) * 3.0f});
            window.draw(particle);
        } else if (plot.sprinkler_installed) {
            crop.setOutlineThickness(1.0f);
            crop.setOutlineColor(sf::Color(80, 160, 220, 220));
        }
    }
}

// ============================================================================
// 【WorldRenderer::RenderNpcs_】渲染 NPC
// ============================================================================
void WorldRenderer::RenderNpcs_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    for (const auto& n : ws.GetNpcs()) {
        if (!n.visible) {
            continue;
        }
        window.draw(n.shape);
    }
}

// ============================================================================
// 【WorldRenderer::RenderSpiritBeast_】渲染灵兽
// ============================================================================
void WorldRenderer::RenderSpiritBeast_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    window.draw(ws.GetSpiritBeast().shape);
}

// ============================================================================
// 【WorldRenderer::RenderParticles_】渲染粒子效果
// ============================================================================
void WorldRenderer::RenderParticles_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    for (const auto& h : ws.GetHeartParticles()) {
        window.draw(h.shape);
    }
}

// ============================================================================
// 【WorldRenderer::RenderPickups_】渲染可拾取物
// ============================================================================
void WorldRenderer::RenderPickups_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    for (const auto& pk : ws.GetPickups()) {
        window.draw(pk.Shape());
    }
}

// ============================================================================
// 【WorldRenderer::RenderPlayer_】渲染玩家
// ============================================================================
void WorldRenderer::RenderPlayer_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    player_visual_.SyncFromDomainPlayer(ws.GetPlayer());
    window.draw(player_visual_.Shape());
}

}  // namespace CloudSeamanor::engine
