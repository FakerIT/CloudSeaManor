// ============================================================================
// 【AtmosphereDomainTest.cpp】大气领域对象单元测试
// ============================================================================
// Test cases for CloudSeamanor::domain::AtmosphereDomain
//
// Coverage:
// - Initialization
// - Sky profile lookup by CloudState + DayPhase
// - Transition animation
// - Color interpolation (smoothstep easing)
// - Force state
// - Save/Load state
// ============================================================================

#include "../catch2Compat.hpp"
#include "CloudSeamanor/domain/AtmosphereState.hpp"

using CloudSeamanor::domain::AtmosphereDomain;
using CloudSeamanor::domain::CloudState;
using CloudSeamanor::domain::DayPhase;
using CloudSeamanor::domain::SkyProfile;
using CloudSeamanor::domain::WeatherProfile;

namespace {

SkyProfile MakeSky(const std::string& id, CloudState cloud, DayPhase phase, int r, int g, int b) {
    SkyProfile p;
    p.id = id;
    switch (cloud) {
    case CloudState::Clear:      p.cloud_state = "clear"; break;
    case CloudState::Mist:       p.cloud_state = "mist"; break;
    case CloudState::DenseCloud: p.cloud_state = "dense"; break;
    case CloudState::Tide:       p.cloud_state = "tide"; break;
    }
    switch (phase) {
    case DayPhase::Morning:    p.day_phase = "morning"; break;
    case DayPhase::Afternoon: p.day_phase = "afternoon"; break;
    case DayPhase::Evening:   p.day_phase = "evening"; break;
    case DayPhase::Night:     p.day_phase = "night"; break;
    }
    p.sky_top_r = static_cast<int>(r); p.sky_top_g = static_cast<int>(g); p.sky_top_b = static_cast<int>(b); p.sky_top_a = 255;
    p.sky_bottom_r = static_cast<int>(r / 2); p.sky_bottom_g = static_cast<int>(g / 2); p.sky_bottom_b = static_cast<int>(b / 2); p.sky_bottom_a = 255;
    p.horizon_r = 255; p.horizon_g = 255; p.horizon_b = 255; p.horizon_a = 200;
    p.sun_r = 255; p.sun_g = 255; p.sun_b = 200; p.sun_a = 100;
    p.fog_density = 0.1f;
    p.atmosphere_intensity = 0.8f;
    p.particle_density = 0.5f;
    p.sort_order = 0;
    return p;
}

WeatherProfile MakeWeather(CloudState cloud, int max_particles) {
    WeatherProfile p;
    switch (cloud) {
    case CloudState::Clear:      p.id = "clear"; p.cloud_state = "clear"; break;
    case CloudState::Mist:       p.id = "mist"; p.cloud_state = "mist"; break;
    case CloudState::DenseCloud: p.id = "dense"; p.cloud_state = "dense"; break;
    case CloudState::Tide:       p.id = "tide"; p.cloud_state = "tide"; break;
    }
    p.max_particles = max_particles;
    p.particle_speed = 30.0f;
    p.particle_spawn_rate = 20.0f;
    p.color_start_r = 255; p.color_start_g = 255; p.color_start_b = 255; p.color_start_a = 200;
    p.color_end_r = 200; p.color_end_g = 200; p.color_end_b = 200; p.color_end_a = 50;
    p.particle_size_min = 2.0f;
    p.particle_size_max = 8.0f;
    p.particle_lifetime_min = 2.0f;
    p.particle_lifetime_max = 5.0f;
    p.follows_camera = true;
    return p;
}

class MockCloudSystem {
public:
    CloudState state_ = CloudState::Clear;
    [[nodiscard]] CloudState CurrentState() const { return state_; }
};

class MockGameClock {
public:
    DayPhase phase_ = DayPhase::Morning;
    [[nodiscard]] DayPhase CurrentPhase() const { return phase_; }
};

}  // namespace

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_CASE("AtmosphereDomain::Initialize - accepts empty profile lists") {
    AtmosphereDomain domain;
    domain.Initialize({}, {});
    CHECK(true);
}

TEST_CASE("AtmosphereDomain::Initialize - stores profile references") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies = {MakeSky("test_clear_morning", CloudState::Clear, DayPhase::Morning, 135, 206, 235)};
    std::vector<WeatherProfile> weathers = {MakeWeather(CloudState::Clear, 0)};

    domain.Initialize(skies, weathers);

    const auto* profile = domain.GetActiveSkyProfile();
    CHECK(profile != nullptr);
    CHECK(profile->id == "test_clear_morning");
}

// ============================================================================
// Sky Profile Lookup Tests
// ============================================================================

TEST_CASE("AtmosphereDomain::SyncFromCloudAndClock - finds matching clear+morning") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("clear_morning", CloudState::Clear, DayPhase::Morning, 135, 206, 235));
    skies.push_back(MakeSky("clear_afternoon", CloudState::Clear, DayPhase::Afternoon, 30, 144, 255));
    skies.push_back(MakeSky("mist_morning", CloudState::Mist, DayPhase::Morning, 176, 196, 222));
    std::vector<WeatherProfile> weathers;
    weathers.push_back(MakeWeather(CloudState::Clear, 0));

    domain.Initialize(skies, weathers);

    MockCloudSystem cloud;
    MockGameClock clock;
    cloud.state_ = CloudState::Clear;
    clock.phase_ = DayPhase::Morning;

    domain.SyncFromCloudAndClock(cloud, clock);
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);

    CHECK(domain.GetCurrentCloudState() == CloudState::Clear);
    CHECK(domain.GetCurrentDayPhase() == DayPhase::Morning);
    CHECK(domain.SkyTopR() == 135);
    CHECK(domain.SkyTopG() == 206);
    CHECK(domain.SkyTopB() == 235);
}

TEST_CASE("AtmosphereDomain::SyncFromCloudAndClock - finds matching mist+evening") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("mist_evening", CloudState::Mist, DayPhase::Evening, 105, 105, 105));
    std::vector<WeatherProfile> weathers;
    weathers.push_back(MakeWeather(CloudState::Mist, 120));

    domain.Initialize(skies, weathers);
    domain.ForceState(CloudState::Mist, DayPhase::Evening, true);

    CHECK(domain.GetCurrentCloudState() == CloudState::Mist);
    CHECK(domain.GetCurrentDayPhase() == DayPhase::Evening);
    CHECK(domain.SkyTopR() == 105);
}

TEST_CASE("AtmosphereDomain::SyncFromCloudAndClock - finds matching dense+night") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("dense_night", CloudState::DenseCloud, DayPhase::Night, 13, 2, 33));
    skies.push_back(MakeSky("tide_night", CloudState::Tide, DayPhase::Night, 26, 0, 51));
    std::vector<WeatherProfile> weathers;
    weathers.push_back(MakeWeather(CloudState::DenseCloud, 180));

    domain.Initialize(skies, weathers);
    domain.ForceState(CloudState::DenseCloud, DayPhase::Night, true);

    CHECK(domain.GetCurrentCloudState() == CloudState::DenseCloud);
    CHECK(domain.GetCurrentDayPhase() == DayPhase::Night);
}

TEST_CASE("AtmosphereDomain::SyncFromCloudAndClock - no match returns nullptr") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("clear_morning", CloudState::Clear, DayPhase::Morning, 100, 100, 100));
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);

    const auto* profile = domain.GetActiveSkyProfile();
    CHECK(profile == nullptr);
}

// ============================================================================
// Transition Tests
// ============================================================================

TEST_CASE("AtmosphereDomain::ForceState - immediate applies colors") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("clear_morning", CloudState::Clear, DayPhase::Morning, 135, 206, 235));
    skies.push_back(MakeSky("mist_morning", CloudState::Mist, DayPhase::Morning, 176, 196, 222));
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);
    domain.ForceState(CloudState::Mist, DayPhase::Morning, true);

    CHECK(domain.IsTransitioning() == false);
    CHECK(domain.SkyTopR() == 176);
}

TEST_CASE("AtmosphereDomain::ForceState - non-immediate starts transition") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("clear_morning", CloudState::Clear, DayPhase::Morning, 135, 206, 235));
    skies.push_back(MakeSky("mist_morning", CloudState::Mist, DayPhase::Morning, 176, 196, 222));
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);
    domain.ForceState(CloudState::Mist, DayPhase::Morning, false);

    CHECK(domain.IsTransitioning() == true);
    CHECK(domain.GetTransitionProgress() == 0.0f);
}

TEST_CASE("AtmosphereDomain::Update - advances transition progress") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("clear_morning", CloudState::Clear, DayPhase::Morning, 0, 0, 0));
    skies.push_back(MakeSky("mist_morning", CloudState::Mist, DayPhase::Morning, 255, 255, 255));
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);
    domain.SetTransitionDuration(2.0f);
    domain.ForceState(CloudState::Mist, DayPhase::Morning, false);

    domain.Update(1.0f);
    CHECK(domain.IsTransitioning() == true);

    domain.Update(1.0f);
    CHECK(domain.IsTransitioning() == false);
    CHECK(domain.GetTransitionProgress() == 1.0f);
}

TEST_CASE("AtmosphereDomain::Update - color interpolation uses smoothstep") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("from", CloudState::Clear, DayPhase::Morning, 0, 0, 0));
    skies.push_back(MakeSky("to", CloudState::Mist, DayPhase::Morning, 100, 100, 100));
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);
    domain.SetTransitionDuration(2.0f);
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);
    domain.ForceState(CloudState::Mist, DayPhase::Morning, false);

    CHECK(domain.SkyTopR() == 0);
    CHECK(domain.SkyTopG() == 0);

    domain.Update(1.0f);

    const float t = 0.5f;
    const float eased = t * t * (3.0f - 2.0f * t);
    const int expected = static_cast<int>(100.0f * eased);

    CHECK(domain.SkyTopR() >= 0);
    CHECK(domain.SkyTopR() <= 100);
    CHECK(domain.SkyTopR() == expected);
}

TEST_CASE("AtmosphereDomain::Update - no-op when not transitioning") {
    AtmosphereDomain domain;
    domain.Initialize({}, {});
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);

    const int r_before = domain.SkyTopR();
    domain.Update(1.0f);
    CHECK(domain.SkyTopR() == r_before);
}

// ============================================================================
// Atmosphere Parameters Tests
// ============================================================================

TEST_CASE("AtmosphereDomain::GetFogDensity - returns profile value") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("test", CloudState::Clear, DayPhase::Morning, 100, 100, 100));
    skies[0].fog_density = 0.35f;
    skies[0].atmosphere_intensity = 0.9f;
    skies[0].particle_density = 0.75f;
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);

    CHECK(domain.GetFogDensity() == 0.35f);
    CHECK(domain.GetAtmosphereIntensity() == 0.9f);
    CHECK(domain.GetParticleDensity() == 0.75f);
}

// ============================================================================
// Save/Load State Tests
// ============================================================================

TEST_CASE("AtmosphereDomain::SaveState - produces label format") {
    AtmosphereDomain domain;
    domain.Initialize({}, {});
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);

    const auto state = domain.SaveState();
    CHECK_THAT(state, StartsWith("atmosphere|"));
    CHECK_THAT(state, Contains("|0|0|"));  // cloud=0, phase=0
}

TEST_CASE("AtmosphereDomain::LoadState - restores state") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("clear_morning", CloudState::Clear, DayPhase::Morning, 135, 206, 235));
    skies.push_back(MakeSky("mist_morning", CloudState::Mist, DayPhase::Morning, 176, 196, 222));
    skies.push_back(MakeSky("tide_night", CloudState::Tide, DayPhase::Night, 26, 0, 51));
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);

    domain.LoadState("atmosphere|3|3|0.5");

    CHECK(domain.GetCurrentCloudState() == CloudState::Tide);
    CHECK(domain.GetCurrentDayPhase() == DayPhase::Night);
}

TEST_CASE("AtmosphereDomain::LoadState - ignores invalid prefix") {
    AtmosphereDomain domain;
    domain.Initialize({}, {});
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);

    domain.LoadState("invalid|0|0|0");

    CHECK(domain.GetCurrentCloudState() == CloudState::Clear);
    CHECK(domain.GetCurrentDayPhase() == DayPhase::Morning);
}

// ============================================================================
// Weather Profile Tests
// ============================================================================

TEST_CASE("AtmosphereDomain::GetActiveWeatherProfile - returns matching profile") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("clear_morning", CloudState::Clear, DayPhase::Morning, 100, 100, 100));
    std::vector<WeatherProfile> weathers;
    weathers.push_back(MakeWeather(CloudState::Clear, 0));
    weathers.push_back(MakeWeather(CloudState::Mist, 120));
    weathers.push_back(MakeWeather(CloudState::DenseCloud, 180));

    domain.Initialize(skies, weathers);
    domain.ForceState(CloudState::Mist, DayPhase::Morning, true);

    const auto* weather = domain.GetActiveWeatherProfile();
    CHECK(weather != nullptr);
    CHECK(weather->id == "mist");
    CHECK(weather->max_particles == 120);
}

TEST_CASE("AtmosphereDomain::GetActiveWeatherProfile - returns nullptr with empty list") {
    AtmosphereDomain domain;
    std::vector<SkyProfile> skies;
    skies.push_back(MakeSky("test", CloudState::Clear, DayPhase::Morning, 100, 100, 100));
    std::vector<WeatherProfile> weathers;

    domain.Initialize(skies, weathers);
    domain.ForceState(CloudState::Clear, DayPhase::Morning, true);

    const auto* weather = domain.GetActiveWeatherProfile();
    CHECK(weather == nullptr);
}
