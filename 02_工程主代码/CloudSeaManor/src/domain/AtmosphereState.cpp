// ============================================================================
// 【AtmosphereState.cpp】大气状态领域对象实现
// ============================================================================

#include "CloudSeamanor/AtmosphereState.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace CloudSeamanor::domain {

namespace {

[[nodiscard]] std::string CloudStateToString(CloudState state) {
    switch (state) {
    case CloudState::Clear:      return "clear";
    case CloudState::Mist:       return "mist";
    case CloudState::DenseCloud: return "dense";
    case CloudState::Tide:       return "tide";
    }
    return "clear";
}

[[nodiscard]] std::string DayPhaseToString(DayPhase phase) {
    switch (phase) {
    case DayPhase::Morning:    return "morning";
    case DayPhase::Afternoon:  return "afternoon";
    case DayPhase::Evening:    return "evening";
    case DayPhase::Night:      return "night";
    }
    return "morning";
}

[[nodiscard]] CloudState CloudStateFromString(const std::string& s) {
    if (s == "mist")  return CloudState::Mist;
    if (s == "dense") return CloudState::DenseCloud;
    if (s == "tide")  return CloudState::Tide;
    return CloudState::Clear;
}

[[nodiscard]] DayPhase DayPhaseFromString(const std::string& s) {
    if (s == "afternoon") return DayPhase::Afternoon;
    if (s == "evening")   return DayPhase::Evening;
    if (s == "night")     return DayPhase::Night;
    return DayPhase::Morning;
}

}  // namespace

void AtmosphereDomain::Initialize(
    const std::vector<SkyProfile>& sky_profiles,
    const std::vector<WeatherProfile>& weather_profiles
) {
    sky_profiles_ = &sky_profiles;
    weather_profiles_ = &weather_profiles;
}

void AtmosphereDomain::SyncFromCloudAndClock(
    const CloudSystem& cloud,
    const GameClock& clock
) {
    const CloudState cloud_state = cloud.CurrentState();
    const DayPhase phase = clock.CurrentPhase();

    // 如果云海状态或时段发生变化，重新查找配置
    if (cloud_state != current_cloud_state_ || phase != current_day_phase_) {
        current_cloud_state_ = cloud_state;
        current_day_phase_ = phase;

        const SkyProfile* target = FindSkyProfile(cloud_state, phase);
        if (target != nullptr && target != active_sky_profile_) {
            TransitionTo(*target, transition_duration_);
        }

        active_weather_profile_ = FindWeatherProfile(cloud_state);
    }
}

void AtmosphereDomain::Update(float delta_seconds) {
    if (!is_transitioning_) {
        return;
    }

    transition_elapsed_ += delta_seconds;
    const float t = std::clamp(transition_elapsed_ / transition_duration_, 0.0f, 1.0f);

    // 使用 smoothstep 缓动曲线获得更自然的效果
    const float eased_t = t * t * (3.0f - 2.0f * t);

    InterpolateColors_(eased_t);

    if (t >= 1.0f) {
        is_transitioning_ = false;
        transition_elapsed_ = 0.0f;
    }
}

void AtmosphereDomain::ForceState(CloudState cloud_state, DayPhase phase, bool immediate) {
    current_cloud_state_ = cloud_state;
    current_day_phase_ = phase;

    const SkyProfile* target = FindSkyProfile(cloud_state, phase);
    if (target != nullptr) {
        active_sky_profile_ = target;
        if (immediate) {
            ApplySkyProfile(*target);
            is_transitioning_ = false;
            transition_elapsed_ = 0.0f;
        } else {
            TransitionTo(*target, transition_duration_);
        }
    }

    active_weather_profile_ = FindWeatherProfile(cloud_state);
}

void AtmosphereDomain::TransitionTo(const SkyProfile& target, float duration_seconds) {
    // 保存起始颜色
    from_sky_top_r_ = sky_top_r_;
    from_sky_top_g_ = sky_top_g_;
    from_sky_top_b_ = sky_top_b_;
    from_sky_top_a_ = sky_top_a_;
    from_sky_bottom_r_ = sky_bottom_r_;
    from_sky_bottom_g_ = sky_bottom_g_;
    from_sky_bottom_b_ = sky_bottom_b_;
    from_sky_bottom_a_ = sky_bottom_a_;
    from_horizon_r_ = horizon_r_;
    from_horizon_g_ = horizon_g_;
    from_horizon_b_ = horizon_b_;
    from_horizon_a_ = horizon_a_;
    from_sun_r_ = sun_r_;
    from_sun_g_ = sun_g_;
    from_sun_b_ = sun_b_;
    from_sun_a_ = sun_a_;
    from_fog_ = static_cast<int>(fog_density_ * 100.0f);
    from_atmosphere_ = static_cast<int>(atmosphere_intensity_ * 100.0f);
    from_particle_ = static_cast<int>(particle_density_ * 100.0f);

    // 保存目标颜色
    to_sky_top_r_ = target.sky_top_r;
    to_sky_top_g_ = target.sky_top_g;
    to_sky_top_b_ = target.sky_top_b;
    to_sky_top_a_ = target.sky_top_a;
    to_sky_bottom_r_ = target.sky_bottom_r;
    to_sky_bottom_g_ = target.sky_bottom_g;
    to_sky_bottom_b_ = target.sky_bottom_b;
    to_sky_bottom_a_ = target.sky_bottom_a;
    to_horizon_r_ = target.horizon_r;
    to_horizon_g_ = target.horizon_g;
    to_horizon_b_ = target.horizon_b;
    to_horizon_a_ = target.horizon_a;
    to_sun_r_ = target.sun_r;
    to_sun_g_ = target.sun_g;
    to_sun_b_ = target.sun_b;
    to_sun_a_ = target.sun_a;
    to_fog_ = static_cast<int>(target.fog_density * 100.0f);
    to_atmosphere_ = static_cast<int>(target.atmosphere_intensity * 100.0f);
    to_particle_ = static_cast<int>(target.particle_density * 100.0f);

    transition_duration_ = duration_seconds;
    transition_elapsed_ = 0.0f;
    is_transitioning_ = true;
    active_sky_profile_ = &target;
}

const SkyProfile* AtmosphereDomain::FindSkyProfile(CloudState cloud, DayPhase phase) const {
    if (sky_profiles_ == nullptr) {
        return nullptr;
    }

    const std::string cloud_str = CloudStateToString(cloud);
    const std::string phase_str = DayPhaseToString(phase);

    const SkyProfile* best = nullptr;
    int best_order = -1;

    for (const auto& profile : *sky_profiles_) {
        if (profile.cloud_state == cloud_str && profile.day_phase == phase_str) {
            if (profile.sort_order > best_order) {
                best = &profile;
                best_order = profile.sort_order;
            }
        }
    }
    return best;
}

const WeatherProfile* AtmosphereDomain::FindWeatherProfile(CloudState cloud) const {
    if (weather_profiles_ == nullptr) {
        return nullptr;
    }

    const std::string cloud_str = CloudStateToString(cloud);
    for (const auto& profile : *weather_profiles_) {
        if (profile.cloud_state == cloud_str) {
            return &profile;
        }
    }
    return nullptr;
}

void AtmosphereDomain::ApplySkyProfile(const SkyProfile& profile) {
    sky_top_r_ = profile.sky_top_r;
    sky_top_g_ = profile.sky_top_g;
    sky_top_b_ = profile.sky_top_b;
    sky_top_a_ = profile.sky_top_a;
    sky_bottom_r_ = profile.sky_bottom_r;
    sky_bottom_g_ = profile.sky_bottom_g;
    sky_bottom_b_ = profile.sky_bottom_b;
    sky_bottom_a_ = profile.sky_bottom_a;
    horizon_r_ = profile.horizon_r;
    horizon_g_ = profile.horizon_g;
    horizon_b_ = profile.horizon_b;
    horizon_a_ = profile.horizon_a;
    sun_r_ = profile.sun_r;
    sun_g_ = profile.sun_g;
    sun_b_ = profile.sun_b;
    sun_a_ = profile.sun_a;
    fog_density_ = profile.fog_density;
    atmosphere_intensity_ = profile.atmosphere_intensity;
    particle_density_ = profile.particle_density;
}

void AtmosphereDomain::InterpolateColors_(float t) {
    sky_top_r_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_top_r_), static_cast<float>(to_sky_top_r_), t));
    sky_top_g_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_top_g_), static_cast<float>(to_sky_top_g_), t));
    sky_top_b_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_top_b_), static_cast<float>(to_sky_top_b_), t));
    sky_top_a_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_top_a_), static_cast<float>(to_sky_top_a_), t));
    sky_bottom_r_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_bottom_r_), static_cast<float>(to_sky_bottom_r_), t));
    sky_bottom_g_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_bottom_g_), static_cast<float>(to_sky_bottom_g_), t));
    sky_bottom_b_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_bottom_b_), static_cast<float>(to_sky_bottom_b_), t));
    sky_bottom_a_ = static_cast<int>(std::lerp(static_cast<float>(from_sky_bottom_a_), static_cast<float>(to_sky_bottom_a_), t));
    horizon_r_ = static_cast<int>(std::lerp(static_cast<float>(from_horizon_r_), static_cast<float>(to_horizon_r_), t));
    horizon_g_ = static_cast<int>(std::lerp(static_cast<float>(from_horizon_g_), static_cast<float>(to_horizon_g_), t));
    horizon_b_ = static_cast<int>(std::lerp(static_cast<float>(from_horizon_b_), static_cast<float>(to_horizon_b_), t));
    horizon_a_ = static_cast<int>(std::lerp(static_cast<float>(from_horizon_a_), static_cast<float>(to_horizon_a_), t));
    sun_r_ = static_cast<int>(std::lerp(static_cast<float>(from_sun_r_), static_cast<float>(to_sun_r_), t));
    sun_g_ = static_cast<int>(std::lerp(static_cast<float>(from_sun_g_), static_cast<float>(to_sun_g_), t));
    sun_b_ = static_cast<int>(std::lerp(static_cast<float>(from_sun_b_), static_cast<float>(to_sun_b_), t));
    sun_a_ = static_cast<int>(std::lerp(static_cast<float>(from_sun_a_), static_cast<float>(to_sun_a_), t));
    fog_density_ = std::lerp(static_cast<float>(from_fog_) / 100.0f, static_cast<float>(to_fog_) / 100.0f, t);
    atmosphere_intensity_ = std::lerp(static_cast<float>(from_atmosphere_) / 100.0f, static_cast<float>(to_atmosphere_) / 100.0f, t);
    particle_density_ = std::lerp(static_cast<float>(from_particle_) / 100.0f, static_cast<float>(to_particle_) / 100.0f, t);
}

std::string AtmosphereDomain::SaveState() const {
    std::ostringstream oss;
    oss << "atmosphere|"
        << static_cast<int>(current_cloud_state_) << "|"
        << static_cast<int>(current_day_phase_) << "|"
        << GetTransitionProgress();
    return oss.str();
}

void AtmosphereDomain::LoadState(const std::string& state) {
    std::istringstream iss(state);
    std::string token;
    if (!std::getline(iss, token, '|')) return;
    if (token != "atmosphere") return;

    int cloud_idx = 0, phase_idx = 0;
    float transition_progress = 0.0f;

    if (std::getline(iss, token, '|')) cloud_idx = std::stoi(token);
    if (std::getline(iss, token, '|')) phase_idx = std::stoi(token);
    if (std::getline(iss, token, '|')) transition_progress = std::stof(token);

    ForceState(static_cast<CloudState>(cloud_idx), static_cast<DayPhase>(phase_idx), true);
    (void)transition_progress;
}

}  // namespace CloudSeamanor::domain
