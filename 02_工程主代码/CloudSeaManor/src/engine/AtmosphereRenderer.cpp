// ============================================================================
// 【AtmosphereRenderer.cpp】大气渲染器实现
// ============================================================================
// 渲染所有大气视觉层次：天空渐变、天气粒子、光晕叠加、调试面板。
//
// 性能设计：
// - 天空渐变：4 顶点 VertexArray，单次 DrawCall
// - 粒子系统：对象池，最多 200 粒子，单次 DrawCall（TriangleStrip）
// - 光晕叠加：全屏矩形 + 顶点颜色，单次 DrawCall
// - 总计最多 3 次 DrawCall
// ============================================================================

#include "CloudSeamanor/engine/rendering/AtmosphereRenderer.hpp"

#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace CloudSeamanor::engine {

namespace {

[[nodiscard]] float RandFloat(std::mt19937& rng, float min_val, float max_val) {
    std::uniform_real_distribution<float> dist(min_val, max_val);
    return dist(rng);
}

[[nodiscard]] int RandInt(std::mt19937& rng, int min_val, int max_val) {
    std::uniform_int_distribution<int> dist(min_val, max_val);
    return dist(rng);
}

[[nodiscard]] float Lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

}  // namespace

// ============================================================================
// 初始化
// ============================================================================

void AtmosphereRenderer::Initialize(
    sf::RenderWindow& window,
    domain::AtmosphereDomain& domain,
    const infrastructure::UiLayoutConfig& ui_layout
) {
    window_ref_ = window;
    domain_ref_ = domain;
    ui_layout_ref_ = ui_layout;

    sky_vertices_.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
    sky_vertices_.resize(4);

    const auto size = window.getSize();
    cached_window_width_ = size.x;
    cached_window_height_ = size.y;

    infrastructure::Logger::Info("AtmosphereRenderer 初始化完成。粒子池大小: " + std::to_string(kMaxParticles));
}

// ============================================================================
// 每帧更新
// ============================================================================

void AtmosphereRenderer::Update(float delta_seconds) {
    if (!domain_ref_.has_value() || !window_ref_.has_value()) {
        return;
    }
    auto& domain = domain_ref_->get();
    auto& window = window_ref_->get();

    const auto size = window.getSize();
    if (size.x != cached_window_width_ || size.y != cached_window_height_) {
        cached_window_width_ = size.x;
        cached_window_height_ = size.y;
        sky_geometry_dirty_ = true;
    }

    // 同步粒子参数
    if (const auto* profile = domain.GetActiveWeatherProfile()) {
        if (debug_weather_override_.empty()) {
            current_max_particles_ = static_cast<int>(
                profile->max_particles * domain.GetParticleDensity() * particle_density_multiplier_
            );
            current_spawn_rate_ = profile->particle_spawn_rate * particle_density_multiplier_;
        } else {
            current_max_particles_ = 0;
            current_spawn_rate_ = 0.0f;
        }
        current_speed_ = profile->particle_speed;
        current_direction_ = ParseDirection_(profile->particle_direction);
        color_start_r_ = profile->color_start_r;
        color_start_g_ = profile->color_start_g;
        color_start_b_ = profile->color_start_b;
        color_start_a_ = profile->color_start_a;
        color_end_r_ = profile->color_end_r;
        color_end_g_ = profile->color_end_g;
        color_end_b_ = profile->color_end_b;
        color_end_a_ = profile->color_end_a;
        size_min_ = profile->particle_size_min;
        size_max_ = profile->particle_size_max;
        lifetime_min_ = profile->particle_lifetime_min;
        lifetime_max_ = profile->particle_lifetime_max;
    }

    if (!particles_paused_) {
        UpdateParticleSpawn_(delta_seconds);
        UpdateParticleMove_(delta_seconds,
            static_cast<float>(cached_window_width_),
            static_cast<float>(cached_window_height_));
        UpdateParticleDespawn_();
    }

    sky_geometry_dirty_ = true;
}

// ============================================================================
// 天空渐变
// ============================================================================

void AtmosphereRenderer::DrawSkyGradient(sf::RenderWindow& window) {
    if (!domain_ref_.has_value()) {
        return;
    }
    const auto& domain = domain_ref_->get();

    const float w = static_cast<float>(cached_window_width_);
    const float h = static_cast<float>(cached_window_height_);

    if (sky_geometry_dirty_ || sky_vertices_[0].color.a == 0) {
        const sf::Color top_color(
            static_cast<std::uint8_t>(domain.SkyTopR()),
            static_cast<std::uint8_t>(domain.SkyTopG()),
            static_cast<std::uint8_t>(domain.SkyTopB()),
            static_cast<std::uint8_t>(domain.SkyTopA())
        );
        const sf::Color bottom_color(
            static_cast<std::uint8_t>(domain.SkyBottomR()),
            static_cast<std::uint8_t>(domain.SkyBottomG()),
            static_cast<std::uint8_t>(domain.SkyBottomB()),
            static_cast<std::uint8_t>(domain.SkyBottomA())
        );

        sky_vertices_[0] = sf::Vertex(sf::Vector2f(0.0f, 0.0f), top_color);
        sky_vertices_[1] = sf::Vertex(sf::Vector2f(w, 0.0f), top_color);
        sky_vertices_[2] = sf::Vertex(sf::Vector2f(0.0f, h), bottom_color);
        sky_vertices_[3] = sf::Vertex(sf::Vector2f(w, h), bottom_color);

        sky_geometry_dirty_ = false;
    }

    window.draw(sky_vertices_);
}

// ============================================================================
// 天气粒子
// ============================================================================

void AtmosphereRenderer::DrawWeatherParticles(sf::RenderWindow& window) {
    if (active_particle_count_ == 0) {
        return;
    }

    std::vector<sf::Vertex> vertices;
    vertices.reserve(active_particle_count_ * 6);

    for (const auto& p : particle_pool_) {
        if (!p.active) {
            continue;
        }

        const float half_size = p.size * 0.5f;
        const sf::Color c = p.color;

        vertices.push_back(sf::Vertex(sf::Vector2f(p.position.x - half_size, p.position.y - half_size), c));
        vertices.push_back(sf::Vertex(sf::Vector2f(p.position.x + half_size, p.position.y - half_size), c));
        vertices.push_back(sf::Vertex(sf::Vector2f(p.position.x - half_size, p.position.y + half_size), c));
        vertices.push_back(sf::Vertex(sf::Vector2f(p.position.x + half_size, p.position.y - half_size), c));
        vertices.push_back(sf::Vertex(sf::Vector2f(p.position.x + half_size, p.position.y + half_size), c));
        vertices.push_back(sf::Vertex(sf::Vector2f(p.position.x - half_size, p.position.y + half_size), c));
    }

    if (!vertices.empty()) {
        sf::VertexArray array(sf::PrimitiveType::Triangles);
        for (auto& v : vertices) {
            array.append(v);
        }
        window.draw(array);
    }
}

void AtmosphereRenderer::UpdateParticleSpawn_(float delta_seconds) {
    if (current_spawn_rate_ <= 0.0f || current_max_particles_ <= 0) {
        return;
    }

    particle_spawn_timer_ += delta_seconds;
    const float spawn_interval = 1.0f / current_spawn_rate_;
    const float window_w = static_cast<float>(cached_window_width_);
    const float window_h = static_cast<float>(cached_window_height_);

    while (particle_spawn_timer_ >= spawn_interval && active_particle_count_ < static_cast<std::size_t>(current_max_particles_)) {
        AtmosphereParticle* p = AcquireParticle_();
        if (p == nullptr) {
            break;
        }

        p->position = GetSpawnPosition_(current_direction_, window_w, window_h);
        p->velocity = GetMoveDirection_(current_direction_, p->position.x, p->position.y)
                    * current_speed_;
        p->size = RandFloat(rng_, size_min_, size_max_);
        p->lifetime = RandFloat(rng_, lifetime_min_, lifetime_max_);
        p->age = 0.0f;

        sf::Color start_col(
            static_cast<std::uint8_t>(color_start_r_),
            static_cast<std::uint8_t>(color_start_g_),
            static_cast<std::uint8_t>(color_start_b_),
            static_cast<std::uint8_t>(color_start_a_)
        );
        p->color = start_col;

        particle_spawn_timer_ -= spawn_interval;
        ++active_particle_count_;
    }

    if (particle_spawn_timer_ >= spawn_interval) {
        particle_spawn_timer_ = 0.0f;
    }
}

void AtmosphereRenderer::UpdateParticleMove_(float delta_seconds, float window_w, float window_h) {
    for (auto& p : particle_pool_) {
        if (!p.active) {
            continue;
        }

        p.position += p.velocity * delta_seconds;
        p.age += delta_seconds;

        const float ratio = std::clamp(p.age / p.lifetime, 0.0f, 1.0f);
        p.color = InterpolateParticleColor_(ratio, domain::WeatherProfile{});
        p.color.a = static_cast<std::uint8_t>(
            static_cast<float>(p.color.a) * (1.0f - ratio)
        );

        if (p.position.x < -50.0f || p.position.x > window_w + 50.0f
            || p.position.y < -50.0f || p.position.y > window_h + 50.0f) {
            p.active = false;
        }
    }
}

void AtmosphereRenderer::UpdateParticleDespawn_() {
    std::size_t removed = 0;
    for (auto& p : particle_pool_) {
        if (p.active && p.age >= p.lifetime) {
            p.active = false;
            ++removed;
        }
    }
    active_particle_count_ -= removed;
}

AtmosphereParticle* AtmosphereRenderer::AcquireParticle_() {
    for (auto& p : particle_pool_) {
        if (!p.active) {
            p.active = true;
            return &p;
        }
    }
    return nullptr;
}

sf::Color AtmosphereRenderer::InterpolateParticleColor_(float age_ratio, const domain::WeatherProfile&) const {
    const std::uint8_t r = static_cast<std::uint8_t>(
        Lerp(static_cast<float>(color_start_r_), static_cast<float>(color_end_r_), age_ratio)
    );
    const std::uint8_t g = static_cast<std::uint8_t>(
        Lerp(static_cast<float>(color_start_g_), static_cast<float>(color_end_g_), age_ratio)
    );
    const std::uint8_t b = static_cast<std::uint8_t>(
        Lerp(static_cast<float>(color_start_b_), static_cast<float>(color_end_b_), age_ratio)
    );
    return sf::Color(r, g, b, 255);
}

ParticleDirection AtmosphereRenderer::ParseDirection_(const std::string& dir) {
    if (dir == "left") return ParticleDirection::Left;
    if (dir == "radial") return ParticleDirection::Radial;
    if (dir == "down") return ParticleDirection::Down;
    return ParticleDirection::Right;
}

sf::Vector2f AtmosphereRenderer::GetSpawnPosition_(ParticleDirection dir, float window_w, float window_h) {
    switch (dir) {
    case ParticleDirection::Left:
        return sf::Vector2f(window_w + 10.0f, RandFloat(rng_, 0.0f, window_h));
    case ParticleDirection::Right:
        return sf::Vector2f(-10.0f, RandFloat(rng_, 0.0f, window_h));
    case ParticleDirection::Down:
        return sf::Vector2f(RandFloat(rng_, 0.0f, window_w), -10.0f);
    case ParticleDirection::Radial: {
        const float angle = RandFloat(rng_, 0.0f, 6.28318530718f);
        const float dist = std::sqrt(window_w * window_w + window_h * window_h) * 0.6f;
        return sf::Vector2f(
            window_w * 0.5f + std::cos(angle) * dist,
            window_h * 0.5f + std::sin(angle) * dist
        );
    }
    }
    return sf::Vector2f(-10.0f, RandFloat(rng_, 0.0f, window_h));
}

sf::Vector2f AtmosphereRenderer::GetMoveDirection_(ParticleDirection dir, float x, float y) {
    switch (dir) {
    case ParticleDirection::Left:
        return sf::Vector2f(-1.0f, RandFloat(rng_, -0.2f, 0.2f));
    case ParticleDirection::Right:
        return sf::Vector2f(1.0f, RandFloat(rng_, -0.2f, 0.2f));
    case ParticleDirection::Down:
        return sf::Vector2f(RandFloat(rng_, -0.3f, 0.3f), 1.0f);
    case ParticleDirection::Radial: {
        const float cx = static_cast<float>(cached_window_width_) * 0.5f;
        const float cy = static_cast<float>(cached_window_height_) * 0.5f;
        const float dx = cx - x;
        const float dy = cy - y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) {
            return sf::Vector2f(0.0f, -1.0f);
        }
        return sf::Vector2f(dx / len, dy / len);
    }
    }
    return sf::Vector2f(1.0f, 0.0f);
}

// ============================================================================
// 光晕叠加
// ============================================================================

void AtmosphereRenderer::DrawAuraOverlay(sf::RenderWindow& window) {
    if (!domain_ref_.has_value() || !ui_layout_ref_.has_value()) {
        return;
    }
    const auto& domain = domain_ref_->get();
    const auto& ui_layout = ui_layout_ref_->get();

    const auto& cloud = domain.GetCurrentCloudState();
    const std::string key = [&] {
        switch (cloud) {
        case domain::CloudState::Clear:      return "cloud_clear";
        case domain::CloudState::Mist:       return "cloud_mist";
        case domain::CloudState::DenseCloud: return "cloud_dense";
        case domain::CloudState::Tide:       return "cloud_tide";
        }
        return "cloud_clear";
    }();

    const auto aura_config = ui_layout.GetCloudColor(key);
    const sf::Color aura_base = adapter::PackedRgbaToColor(aura_config.aura);

    const float intensity = domain.GetAtmosphereIntensity();
    const std::uint8_t alpha = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(aura_base.a * intensity), 0, 255)
    );
    const sf::Color overlay_color(aura_base.r, aura_base.g, aura_base.b, alpha);

    if (overlay_color.a < 5) {
        return;
    }

    sf::VertexArray overlay(sf::PrimitiveType::TriangleStrip, 4);
    const float w = static_cast<float>(cached_window_width_);
    const float h = static_cast<float>(cached_window_height_);

    overlay[0] = sf::Vertex(sf::Vector2f(0.0f, 0.0f), overlay_color);
    overlay[1] = sf::Vertex(sf::Vector2f(w, 0.0f), overlay_color);
    overlay[2] = sf::Vertex(sf::Vector2f(0.0f, h), overlay_color);
    overlay[3] = sf::Vertex(sf::Vector2f(w, h), overlay_color);

    window.draw(overlay);
}

// ============================================================================
// 调试面板
// ============================================================================

void AtmosphereRenderer::DrawDebugPanel(sf::RenderWindow& window) {
    if (!show_debug_panel_ || !domain_ref_.has_value() || debug_font_ == nullptr) {
        return;
    }
    const auto& domain = domain_ref_->get();

    const float panel_w = 320.0f;
    const float panel_h = 240.0f;
    const float panel_x = static_cast<float>(cached_window_width_) - panel_w - 16.0f;
    const float panel_y = 16.0f;

    sf::RectangleShape panel;
    panel.setPosition({panel_x, panel_y});
    panel.setSize({panel_w, panel_h});
    panel.setFillColor(sf::Color(10, 10, 30, 200));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(sf::Color(100, 150, 255, 200));
    window.draw(panel);

    std::ostringstream info;
    info << "=== Atmosphere Debug ===\n";
    info << "Cloud: ";
    switch (domain.GetCurrentCloudState()) {
    case domain::CloudState::Clear:      info << "Clear\n";      break;
    case domain::CloudState::Mist:       info << "Mist\n";       break;
    case domain::CloudState::DenseCloud: info << "DenseCloud\n"; break;
    case domain::CloudState::Tide:      info << "Tide\n";      break;
    }
    info << "Phase: ";
    switch (domain.GetCurrentDayPhase()) {
    case domain::DayPhase::Morning:    info << "Morning\n";    break;
    case domain::DayPhase::Afternoon: info << "Afternoon\n"; break;
    case domain::DayPhase::Evening:   info << "Evening\n";   break;
    case domain::DayPhase::Night:     info << "Night\n";     break;
    }
    info << "Atmosphere: " << domain.GetAtmosphereIntensity() << "\n";
    info << "Fog Density: " << domain.GetFogDensity() << "\n";
    info << "Particles: " << active_particle_count_ << "/" << current_max_particles_ << "\n";
    info << "Transition: " << (domain.IsTransitioning() ? "YES" : "no");
    if (domain.IsTransitioning()) {
        info << " (" << (domain.GetTransitionProgress() * 100.0f) << "%)\n";
    } else {
        info << "\n";
    }
    info << "SkyTop: #" << std::hex
         << domain.SkyTopR() << domain.SkyTopG() << domain.SkyTopB()
         << std::dec << "\n";
    info << "SkyBottom: #" << std::hex
         << domain.SkyBottomR() << domain.SkyBottomG() << domain.SkyBottomB()
         << std::dec << "\n";
    info << "[1]Clear [2]Mist [3]Dense [4]Tide\n";
    info << "[T] Transition [R] Reset [P] Pause\n";
    info << "[+/-] Density: " << particle_density_multiplier_ << "x";

    sf::Text debug_text(*debug_font_);
    debug_text.setString(sf::String::fromUtf8(info.str().begin(), info.str().end()));
    debug_text.setCharacterSize(12);
    debug_text.setFillColor(sf::Color(180, 220, 255));
    debug_text.setPosition({panel_x + 10.0f, panel_y + 10.0f});

    window.draw(debug_text);
}

void AtmosphereRenderer::SetDebugWeatherOverride(const std::string& type) {
    debug_weather_override_ = type;
}

// ============================================================================
// 批量绘制
// ============================================================================

void AtmosphereRenderer::DrawAll(sf::RenderWindow& window) {
    DrawSkyGradient(window);
    DrawWeatherParticles(window);
    DrawAuraOverlay(window);
    DrawDebugPanel(window);
}

}  // namespace CloudSeamanor::engine
