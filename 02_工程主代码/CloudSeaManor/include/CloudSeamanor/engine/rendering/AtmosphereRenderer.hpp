// ============================================================================
// 【AtmosphereRenderer.hpp】大气渲染器
// ============================================================================
// 将 AtmosphereDomain 的状态渲染到 SFML 窗口中。
//
// 主要职责：
// - 渲染天空渐变层（全屏双三角渐变）
// - 管理天气粒子系统（对象池，最高效）
// - 渲染大气光晕叠加层（复用 ui_layout.json 的 aura 颜色）
// - 管理昼夜天空过渡动画的 GPU 端颜色插值
// - 提供调试面板（显示当前天空配置和过渡进度）
//
// 与其他系统的关系：
// - 依赖：AtmosphereDomain（数据源）、sf::RenderWindow（渲染目标）
// - 依赖：UiLayoutConfig（光晕颜色）、CloudSystem（状态）
// - 被依赖：GameApp::Render（主循环调用）
//
// 渲染层次（从底到顶）：
//   L0: 窗口背景清屏（由 GameApp::Render 负责，不在本类中）
//   L1: 天空渐变层（DrawSkyGradient）
//   L2: 远景雾效层（DrawDistanceFog，可用 shader）
//   L3: 天气粒子层（DrawWeatherParticles，对象池）
//   L4: 场景内容（由 WorldRenderer 渲染，不在本类中）
//   L5: 大气光晕叠加（DrawAuraOverlay）
//   L6: 调试面板（DrawDebugPanel）
//
// 性能目标：
// - 天空渐变：单次 DrawCall（4 顶点 VertexArray）
// - 粒子系统：对象池，≤ 200 粒子，最大 1 次 DrawCall
// - 光晕叠加：单次 DrawCall（全屏矩形 + 顶点颜色）
// - 总体：≤ 3 次 DrawCall，稳定 60 FPS
// ============================================================================

#pragma once

#include "CloudSeamanor/domain/AtmosphereState.hpp"
#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/infrastructure/UiLayoutConfig.hpp"

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <random>
#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【ParticleDirection】粒子移动方向
// ============================================================================
enum class ParticleDirection {
    Left,
    Right,
    Radial,
    Down,
};

// ============================================================================
// 【AtmosphereParticle】大气粒子
// ============================================================================
// 天气粒子系统中的单个粒子。
struct AtmosphereParticle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float size = 4.0f;
    float lifetime = 3.0f;
    float age = 0.0f;
    sf::Color color = sf::Color::White;
    bool active = false;
};

// ============================================================================
// 【AtmosphereRenderer】大气渲染器
// ============================================================================
// 负责所有大气视觉效果的实际渲染。
class AtmosphereRenderer {
public:
    // ========================================================================
    // 【Initialize】初始化渲染器
    // ========================================================================
    // @param window 渲染窗口引用
    // @param domain 大气领域对象引用
    // @param ui_layout UI 布局配置引用
    void Initialize(
        sf::RenderWindow& window,
        domain::AtmosphereDomain& domain,
        const infrastructure::UiLayoutConfig& ui_layout
    );

    // ========================================================================
    // 【Update】每帧更新
    // ========================================================================
    // @param delta_seconds 帧间隔秒数
    // @note 更新粒子生成、移动、消亡
    void Update(float delta_seconds);

    // ========================================================================
    // 【DrawSkyGradient】渲染天空渐变层
    // ========================================================================
    // @param window 渲染窗口
    // @note 全屏渐变，从天空顶部色到天空底部色
    void DrawSkyGradient(sf::RenderWindow& window);

    // ========================================================================
    // 【DrawWeatherParticles】渲染天气粒子层
    // ========================================================================
    // @param window 渲染窗口
    // @note 使用 VertexArray + 对象池绘制所有活跃粒子
    void DrawWeatherParticles(sf::RenderWindow& window);

    // ========================================================================
    // 【DrawAuraOverlay】渲染大气光晕叠加层
    // ========================================================================
    // @param window 渲染窗口
    // @note 全屏半透明叠加，颜色取自 ui_layout.json 的 aura 字段
    void DrawAuraOverlay(sf::RenderWindow& window);

    // ========================================================================
    // 【DrawDebugPanel】渲染调试面板
    // ========================================================================
    // @param window 渲染窗口
    // @note 仅在调试模式（show_debug_panel_ == true）时渲染
    void DrawDebugPanel(sf::RenderWindow& window);

    // ========================================================================
    // 【DrawAll】渲染所有大气层
    // ========================================================================
    // @param window 渲染窗口
    // @note 调用 DrawSkyGradient → DrawWeatherParticles → DrawAuraOverlay
    void DrawAll(sf::RenderWindow& window);

    // ========================================================================
    // 【调试控制】
    // ========================================================================

    void ToggleDebugPanel() { show_debug_panel_ = !show_debug_panel_; }
    [[nodiscard]] bool IsDebugPanelVisible() const { return show_debug_panel_; }

    // 粒子密度倍率（0.0 ~ 2.0，默认 1.0）
    void SetParticleDensityMultiplier(float m) { particle_density_multiplier_ = std::clamp(m, 0.0f, 2.0f); }
    [[nodiscard]] float GetParticleDensityMultiplier() const { return particle_density_multiplier_; }

    // 强制设置天气类型（调试用）
    void SetDebugWeatherOverride(const std::string& type);
    void ClearDebugWeatherOverride() { debug_weather_override_.clear(); }

    // 暂停/恢复粒子更新
    void SetPaused(bool paused) { particles_paused_ = paused; }
    [[nodiscard]] bool IsPaused() const { return particles_paused_; }

private:
    // ========================================================================
    // 【内部方法】
    // ========================================================================

    void UpdateParticleSpawn_(float delta_seconds);
    void UpdateParticleMove_(float delta_seconds, float window_w, float window_h);
    void UpdateParticleDespawn_();

    [[nodiscard]] AtmosphereParticle* AcquireParticle_();
    void ReleaseParticle_(AtmosphereParticle* p);

    [[nodiscard]] sf::Color InterpolateParticleColor_(float age_ratio, const domain::WeatherProfile& profile) const;
    [[nodiscard]] ParticleDirection ParseDirection_(const std::string& dir);
    [[nodiscard]] sf::Vector2f GetSpawnPosition_(ParticleDirection dir, float window_w, float window_h);
    [[nodiscard]] sf::Vector2f GetMoveDirection_(ParticleDirection dir, float x, float y);

    // ========================================================================
    // 成员变量
    // ========================================================================

    // 依赖引用
    std::optional<std::reference_wrapper<sf::RenderWindow>> window_ref_;
    std::optional<std::reference_wrapper<domain::AtmosphereDomain>> domain_ref_;
    std::optional<std::reference_wrapper<const infrastructure::UiLayoutConfig>> ui_layout_ref_;

    // 天空渐变顶点（4 顶点：左上、右上、右下、左下）
    mutable sf::VertexArray sky_vertices_;
    mutable bool sky_geometry_dirty_ = true;

    // 粒子系统
    static constexpr std::size_t kMaxParticles = 200;
    std::array<AtmosphereParticle, kMaxParticles> particle_pool_;
    std::size_t active_particle_count_ = 0;

    float particle_spawn_timer_ = 0.0f;
    std::mt19937 rng_{std::random_device{}()};

    // 当前粒子参数（从 active_weather_profile 同步）
    int current_max_particles_ = 0;
    float current_spawn_rate_ = 0.0f;
    float current_speed_ = 30.0f;
    ParticleDirection current_direction_ = ParticleDirection::Right;
    int color_start_r_ = 255, color_start_g_ = 255, color_start_b_ = 255, color_start_a_ = 200;
    int color_end_r_ = 255, color_end_g_ = 255, color_end_b_ = 255, color_end_a_ = 50;
    float size_min_ = 2.0f, size_max_ = 8.0f;
    float lifetime_min_ = 2.0f, lifetime_max_ = 5.0f;

    // 调试
    bool show_debug_panel_ = false;
    bool particles_paused_ = false;
    float particle_density_multiplier_ = 1.0f;
    std::string debug_weather_override_;

    // 窗口尺寸缓存
    unsigned int cached_window_width_ = 0;
    unsigned int cached_window_height_ = 0;

    // 字体（调试面板）
    const sf::Font* debug_font_ = nullptr;
};

}  // namespace CloudSeamanor::engine
