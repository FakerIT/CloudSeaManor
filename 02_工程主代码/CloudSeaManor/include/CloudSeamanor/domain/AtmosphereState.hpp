// ============================================================================
// 【AtmosphereState.hpp】大气状态领域对象
// ============================================================================
// 统一管理游戏的大气视觉状态：天空颜色、大气强度、昼夜转换、雾效密度。
//
// 主要职责：
// - 持有当前天空轮廓配置和天气粒子配置引用
// - 管理云海状态 → 天空配置的映射
// - 管理昼夜时段 → 天空配置的映射
// - 计算天空颜色插值和过渡动画
// - 提供查询接口供 AtmosphereRenderer 使用
//
// 与其他系统的关系：
// - 依赖：CloudSystem（云海状态）、GameClock（昼夜时段）
// - 被依赖：AtmosphereRenderer（读取状态渲染）、SaveLoad（存档）
//
// 设计决策：
// - 纯领域层，无 SFML 依赖
// - 状态由外部系统（CloudSystem / GameClock）驱动
// - 天空配置由数据表驱动（atmosphere_profiles.csv）
// - 过渡动画时间在领域层计算，插值在引擎层执行
// ============================================================================

#pragma once

#include "CloudSeamanor/domain/CloudSystem.hpp"
#include "CloudSeamanor/domain/GameClock.hpp"

#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <string>
#include <vector>

namespace CloudSeamanor::domain {

// ============================================================================
// 【SkyProfile】天空轮廓配置
// ============================================================================
// 从 atmosphere_profiles.csv 加载的一行数据。
// 描述某个云海状态 + 时段组合下的理想天空外观。
struct SkyProfile {
    std::string id;                 // 唯一标识
    std::string cloud_state;        // 关联云海状态: "clear|mist|dense|tide"
    std::string day_phase;          // 关联时段: "morning|afternoon|evening|night"
    int sky_top_r = 135;            // 天空顶部颜色 R
    int sky_top_g = 206;            // 天空顶部颜色 G
    int sky_top_b = 235;            // 天空顶部颜色 B
    int sky_top_a = 255;            // 天空顶部颜色 A
    int sky_bottom_r = 240;         // 天空底部颜色 R
    int sky_bottom_g = 248;         // 天空底部颜色 G
    int sky_bottom_b = 255;         // 天空底部颜色 B
    int sky_bottom_a = 255;         // 天空底部颜色 A
    int horizon_r = 255;            // 地平线颜色 R
    int horizon_g = 255;            // 地平线颜色 G
    int horizon_b = 255;            // 地平线颜色 B
    int horizon_a = 200;            // 地平线颜色 A
    int sun_r = 255;                // 太阳光晕颜色 R
    int sun_g = 255;               // 太阳光晕颜色 G
    int sun_b = 200;               // 太阳光晕颜色 B
    int sun_a = 100;               // 太阳光晕颜色 A
    int ambient_r = 240;            // 环境光颜色 R
    int ambient_g = 248;           // 环境光颜色 G
    int ambient_b = 255;           // 环境光颜色 B
    int ambient_a = 200;           // 环境光颜色 A
    float fog_density = 0.0f;      // 基础雾密度 0.0~1.0
    float atmosphere_intensity = 1.0f;  // 大气强度
    float particle_density = 0.0f;  // 粒子密度倍率
    int sort_order = 0;            // 排序优先级
};

// ============================================================================
// 【WeatherProfile】天气粒子效果配置
// ============================================================================
// 从 weather_effects.json 加载的配置。
// 描述某种云海状态下的粒子效果参数。
struct WeatherProfile {
    std::string id;                 // 唯一标识
    std::string cloud_state;        // 关联云海状态
    int max_particles = 0;          // 最大粒子数量
    float particle_speed = 30.0f;   // 粒子移动速度（像素/秒）
    std::string particle_direction = "right";  // 移动方向: "left|right|radial|down"
    float particle_size_min = 2.0f; // 粒子最小尺寸
    float particle_size_max = 8.0f; // 粒子最大尺寸
    int color_start_r = 255;        // 粒子起始颜色 R
    int color_start_g = 255;       // 粒子起始颜色 G
    int color_start_b = 255;       // 粒子起始颜色 B
    int color_start_a = 200;       // 粒子起始颜色 A
    int color_end_r = 255;          // 粒子结束颜色 R
    int color_end_g = 255;         // 粒子结束颜色 G
    int color_end_b = 255;         // 粒子结束颜色 B
    int color_end_a = 50;          // 粒子结束颜色 A
    float particle_lifetime_min = 2.0f;  // 粒子最小存活时间
    float particle_lifetime_max = 5.0f;  // 粒子最大存活时间
    float particle_spawn_rate = 30.0f;  // 每秒生成粒子数
    bool follows_camera = true;     // 是否跟随相机
};

// ============================================================================
// 【AtmosphereDomain】大气领域对象
// ============================================================================
// 游戏大气视觉状态的领域逻辑核心。
//
// 管理职责：
// - 根据 CloudSystem 和 GameClock 计算当前天空配置
// - 管理云海状态切换时的天空过渡动画
// - 管理昼夜时段切换时的天空过渡动画
// - 计算大气强度和雾效密度
// - 提供插值颜色供渲染器使用
//
// 使用示例：
// @code
// AtmosphereDomain atmosphere;
// atmosphere.Initialize(cloud_system, game_clock);
//
// // 每帧
// atmosphere.SyncFromCloudAndClock(cloud_system, game_clock);
// atmosphere.Update(delta_seconds);
//
// // 渲染前查询
// auto colors = atmosphere.GetCurrentSkyColors();
// float fog_density = atmosphere.GetFogDensity();
// @endcode
class AtmosphereDomain {
public:
    // ========================================================================
    // 【Initialize】初始化大气系统
    // ========================================================================
    // @param sky_profiles 天空轮廓配置列表（由数据加载器传入）
    // @param weather_profiles 天气粒子配置列表（由数据加载器传入）
    void Initialize(
        const std::vector<SkyProfile>& sky_profiles,
        const std::vector<WeatherProfile>& weather_profiles
    );

    // ========================================================================
    // 【SyncFromCloudAndClock】从云海系统和时钟同步状态
    // ========================================================================
    // @param cloud 云海系统引用
    // @param clock 游戏时钟引用
    // @note 每帧调用，根据当前云海状态和时段查找匹配的 sky_profile
    // @note 如果 sky_profile 发生变化，自动触发过渡动画
    void SyncFromCloudAndClock(
        const CloudSystem& cloud,
        const GameClock& clock
    );

    // ========================================================================
    // 【Update】更新过渡动画
    // ========================================================================
    // @param delta_seconds 帧间隔秒数
    // @note 更新天空过渡进度、昼夜插值等
    void Update(float delta_seconds);

    // ========================================================================
    // 【ForceState】强制设置大气状态（用于调试或剧情）
    // ========================================================================
    // @param cloud_state 目标云海状态
    // @param phase 目标时段
    // @param immediate 是否立即切换（true=无过渡动画）
    void ForceState(
        CloudState cloud_state,
        DayPhase phase,
        bool immediate = false
    );

    // ========================================================================
    // 【TransitionTo】触发动画过渡
    // ========================================================================
    // @param target_sky 目标天空配置
    // @param duration_seconds 过渡持续秒数
    void TransitionTo(const SkyProfile& target_sky, float duration_seconds = 3.0f);

    // ========================================================================
    // 【SetTransitionDuration】设置过渡动画持续时间
    // ========================================================================
    void SetTransitionDuration(float seconds) { transition_duration_ = seconds; }

    // ========================================================================
    // 【天空颜色查询】
    // ========================================================================

    // 天空顶部颜色（当前插值结果）
    [[nodiscard]] int SkyTopR() const noexcept { return sky_top_r_; }
    [[nodiscard]] int SkyTopG() const noexcept { return sky_top_g_; }
    [[nodiscard]] int SkyTopB() const noexcept { return sky_top_b_; }
    [[nodiscard]] int SkyTopA() const noexcept { return sky_top_a_; }

    // 天空底部颜色（当前插值结果）
    [[nodiscard]] int SkyBottomR() const noexcept { return sky_bottom_r_; }
    [[nodiscard]] int SkyBottomG() const noexcept { return sky_bottom_g_; }
    [[nodiscard]] int SkyBottomB() const noexcept { return sky_bottom_b_; }
    [[nodiscard]] int SkyBottomA() const noexcept { return sky_bottom_a_; }

    // 地平线颜色
    [[nodiscard]] int HorizonR() const noexcept { return horizon_r_; }
    [[nodiscard]] int HorizonG() const noexcept { return horizon_g_; }
    [[nodiscard]] int HorizonB() const noexcept { return horizon_b_; }
    [[nodiscard]] int HorizonA() const noexcept { return horizon_a_; }

    // 太阳光晕颜色
    [[nodiscard]] int SunR() const noexcept { return sun_r_; }
    [[nodiscard]] int SunG() const noexcept { return sun_g_; }
    [[nodiscard]] int SunB() const noexcept { return sun_b_; }
    [[nodiscard]] int SunA() const noexcept { return sun_a_; }

    // ========================================================================
    // 【大气参数查询】
    // ========================================================================

    [[nodiscard]] float GetFogDensity() const noexcept { return fog_density_; }
    [[nodiscard]] float GetAtmosphereIntensity() const noexcept { return atmosphere_intensity_; }
    [[nodiscard]] float GetParticleDensity() const noexcept { return particle_density_; }
    [[nodiscard]] bool IsTransitioning() const noexcept { return is_transitioning_; }
    [[nodiscard]] float GetTransitionProgress() const noexcept {
        return transition_duration_ > 0.0f
            ? std::clamp(transition_elapsed_ / transition_duration_, 0.0f, 1.0f)
            : 1.0f;
    }

    // ========================================================================
    // 【当前活跃配置查询】
    // ========================================================================

    [[nodiscard]] CloudState GetCurrentCloudState() const noexcept { return current_cloud_state_; }
    [[nodiscard]] DayPhase GetCurrentDayPhase() const noexcept { return current_day_phase_; }
    [[nodiscard]] const SkyProfile* GetActiveSkyProfile() const { return active_sky_profile_; }
    [[nodiscard]] const WeatherProfile* GetActiveWeatherProfile() const { return active_weather_profile_; }

    // ========================================================================
    // 【存档接口】
    // ========================================================================

    // 保存当前大气状态
    [[nodiscard]] std::string SaveState() const;

    // 恢复大气状态
    void LoadState(const std::string& state);

private:
    // ========================================================================
    // 【FindSkyProfile】查找匹配的天空配置
    // ========================================================================
    [[nodiscard]] const SkyProfile* FindSkyProfile(
        CloudState cloud, DayPhase phase
    ) const;

    // ========================================================================
    // 【FindWeatherProfile】查找匹配的天气配置
    // ========================================================================
    [[nodiscard]] const WeatherProfile* FindWeatherProfile(
        CloudState cloud
    ) const;

    // ========================================================================
    // 【ApplySkyProfile】应用天空配置到当前状态
    // ========================================================================
    void ApplySkyProfile(const SkyProfile& profile);

    // ========================================================================
    // 【InterpolateColors】插值天空颜色
    // ========================================================================
    void InterpolateColors_(float t);

    // ========================================================================
    // 成员变量
    // ========================================================================

    // 数据引用（由 Initialize 传入，不持有所有权）
    const std::vector<SkyProfile>* sky_profiles_ = nullptr;
    const std::vector<WeatherProfile>* weather_profiles_ = nullptr;

    // 当前活跃配置
    const SkyProfile* active_sky_profile_ = nullptr;
    const WeatherProfile* active_weather_profile_ = nullptr;

    // 当前插值状态
    CloudState current_cloud_state_ = CloudState::Clear;
    DayPhase current_day_phase_ = DayPhase::Morning;

    // 当前天空颜色（引擎直接读取这些值）
    int sky_top_r_ = 135, sky_top_g_ = 206, sky_top_b_ = 235, sky_top_a_ = 255;
    int sky_bottom_r_ = 240, sky_bottom_g_ = 248, sky_bottom_b_ = 255, sky_bottom_a_ = 255;
    int horizon_r_ = 255, horizon_g_ = 255, horizon_b_ = 255, horizon_a_ = 200;
    int sun_r_ = 255, sun_g_ = 255, sun_b_ = 200, sun_a_ = 100;

    // 大气参数
    float fog_density_ = 0.0f;
    float atmosphere_intensity_ = 1.0f;
    float particle_density_ = 0.0f;

    // 过渡动画状态
    float transition_duration_ = 3.0f;
    float transition_elapsed_ = 0.0f;
    bool is_transitioning_ = false;

    // 过渡起始颜色
    int from_sky_top_r_ = 135, from_sky_top_g_ = 206, from_sky_top_b_ = 235, from_sky_top_a_ = 255;
    int from_sky_bottom_r_ = 240, from_sky_bottom_g_ = 248, from_sky_bottom_b_ = 255, from_sky_bottom_a_ = 255;
    int from_horizon_r_ = 255, from_horizon_g_ = 255, from_horizon_b_ = 255, from_horizon_a_ = 200;
    int from_sun_r_ = 255, from_sun_g_ = 255, from_sun_b_ = 200, from_sun_a_ = 100;
    int from_fog_ = 0, from_atmosphere_ = 100, from_particle_ = 0;

    // 过渡目标颜色
    int to_sky_top_r_ = 135, to_sky_top_g_ = 206, to_sky_top_b_ = 235, to_sky_top_a_ = 255;
    int to_sky_bottom_r_ = 240, to_sky_bottom_g_ = 248, to_sky_bottom_b_ = 255, to_sky_bottom_a_ = 255;
    int to_horizon_r_ = 255, to_horizon_g_ = 255, to_horizon_b_ = 255, to_horizon_a_ = 200;
    int to_sun_r_ = 255, to_sun_g_ = 255, to_sun_b_ = 200, to_sun_a_ = 100;
    int to_fog_ = 0, to_atmosphere_ = 100, to_particle_ = 0;
};

}  // namespace CloudSeamanor::domain
