#include "CloudSeamanor/engine/rendering/WorldRenderer.hpp"

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/domain/CloudGuardianContract.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"
#include "CloudSeamanor/Profiling.hpp"
#include "CloudSeamanor/domain/CropData.hpp"

#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

WorldRenderer::WorldRenderer() = default;

bool WorldRenderer::LoadFestivalDecorations(const std::string& config_path) {
    return festival_decorations_.LoadFromFile(config_path);
}

namespace {
float StageWaveOffset_(const int stage, const float phase_seed) {
    if (stage < 3) {
        return 0.0f;
    }
    const float amplitude = (stage >= 4) ? 2.0f : 1.0f;
    return std::sin(phase_seed) * amplitude;
}

// ============================================================================
// 【StageCropColor_】作物颜色（5阶段+品质）
// ============================================================================
sf::Color StageCropColor_(const int stage, const CloudSeamanor::domain::CropQuality quality) {
    // 基础颜色：5个生长阶段
    switch (stage) {
    case 0: return sf::Color(126, 90, 56, 240);   // 种子：棕色
    case 1: return sf::Color(158, 216, 132, 245); // 幼苗：浅绿
    case 2: return sf::Color(82, 174, 92, 250);   // 成长期：深绿
    case 3: {
        // 3阶段根据品质微调
        switch (quality) {
        case CloudSeamanor::domain::CropQuality::Fine:   return sf::Color(90, 190, 100, 255);
        case CloudSeamanor::domain::CropQuality::Rare:   return sf::Color(100, 200, 130, 255);
        case CloudSeamanor::domain::CropQuality::Spirit: return sf::Color(140, 220, 160, 255);
        case CloudSeamanor::domain::CropQuality::Holy:   return sf::Color(180, 240, 200, 255);
        default: return sf::Color(106, 198, 124, 255);
        }
    }
    default: return sf::Color(106, 198, 124, 255);
    }
}

// ============================================================================
// 【StageCropSize_】作物尺寸（5阶段递增）
// ============================================================================
sf::Vector2f StageCropSize_(const int stage, const CloudSeamanor::domain::CropQuality quality) {
    // 基础尺寸（宽, 高）
    switch (stage) {
    case 0: return {4.0f, 4.0f};    // 种子
    case 1: return {8.0f, 8.0f};    // 幼苗
    case 2: return {12.0f, 12.0f};  // 成长期
    case 3: return {14.0f, 14.0f};  // 即将成熟
    default: {
        // 成熟阶段：根据品质增加尺寸
        float size_mult = 1.0f;
        switch (quality) {
        case CloudSeamanor::domain::CropQuality::Fine:   size_mult = 1.1f; break;
        case CloudSeamanor::domain::CropQuality::Rare:   size_mult = 1.2f; break;
        case CloudSeamanor::domain::CropQuality::Spirit: size_mult = 1.3f; break;
        case CloudSeamanor::domain::CropQuality::Holy:   size_mult = 1.5f; break;
        default: break;
        }
        return {16.0f * size_mult, 16.0f * size_mult};
    }
    }
}

// ============================================================================
// 【QualityGlowColor_】品质光晕颜色
// ============================================================================
sf::Color QualityGlowColor_(const CloudSeamanor::domain::CropQuality quality) {
    switch (quality) {
    case CloudSeamanor::domain::CropQuality::Fine:   return sf::Color(100, 200, 100, 180);  // 浅绿
    case CloudSeamanor::domain::CropQuality::Rare:   return sf::Color(150, 150, 255, 200);  // 淡紫
    case CloudSeamanor::domain::CropQuality::Spirit: return sf::Color(200, 150, 255, 220);  // 粉紫
    case CloudSeamanor::domain::CropQuality::Holy:   return sf::Color(255, 220, 100, 240);  // 金色
    default: return sf::Color(255, 215, 0, 180);  // 普通金色边框
    }
}

// ============================================================================
// 【PlotStatusTint_】地块状态色调
// ============================================================================
sf::Color PlotStatusTint_(const TeaPlot& plot) {
    // 病虫害：褐色
    if (plot.disease || plot.pest) {
        return sf::Color(139, 90, 43, 160);
    }
    // 缺水：偏黄棕色
    if (plot.seeded && !plot.watered && !plot.sprinkler_installed) {
        return sf::Color(180, 140, 80, 80);
    }
    // 施肥：淡金色
    if (plot.fertilized) {
        return sf::Color(255, 220, 150, 60);
    }
    return sf::Color(0, 0, 0, 0);
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
    RenderPlacedObjects_(window, world_state);
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

    // E16 节日装饰：使用数据驱动的 FestivalDecorationSystem
    const std::string& festival_id = ws.GetActiveFestivalId();
    if (!festival_id.empty() && festival_decorations_.HasFestival(festival_id)) {
        auto decorations = festival_decorations_.GetDecorations(festival_id, ws.GetSessionTime());
        for (auto& decor : decorations) {
            window.draw(*decor);
        }
    }

    const int gday = ws.GetClock().Day();
    const auto& fr = ws.GetFestivalRuntime();
    if (fr.flower_bloom_visual_day == gday) {
        sf::RectangleShape tint({1280.0f, 720.0f});
        tint.setFillColor(sf::Color(255, 190, 210, 28));
        window.draw(tint);
    }
    if (fr.winter_solstice_polar_anchor_day == gday) {
        sf::RectangleShape polar({1280.0f, 720.0f});
        polar.setFillColor(sf::Color(18, 22, 40, 85));
        window.draw(polar);
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
    const float pulse = ws.GetSessionTime();
    for (const auto& i : ws.GetInteractables()) {
        window.draw(i.Shape());

        // 战斗区域视觉指示器：在灵界中，战斗锚点显示为脉冲紫色边框
        if (ws.GetInSpiritRealm()
            && (i.Label() == "Spirit Beast Zone" || i.Label() == "Spirit Beast")) {
            const float glow_alpha = static_cast<std::uint8_t>(
                120.0f + 80.0f * std::sin(pulse * 2.5f));
            sf::RectangleShape glow = i.Shape();
            glow.setOutlineColor(sf::Color(180, 80, 220, glow_alpha));
            glow.setOutlineThickness(3.0f);
            glow.setFillColor(sf::Color(0, 0, 0, 0));
            window.draw(glow);
        }
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
    const auto& plots = ws.GetTeaPlots();
    const auto& shapes = ws.GetSceneVisuals().TeaPlotShapes();
    const std::size_t count = std::min(plots.size(), shapes.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto& plot = plots[i];
        sf::RectangleShape soil = shapes[i];
        
        // 地块基础颜色：浇水 vs 未浇水
        if (plot.watered) {
            soil.setFillColor(sf::Color(114, 142, 176, 220));  // 蓝色调
        } else {
            soil.setFillColor(sf::Color(176, 142, 98, 220));   // 土黄色调
        }
        soil.setOutlineThickness(1.5f);
        soil.setOutlineColor(sf::Color(88, 60, 38, 180));
        
        // 状态色调叠加（病虫害/缺水/施肥）
        const sf::Color status_tint = PlotStatusTint_(plot);
        if (status_tint.a > 0) {
            soil.setFillColor(sf::Color(
                static_cast<std::uint8_t>((soil.getFillColor().r + status_tint.r) / 2),
                static_cast<std::uint8_t>((soil.getFillColor().g + status_tint.g) / 2),
                static_cast<std::uint8_t>((soil.getFillColor().b + status_tint.b) / 2),
                soil.getFillColor().a
            ));
        }
        
        window.draw(soil);
        const sf::Vector2f size = soil.getSize();
        const sf::Vector2f base = soil.getPosition();
        if (ws.GetPurifyReturnDays() > 0) {
            const float pulse01 = 0.5f + 0.5f * std::sin(pulse * 3.1f + base.x * 0.02f + base.y * 0.03f);
            const std::uint8_t alpha = static_cast<std::uint8_t>(40.0f + pulse01 * 90.0f);
            sf::CircleShape spirit_dot(3.0f);
            spirit_dot.setFillColor(sf::Color(155, 235, 255, alpha));
            spirit_dot.setPosition({base.x + size.x * 0.15f, base.y + size.y * 0.15f});
            window.draw(spirit_dot);
            spirit_dot.setPosition({base.x + size.x * 0.72f, base.y + size.y * 0.28f});
            window.draw(spirit_dot);
        }

        if (!plot.seeded) {
            continue;
        }

        const float wave = StageWaveOffset_(plot.stage, pulse + base.x * 0.03f + base.y * 0.02f);
        
        // 获取作物尺寸
        const sf::Vector2f crop_size = StageCropSize_(plot.stage, plot.quality);
        
        // ===== 阶段0：种子 =====
        if (plot.stage <= 0) {
            sf::RectangleShape crop;
            crop.setSize(crop_size);
            crop.setFillColor(StageCropColor_(0, plot.quality));
            crop.setPosition({base.x + size.x * 0.5f - crop_size.x * 0.5f, base.y + size.y * 0.5f - crop_size.y * 0.5f});
            window.draw(crop);
            continue;
        }

        // ===== 阶段1：幼苗 =====
        if (plot.stage == 1) {
            sf::RectangleShape crop;
            crop.setSize(crop_size);
            crop.setFillColor(StageCropColor_(1, plot.quality));
            crop.setPosition({base.x + size.x * 0.5f - crop_size.x * 0.5f, base.y + size.y * 0.5f - crop_size.y * 0.5f});
            window.draw(crop);
            continue;
        }

        // ===== 阶段2：成长期 =====
        if (plot.stage == 2) {
            sf::RectangleShape crop;
            crop.setSize(crop_size);
            crop.setFillColor(StageCropColor_(2, plot.quality));
            crop.setPosition({base.x + size.x * 0.5f - crop_size.x * 0.5f, base.y + size.y * 0.5f - crop_size.y * 0.5f});
            window.draw(crop);
            continue;
        }

        // ===== 阶段3：即将成熟 + 阶段4：成熟 =====
        sf::RectangleShape crop;
        crop.setSize(crop_size);
        crop.setFillColor(StageCropColor_(3, plot.quality));
        crop.setPosition({
            base.x + size.x * 0.5f - crop_size.x * 0.5f + wave,
            base.y + size.y * 0.5f - crop_size.y * 0.7f
        });
        window.draw(crop);

        // ===== 成熟阶段：收获光效 + 品质光晕 =====
        if (plot.ready || plot.stage >= 4) {
            // 品质光晕（inner glow）
            if (plot.quality >= CloudSeamanor::domain::CropQuality::Fine) {
                const sf::Color glow = QualityGlowColor_(plot.quality);
                sf::CircleShape quality_glow(crop_size.x * 0.7f);
                quality_glow.setFillColor(sf::Color(glow.r, glow.g, glow.b, glow.a / 3));
                quality_glow.setPosition({
                    crop.getPosition().x + crop_size.x * 0.15f,
                    crop.getPosition().y + crop_size.y * 0.15f
                });
                window.draw(quality_glow);
            }
            
            // 金色边框（收获提示）
            const float border_size = crop_size.x + 4.0f;
            sf::RectangleShape harvest_glow({border_size, border_size});
            harvest_glow.setPosition({
                base.x + size.x * 0.5f - border_size * 0.5f + wave,
                base.y + size.y * 0.5f - crop_size.y * 0.7f - 2.0f
            });
            harvest_glow.setFillColor(sf::Color(0, 0, 0, 0));
            harvest_glow.setOutlineThickness(2.0f);
            harvest_glow.setOutlineColor(QualityGlowColor_(plot.quality));
            window.draw(harvest_glow);

            // 收获粒子动画
            const float particle_offset = std::sin(pulse * 3.0f + base.x) * 4.0f;
            sf::CircleShape particle(2.5f);
            particle.setFillColor(sf::Color(255, 236, 128, 220));
            particle.setPosition({
                base.x + size.x * 0.5f + crop_size.x * 0.3f + particle_offset,
                base.y + size.y * 0.5f - crop_size.y * 0.9f + std::sin(pulse * 2.5f + base.y) * 3.0f
            });
            window.draw(particle);
        } else if (plot.sprinkler_installed) {
            // 洒水器地块：蓝色边框
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
    const auto& npcs = ws.GetNpcs();
    const auto& shapes = ws.GetSceneVisuals().NpcShapes();
    const std::size_t count = std::min(npcs.size(), shapes.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (!npcs[i].visible) {
            continue;
        }
        window.draw(shapes[i]);
    }
}

// ============================================================================
// 【WorldRenderer::RenderSpiritBeast_】渲染灵兽
// ============================================================================
void WorldRenderer::RenderSpiritBeast_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    window.draw(ws.GetSceneVisuals().SpiritBeastShape());
}

// ============================================================================
// 【WorldRenderer::RenderParticles_】渲染粒子效果
// ============================================================================
void WorldRenderer::RenderParticles_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    for (const auto& h : ws.GetHeartParticles()) {
        sf::CircleShape particle(h.radius, CloudSeamanor::GameConstants::SpiritBeast::ParticlePointCount);
        particle.setFillColor(CloudSeamanor::adapter::PackedRgbaToColor(h.color_rgba));
        particle.setPosition(CloudSeamanor::adapter::ToSf(h.position));
        window.draw(particle);
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

void WorldRenderer::RenderPlacedObjects_(
    sf::RenderWindow& window,
    const GameWorldState& ws
) {
    const sf::Vector2f origin{880.0f, 96.0f};
    for (const auto& obj : ws.GetPlacedObjects()) {
        if (obj.room != "tea_room") {
            continue;
        }
        sf::RectangleShape block({18.0f, 18.0f});
        block.setPosition(origin + sf::Vector2f{obj.tile_x * 22.0f, obj.tile_y * 22.0f});
        block.setOutlineThickness(2.0f);
        block.setRotation(sf::degrees(static_cast<float>(obj.rotation)));
        if (obj.custom_data == "preview") {
            block.setFillColor(sf::Color(160, 210, 255, 140));
        } else if (obj.custom_data == "ready") {
            block.setFillColor(sf::Color(220, 200, 120, 180));
        } else {
            block.setFillColor(sf::Color(140, 120, 90, 220));
        }
        block.setOutlineColor(sf::Color(72, 48, 24, 220));
        window.draw(block);
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
