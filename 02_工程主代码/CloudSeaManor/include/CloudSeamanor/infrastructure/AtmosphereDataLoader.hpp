// ============================================================================
// 【AtmosphereDataLoader.hpp】大气数据加载器
// ============================================================================
// 从外部数据文件加载大气和天气配置，供 AtmosphereDomain 使用。
//
// 主要职责：
// - 解析 atmosphere_profiles.csv 天空轮廓配置
// - 解析 weather_effects.json 天气粒子配置
// - 提供完整的 Profile 列表给 AtmosphereDomain::Initialize
//
// 与其他系统的关系：
// - 依赖：标准库（无 SFML 依赖）
// - 被依赖：AtmosphereDomain::Initialize
//
// 数据驱动规范：
// - CSV 用于表格型数据（天空轮廓，Excel 可编辑）
// - JSON 用于嵌套型数据（天气粒子参数）
// ============================================================================

#pragma once

#include "CloudSeamanor/AtmosphereState.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

// ============================================================================
// 【AtmosphereDataLoader】大气数据加载器
// ============================================================================
// 加载并解析大气系统的外部配置文件。
//
// 使用示例：
// @code
// AtmosphereDataLoader loader;
// std::vector<domain::SkyProfile> skies;
// std::vector<domain::WeatherProfile> weathers;
//
// bool ok = loader.LoadSkyProfiles(
//     "assets/data/atmosphere/atmosphere_profiles.csv", skies
// );
// ok = ok && loader.LoadWeatherProfiles(
//     "assets/data/atmosphere/weather_effects.json", weathers
// );
//
// AtmosphereDomain domain;
// domain.Initialize(skies, weathers);
// @endcode
class AtmosphereDataLoader {
public:
    // ========================================================================
    // 【LoadSkyProfiles】加载天空轮廓配置（CSV）
    // ========================================================================
    // @param file_path CSV 文件路径
    // @param out_profiles 输出：解析后的天空轮廓列表
    // @return 是否加载成功
    // @note CSV 格式：id,cloud_state,day_phase,sky_top_r,sky_top_g,sky_top_b,sky_top_a,
    //                    sky_bottom_r,sky_bottom_g,sky_bottom_b,sky_bottom_a,
    //                    horizon_r,horizon_g,horizon_b,horizon_a,
    //                    sun_r,sun_g,sun_b,sun_a,
    //                    ambient_r,ambient_g,ambient_b,ambient_a,
    //                    fog_density,atmosphere_intensity,particle_density,sort_order
    [[nodiscard]] bool LoadSkyProfiles(
        const std::filesystem::path& file_path,
        std::vector<domain::SkyProfile>& out_profiles
    ) const;

    // ========================================================================
    // 【LoadWeatherProfiles】加载天气粒子配置（JSON）
    // ========================================================================
    // @param file_path JSON 文件路径
    // @param out_profiles 输出：解析后的天气配置列表
    // @return 是否加载成功
    // @note JSON 格式：{ "clear": {...}, "mist": {...}, "dense": {...}, "tide": {...} }
    [[nodiscard]] bool LoadWeatherProfiles(
        const std::filesystem::path& file_path,
        std::vector<domain::WeatherProfile>& out_profiles
    ) const;

    // ========================================================================
    // 【LoadAll】一次性加载所有大气数据
    // ========================================================================
    // @param data_dir assets/data/atmosphere/ 目录路径
    // @param out_skies 输出：天空轮廓列表
    // @param out_weathers 输出：天气配置列表
    // @return 是否全部加载成功
    [[nodiscard]] bool LoadAll(
        const std::filesystem::path& data_dir,
        std::vector<domain::SkyProfile>& out_skies,
        std::vector<domain::WeatherProfile>& out_weathers
    ) const;

    // ========================================================================
    // 【LoadWithDefaults】加载数据，失败时填充默认值
    // ========================================================================
    // @param data_dir assets/data/atmosphere/ 目录路径
    // @param out_skies 输出：天空轮廓列表（加载失败时填充默认）
    // @param out_weathers 输出：天气配置列表（加载失败时填充默认）
    // @return 是否有任何文件加载成功
    [[nodiscard]] bool LoadWithDefaults(
        const std::filesystem::path& data_dir,
        std::vector<domain::SkyProfile>& out_skies,
        std::vector<domain::WeatherProfile>& out_weathers
    ) const;

    // ========================================================================
    // 【GetLastError】获取上次错误信息
    // ========================================================================
    [[nodiscard]] const std::string& GetLastError() const noexcept { return last_error_; }

private:
    // ========================================================================
    // 【ParseHexColor】解析 RGBA 十六进制字符串
    // ========================================================================
    // @param hex 格式 "#RRGGBB" 或 "#RRGGBBAA" 的字符串
    // @param out_r/out_g/out_b/out_a 输出颜色分量
    // @return 是否解析成功
    [[nodiscard]] static bool ParseHexColor(
        const std::string& hex,
        int& out_r, int& out_g, int& out_b, int& out_a
    );

    mutable std::string last_error_;
};

}  // namespace CloudSeamanor::infrastructure
