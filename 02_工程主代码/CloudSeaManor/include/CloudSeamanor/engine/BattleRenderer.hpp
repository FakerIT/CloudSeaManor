#pragma once

// ============================================================================
// 【BattleRenderer.hpp】战斗渲染器
// ============================================================================
// 负责战斗场景的视觉渲染：背景、灵体、灵兽伙伴、命中特效、大气粒子。
//
// 主要职责：
// - 渲染战斗背景（渐变、天气氛围、灵界装饰）
// - 渲染污染灵体（基于元素类型着色）
// - 渲染灵兽伙伴
// - 渲染命中特效（粒子爆炸）
// - 渲染大气粒子（悬浮灵气）
//
// 与其他系统的关系：
// - 依赖：BattleField（数据源）、ResourceManager（纹理资源）
// - 被依赖：GameRuntime（每帧调用渲染）
// ============================================================================

#include "CloudSeamanor/engine/BattleField.hpp"
#include "CloudSeamanor/engine/BattleEntities.hpp"
#include "CloudSeamanor/engine/rendering/ShapePool.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

namespace CloudSeamanor::engine {

// ============================================================================
// 【AmbientParticle】大气粒子
// ============================================================================
struct AmbientParticle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float radius = 3.0f;
    float alpha = 0.5f;
    float lifetime = 3.0f;
    float max_lifetime = 3.0f;
    std::uint8_t r = 200;
    std::uint8_t g = 200;
    std::uint8_t b = 255;
};

// ============================================================================
// 【AmbientParticleRenderer】大气粒子渲染数据
// ============================================================================
// 缓存粒子的渲染属性，避免每帧重新计算。
// ============================================================================
struct AmbientParticleRenderer {
    float base_radius;
    sf::Vector2f position;
    sf::Color color;
};

// ============================================================================
// 【BattleRenderer】战斗渲染器
// ============================================================================
class BattleRenderer {
public:
    // ========================================================================
    // 【初始化】
    // ========================================================================

    /**
     * @brief 初始化渲染器
     * @param window_width 窗口宽度
     * @param window_height 窗口高度
     * @param seed 随机种子（用于粒子系统）
     */
    void Initialize(unsigned int window_width, unsigned int window_height, unsigned int seed = 0);

    // ========================================================================
    // 【每帧更新】
    // ========================================================================

    /**
     * @brief 更新渲染状态
     * @param delta_seconds 帧间隔
     */
    void Update(float delta_seconds);

    // ========================================================================
    // 【渲染】
    // ========================================================================

    /**
     * @brief 渲染战斗场景到窗口
     * @param window 渲染目标窗口
     * @param field 战场数据引用
     */
    void Render(sf::RenderWindow& window, const BattleField& field);

    // ========================================================================
    // 【配置】
    // ========================================================================

    /** 设置天气状态（影响背景氛围） */
    void SetWeather(CloudSeamanor::domain::CloudState weather);

    /** 设置是否在灵界区域 */
    void SetInSpiritRealm(bool in_spirit_realm);

private:
    // ========================================================================
    // 背景渲染
    // ========================================================================
    void RenderBackground_(sf::RenderWindow& window);
    void RenderSpiritRealmDecor_(sf::RenderWindow& window);
    void RenderWeatherOverlay_(sf::RenderWindow& window);

    // ========================================================================
    // 灵体渲染
    // ========================================================================
    void RenderSpirits_(sf::RenderWindow& window, const BattleField& field);
    void RenderSpiritShape_(sf::RenderWindow& window, const PollutedSpirit& spirit);

    // ========================================================================
    // 灵兽伙伴渲染
    // ========================================================================
    void RenderPartners_(sf::RenderWindow& window, const BattleField& field);

    // ========================================================================
    // 特效渲染
    // ========================================================================
    void RenderHitEffects_(sf::RenderWindow& window, const BattleField& field);
    void RenderAmbientParticles_(sf::RenderWindow& window);

    // ========================================================================
    // 工具方法
    // ========================================================================
    [[nodiscard]] sf::Color ElementToColor_(ElementType element, float alpha = 255.0f) const;
    void SpawnAmbientParticles_(int count);

    // ========================================================================
    // 成员变量
    // ========================================================================
    unsigned int window_width_ = 1280;
    unsigned int window_height_ = 720;

    CloudSeamanor::domain::CloudState weather_ = CloudSeamanor::domain::CloudState::Clear;
    bool in_spirit_realm_ = true;

    // 大气粒子
    std::vector<AmbientParticle> ambient_particles_;
    std::mt19937 particle_random_;
    float particle_spawn_timer_ = 0.0f;
    static constexpr int kMaxAmbientParticles = 40;

    // 形状对象池（用于粒子渲染优化）
    rendering::CircleShapePool circle_pool_{64};

    // 灵体动画
    float spirit_pulse_timer_ = 0.0f;

    // 背景渐变顶点缓存
    std::vector<sf::Vertex> background_vertices_;
};

}  // namespace CloudSeamanor::engine
