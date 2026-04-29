#include "CloudSeamanor/engine/BattleRenderer.hpp"

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include <algorithm>
#include <cmath>

namespace CloudSeamanor::engine {

// ============================================================================
// 【Initialize】初始化
// ============================================================================
void BattleRenderer::Initialize(unsigned int window_width, unsigned int window_height, unsigned int seed) {
    window_width_ = window_width;
    window_height_ = window_height;
    particle_random_.seed(seed == 0 ? std::random_device{}() : seed);

    // 预分配粒子数组
    ambient_particles_.reserve(kMaxAmbientParticles);

    // 初始化背景渐变顶点（四边形：两个三角形）
    background_vertices_.resize(6);
}

// ============================================================================
// 【Update】每帧更新
// ============================================================================
void BattleRenderer::Update(float delta_seconds) {
    // 更新大气粒子
    particle_spawn_timer_ += delta_seconds;
    if (particle_spawn_timer_ >= 0.15f && ambient_particles_.size() < static_cast<std::size_t>(kMaxAmbientParticles)) {
        SpawnAmbientParticles_(2);
        particle_spawn_timer_ = 0.0f;
    }

    // 更新现有粒子
    for (std::size_t i = 0; i < ambient_particles_.size();) {
        auto& p = ambient_particles_[i];
        p.x += p.vx * delta_seconds;
        p.y += p.vy * delta_seconds;
        p.lifetime -= delta_seconds;

        // 渐隐
        if (p.max_lifetime > 0.0f) {
            p.alpha = static_cast<float>(p.lifetime / p.max_lifetime) * 0.6f;
        }

        if (p.lifetime <= 0.0f) {
            ambient_particles_[i] = ambient_particles_.back();
            ambient_particles_.pop_back();
        } else {
            ++i;
        }
    }

    // 更新灵体动画时间
    spirit_pulse_timer_ += delta_seconds;
}

// ============================================================================
// 【Render】渲染
// ============================================================================
void BattleRenderer::Render(sf::RenderWindow& window, const BattleField& field) {
    RenderBackground_(window);
    RenderAmbientParticles_(window);
    RenderSpirits_(window, field);
    RenderPartners_(window, field);
    RenderHitEffects_(window, field);
    RenderWeatherOverlay_(window);
}

// ============================================================================
// 【SetWeather】设置天气
// ============================================================================
void BattleRenderer::SetWeather(CloudSeamanor::domain::CloudState weather) {
    weather_ = weather;
}

// ============================================================================
// 【SetInSpiritRealm】设置是否在灵界
// ============================================================================
void BattleRenderer::SetInSpiritRealm(bool in_spirit_realm) {
    in_spirit_realm_ = in_spirit_realm;
}

// ============================================================================
// 【RenderBackground_】渲染背景
// ============================================================================
void BattleRenderer::RenderBackground_(sf::RenderWindow& window) {
    // 使用渐变色矩形模拟灵界氛围背景
    sf::RectangleShape bg;
    bg.setPosition({0.0f, 0.0f});
    bg.setSize({static_cast<float>(window_width_), static_cast<float>(window_height_)});

    // 根据天气设置背景色
    switch (weather_) {
        case CloudSeamanor::domain::CloudState::Clear:
            bg.setFillColor(sf::Color(18, 20, 45));  // 深蓝紫
            break;
        case CloudSeamanor::domain::CloudState::Mist:
            bg.setFillColor(sf::Color(25, 30, 60));  // 浅雾色
            break;
        case CloudSeamanor::domain::CloudState::DenseCloud:
            bg.setFillColor(sf::Color(12, 15, 35));  // 深浓云
            break;
        case CloudSeamanor::domain::CloudState::Tide:
            bg.setFillColor(sf::Color(15, 25, 50));  // 潮汐色
            break;
    }
    window.draw(bg);

    // 渲染灵界装饰
    if (in_spirit_realm_) {
        RenderSpiritRealmDecor_(window);
    }
}

// ============================================================================
// 【RenderSpiritRealmDecor_】渲染灵界装饰
// ============================================================================
void BattleRenderer::RenderSpiritRealmDecor_(sf::RenderWindow& window) {
    // 底部灵气池效果
    sf::RectangleShape pool;
    pool.setPosition({0.0f, static_cast<float>(window_height_) - 80.0f});
    pool.setSize({static_cast<float>(window_width_), 80.0f});

    // 根据天气调整池子颜色
    sf::Color pool_color;
    switch (weather_) {
        case CloudSeamanor::domain::CloudState::Tide:
            pool_color = sf::Color(30, 80, 120, 180);  // 潮汐蓝
            break;
        case CloudSeamanor::domain::CloudState::DenseCloud:
            pool_color = sf::Color(50, 40, 80, 150);   // 浓云紫
            break;
        default:
            pool_color = sf::Color(40, 60, 100, 120);   // 灵界蓝
            break;
    }
    pool.setFillColor(pool_color);
    window.draw(pool);

    // 灵气池波纹动画
    const float wave_y = static_cast<float>(window_height_) - 75.0f + std::sin(spirit_pulse_timer_ * 2.0f) * 3.0f;
    sf::RectangleShape wave;
    wave.setPosition({0.0f, wave_y});
    wave.setSize({static_cast<float>(window_width_), 3.0f});
    wave.setFillColor(sf::Color(80, 120, 180, 100));
    window.draw(wave);

    // 顶部云海效果
    sf::RectangleShape top_haze;
    top_haze.setPosition({0.0f, 0.0f});
    top_haze.setSize({static_cast<float>(window_width_), 60.0f});

    sf::Color haze_color;
    switch (weather_) {
        case CloudSeamanor::domain::CloudState::DenseCloud:
            haze_color = sf::Color(60, 50, 90, 200);  // 浓云
            break;
        case CloudSeamanor::domain::CloudState::Mist:
            haze_color = sf::Color(80, 85, 120, 120);  // 薄雾
            break;
        default:
            haze_color = sf::Color(30, 40, 80, 100);  // 普通灵界
            break;
    }
    top_haze.setFillColor(haze_color);
    window.draw(top_haze);

    // 角落装饰光晕（左上角）
    sf::CircleShape glow1(150.0f);
    glow1.setPosition({-80.0f, -80.0f});
    glow1.setFillColor(sf::Color(60, 40, 100, 40));
    window.draw(glow1);

    // 角落装饰光晕（右下角）
    sf::CircleShape glow2(120.0f);
    glow2.setPosition({static_cast<float>(window_width_) - 60.0f, static_cast<float>(window_height_) - 60.0f});
    glow2.setFillColor(sf::Color(40, 80, 140, 35));
    window.draw(glow2);
}

// ============================================================================
// 【RenderWeatherOverlay_】渲染天气叠加层
// ============================================================================
void BattleRenderer::RenderWeatherOverlay_(sf::RenderWindow& window) {
    // 大潮时添加特殊光效
    if (weather_ == CloudSeamanor::domain::CloudState::Tide) {
        // 潮汐波纹效果
        sf::RectangleShape tideOverlay;
        tideOverlay.setPosition({0.0f, 0.0f});
        tideOverlay.setSize({static_cast<float>(window_width_), static_cast<float>(window_height_)});

        const float pulse = (std::sin(spirit_pulse_timer_ * 1.5f) + 1.0f) * 0.5f;
        const std::uint8_t alpha = static_cast<std::uint8_t>(15.0f + pulse * 10.0f);
        tideOverlay.setFillColor(sf::Color(20, 60, 100, alpha));
        window.draw(tideOverlay);
    }

    // 浓云海时添加暗角效果
    if (weather_ == CloudSeamanor::domain::CloudState::DenseCloud) {
        // 简单的顶部渐变遮罩
        sf::RectangleShape darkHaze;
        darkHaze.setPosition({0.0f, 0.0f});
        darkHaze.setSize({static_cast<float>(window_width_), 150.0f});
        darkHaze.setFillColor(sf::Color(10, 8, 25, 80));
        window.draw(darkHaze);
    }
}

// ============================================================================
// 【RenderSpirits_】渲染所有灵体
// ============================================================================
void BattleRenderer::RenderSpirits_(sf::RenderWindow& window, const BattleField& field) {
    const auto& spirits = field.GetSpirits();
    for (const auto& spirit : spirits) {
        if (spirit.is_defeated || spirit.is_escaped) {
            continue;
        }
        RenderSpiritShape_(window, spirit);
    }
}

// ============================================================================
// 【RenderSpiritShape_】渲染单个灵体
// ============================================================================
void BattleRenderer::RenderSpiritShape_(sf::RenderWindow& window, const PollutedSpirit& spirit) {
    // 基础形状大小
    float base_radius = spirit.size_radius;

    // BOSS 稍微大一些
    if (spirit.type == SpiritType::Boss) {
        base_radius *= 1.5f;
    }

    // 脉冲动画
    const float pulse = 1.0f + std::sin(spirit_pulse_timer_ * 3.0f) * 0.08f;
    float radius = base_radius * pulse;

    // 获取元素颜色
    const sf::Color base_color = ElementToColor_(spirit.element, 255.0f);
    const sf::Color glow_color = ElementToColor_(spirit.element, 80.0f);

    // 外发光效果（多个叠加圆）
    for (int i = 3; i >= 1; --i) {
        sf::CircleShape glow(static_cast<float>(i) * 6.0f + radius);
        glow.setPosition({spirit.pos_x - radius - static_cast<float>(i) * 6.0f,
                         spirit.pos_y - radius - static_cast<float>(i) * 6.0f});
        glow.setFillColor(sf::Color(glow_color.r, glow_color.g, glow_color.b,
                                     static_cast<std::uint8_t>(40 / i)));
        window.draw(glow);
    }

    // 污染程度指示（根据污染值调整颜色明度）
    const float pollution_ratio = spirit.current_pollution / spirit.max_pollution;
    sf::Color spirit_color = base_color;
    if (pollution_ratio > 0.7f) {
        // 高污染：偏暗红
        spirit_color = sf::Color(
            static_cast<std::uint8_t>(std::min(255.0f, base_color.r * 0.6f + 80.0f)),
            static_cast<std::uint8_t>(std::min(255.0f, base_color.g * 0.4f)),
            static_cast<std::uint8_t>(std::min(255.0f, base_color.b * 0.4f))
        );
    }

    // 主体圆形
    sf::CircleShape body(radius);
    body.setPosition({spirit.pos_x - radius, spirit.pos_y - radius});
    body.setFillColor(spirit_color);
    window.draw(body);

    // 内核高光
    sf::CircleShape core(radius * 0.5f);
    core.setPosition({spirit.pos_x - radius * 0.5f, spirit.pos_y - radius * 0.5f});
    core.setFillColor(sf::Color(255, 255, 255, 60));
    window.draw(core);

    // BOSS 特殊标记
    if (spirit.type == SpiritType::Boss) {
        // BOSS 光环
        sf::CircleShape boss_ring(radius + 15.0f);
        boss_ring.setPosition({spirit.pos_x - radius - 15.0f, spirit.pos_y - radius - 15.0f});
        boss_ring.setOutlineThickness(3.0f);
        boss_ring.setOutlineColor(sf::Color(255, 200, 50, 150));
        boss_ring.setFillColor(sf::Color(0, 0, 0, 0));
        window.draw(boss_ring);

        // 旋转的小三角装饰
        const float angle = spirit_pulse_timer_ * 2.0f;
        for (int i = 0; i < 3; ++i) {
            const float a = angle + i * 2.094f;  // 120度
            const float orbit_radius = radius + 25.0f;
            const float dot_x = spirit.pos_x + std::cos(a) * orbit_radius;
            const float dot_y = spirit.pos_y + std::sin(a) * orbit_radius;

            sf::CircleShape dot(4.0f);
            dot.setPosition({dot_x - 4.0f, dot_y - 4.0f});
            dot.setFillColor(sf::Color(255, 220, 80, 200));
            window.draw(dot);
        }
    }

    // 精英标记
    if (spirit.type == SpiritType::Elite) {
        sf::CircleShape elite_ring(radius + 10.0f);
        elite_ring.setPosition({spirit.pos_x - radius - 10.0f, spirit.pos_y - radius - 10.0f});
        elite_ring.setOutlineThickness(2.0f);
        elite_ring.setOutlineColor(sf::Color(180, 140, 255, 180));
        elite_ring.setFillColor(sf::Color(0, 0, 0, 0));
        window.draw(elite_ring);
    }
}

// ============================================================================
// 【RenderPartners_】渲染灵兽伙伴
// ============================================================================
void BattleRenderer::RenderPartners_(sf::RenderWindow& window, const BattleField& field) {
    const auto& partners = field.GetPartners();

    for (std::size_t i = 0; i < partners.size(); ++i) {
        const auto& partner = partners[i];

        // 伙伴位置在左下角区域
        const float base_x = 80.0f + static_cast<float>(i) * 100.0f;
        const float base_y = static_cast<float>(window_height_) - 180.0f;

        // 跟随动画
        const float bob = std::sin(spirit_pulse_timer_ * 2.5f + static_cast<float>(i) * 1.5f) * 4.0f;
        const float x = base_x;
        const float y = base_y + bob;

        // 伙伴光环
        sf::CircleShape partner_glow(35.0f);
        partner_glow.setPosition({x - 35.0f, y - 35.0f});
        partner_glow.setFillColor(sf::Color(100, 200, 255, 40));
        window.draw(partner_glow);

        // 伙伴主体（爱心形状指示羁绊等级）
        sf::CircleShape partner_body(20.0f);
        partner_body.setPosition({x - 20.0f, y - 20.0f});

        // 根据羁绊等级调整颜色
        sf::Color partner_color;
        switch (partner.heart_level) {
            case 1: partner_color = sf::Color(180, 180, 220); break;
            case 2: partner_color = sf::Color(160, 200, 255); break;
            case 3: partner_color = sf::Color(140, 220, 200); break;
            case 4: partner_color = sf::Color(200, 180, 255); break;
            default: partner_color = sf::Color(255, 200, 220); break;
        }
        partner_body.setFillColor(partner_color);
        window.draw(partner_body);

        // 羁绊等级指示
        sf::CircleShape heart_indicator(6.0f);
        heart_indicator.setPosition({x + 15.0f, y - 25.0f});
        heart_indicator.setFillColor(sf::Color(255, 100, 150, 220));
        window.draw(heart_indicator);
    }
}

// ============================================================================
// 【RenderHitEffects_】渲染命中特效
// ============================================================================
void BattleRenderer::RenderHitEffects_(sf::RenderWindow& window, const BattleField& field) {
    const auto& effects = field.GetHitEffects();

    for (const auto& effect : effects) {
        const float progress = 1.0f - (effect.remaining / effect.total);

        // 扩散的圆环
        const float expanded_radius = effect.radius * (1.0f + progress * 2.0f);
        sf::CircleShape ring(expanded_radius);
        ring.setPosition({effect.x - expanded_radius, effect.y - expanded_radius});

        const std::uint8_t alpha = static_cast<std::uint8_t>((1.0f - progress) * 255.0f);
        sf::Color ring_color(
            (effect.color_rgba >> 24) & 0xFF,
            (effect.color_rgba >> 16) & 0xFF,
            (effect.color_rgba >> 8) & 0xFF,
            alpha
        );
        ring.setOutlineThickness(3.0f * (1.0f - progress * 0.5f));
        ring.setOutlineColor(ring_color);
        ring.setFillColor(sf::Color(0, 0, 0, 0));
        window.draw(ring);

        // 中心闪光
        if (progress < 0.3f) {
            const float core_alpha = (1.0f - progress / 0.3f) * 200.0f;
            sf::CircleShape core(effect.radius * 0.6f * (1.0f - progress));
            core.setPosition({effect.x - effect.radius * 0.6f * (1.0f - progress),
                             effect.y - effect.radius * 0.6f * (1.0f - progress)});
            core.setFillColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(core_alpha)));
            window.draw(core);
        }

        // 粒子爆发效果
        const int particle_count = 8;
        const float particle_dist = expanded_radius * 0.8f * progress;
        for (int i = 0; i < particle_count; ++i) {
            const float angle = static_cast<float>(i) / static_cast<float>(particle_count) * 6.28318f;  // 2*PI
            const float px = effect.x + std::cos(angle) * particle_dist;
            const float py = effect.y + std::sin(angle) * particle_dist;

            sf::CircleShape particle(3.0f * (1.0f - progress));
            particle.setPosition({px - 3.0f * (1.0f - progress), py - 3.0f * (1.0f - progress)});
            particle.setFillColor(sf::Color(ring_color.r, ring_color.g, ring_color.b,
                                           static_cast<std::uint8_t>(alpha * 0.7f)));
            window.draw(particle);
        }
    }
}

// ============================================================================
// 【RenderAmbientParticles_】渲染大气粒子
// ============================================================================
void BattleRenderer::RenderAmbientParticles_(sf::RenderWindow& window) {
    for (const auto& p : ambient_particles_) {
        sf::CircleShape particle(p.radius);
        particle.setPosition({p.x - p.radius, p.y - p.radius});
        particle.setFillColor(sf::Color(p.r, p.g, p.b, static_cast<std::uint8_t>(p.alpha * 255.0f)));
        window.draw(particle);
    }
}

// ============================================================================
// 【SpawnAmbientParticles_】生成大气粒子
// ============================================================================
void BattleRenderer::SpawnAmbientParticles_(int count) {
    std::uniform_real_distribution<float> x_dist(0.0f, static_cast<float>(window_width_));
    std::uniform_real_distribution<float> y_dist(50.0f, static_cast<float>(window_height_) - 100.0f);
    std::uniform_real_distribution<float> speed_dist(-15.0f, 15.0f);
    std::uniform_real_distribution<float> size_dist(1.5f, 4.0f);
    std::uniform_real_distribution<float> lifetime_dist(2.0f, 5.0f);

    for (int i = 0; i < count; ++i) {
        AmbientParticle p;
        p.x = x_dist(particle_random_);
        p.y = y_dist(particle_random_);
        p.vx = speed_dist(particle_random_);
        p.vy = speed_dist(particle_random_) * 0.5f - 5.0f;  // 稍微向上飘
        p.radius = size_dist(particle_random_);
        p.lifetime = lifetime_dist(particle_random_);
        p.max_lifetime = p.lifetime;
        p.alpha = 0.3f + static_cast<float>(particle_random_()) * 0.3f;

        // 根据天气设置粒子颜色
        switch (weather_) {
            case CloudSeamanor::domain::CloudState::Tide:
                p.r = 60; p.g = 140; p.b = 220;
                break;
            case CloudSeamanor::domain::CloudState::DenseCloud:
                p.r = 120; p.g = 100; p.b = 160;
                break;
            case CloudSeamanor::domain::CloudState::Mist:
                p.r = 160; p.g = 170; p.b = 200;
                break;
            default:
                p.r = 140; p.g = 160; p.b = 220;
                break;
        }

        ambient_particles_.push_back(p);
    }
}

// ============================================================================
// 【ElementToColor_】元素类型转颜色
// ============================================================================
sf::Color BattleRenderer::ElementToColor_(ElementType element, float alpha) const {
    switch (element) {
        case ElementType::Water:
            return sf::Color(80, 160, 255, static_cast<std::uint8_t>(alpha));  // 潮
        case ElementType::Fire:
            return sf::Color(255, 120, 80, static_cast<std::uint8_t>(alpha));   // 霞
        case ElementType::Wood:
            return sf::Color(100, 220, 160, static_cast<std::uint8_t>(alpha));  // 露
        case ElementType::Metal:
            return sf::Color(200, 220, 255, static_cast<std::uint8_t>(alpha)); // 云
        case ElementType::Earth:
            return sf::Color(180, 200, 160, static_cast<std::uint8_t>(alpha)); // 风
        case ElementType::Light:
            return sf::Color(255, 240, 160, static_cast<std::uint8_t>(alpha));  // 光
        case ElementType::Dark:
            return sf::Color(160, 120, 220, static_cast<std::uint8_t>(alpha)); // 暗
        default:
            return sf::Color(180, 180, 200, static_cast<std::uint8_t>(alpha)); // 中性
    }
}

}  // namespace CloudSeamanor::engine
