#include "TestFramework.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"

using CloudSeamanor::engine::RegisterTest;

namespace CloudSeamanor {
namespace tests {

// ============================================================================
// CloudSystem 测试组
// ============================================================================

TEST_CASE(TestCloudSystemInitialState) {
    domain::CloudSystem cloud;
    ASSERT_TRUE(cloud.CurrentState() == domain::CloudState::Mist
             || cloud.CurrentState() == domain::CloudState::Clear);
    return true;
}

TEST_CASE(TestCloudSystemAdvanceToNextDay) {
    domain::CloudSystem cloud;
    cloud.AdvanceToNextDay(1);
    ASSERT_TRUE(cloud.CurrentState() != domain::CloudState::Tide);  // 不会总是大潮
    return true;
}

TEST_CASE(TestCloudSystemSpiritDensity) {
    domain::CloudSystem cloud;
    cloud.AdvanceToNextDay(5);
    float density = cloud.CurrentSpiritDensity();
    ASSERT_TRUE(density >= 0.0f && density <= 3.0f);
    return true;
}

TEST_CASE(TestCloudSystemSpiritEnergyGain) {
    domain::CloudSystem cloud;
    cloud.AdvanceToNextDay(3);
    int gain = cloud.SpiritEnergyGain();
    ASSERT_TRUE(gain >= 0);
    return true;
}

// ============================================================================
// GameClock 测试组
// ============================================================================

TEST_CASE(TestGameClockInitialValues) {
    domain::GameClock clock;
    ASSERT_EQ(clock.Day(), 1);
    ASSERT_EQ(clock.Hour(), 8);
    ASSERT_EQ(clock.Minute(), 0);
    return true;
}

TEST_CASE(TestGameClockTickAdvancesTime) {
    domain::GameClock clock;
    clock.Tick(60.0f);  // 60 秒游戏时间
    int h = clock.Hour();
    int m = clock.Minute();
    ASSERT_TRUE(h == 8 && m == 1);  // 1分钟流逝
    return true;
}

TEST_CASE(TestGameClockDayBoundary) {
    domain::GameClock clock;
    // 游戏时间 1 天 = 14400 秒（10 分钟）
    clock.Tick(14400.0f);
    ASSERT_EQ(clock.Day(), 2);
    return true;
}

TEST_CASE(TestGameClockSeason) {
    domain::GameClock clock;
    ASSERT_TRUE(clock.Season() == domain::Season::Spring);
    return true;
}

TEST_CASE(TestGameClockDayInSeason) {
    domain::GameClock clock;
    ASSERT_TRUE(clock.DayInSeason() >= 1);
    return true;
}

TEST_CASE(TestGameClockDayInYear) {
    domain::GameClock clock;
    ASSERT_TRUE(clock.DayInYear() >= 1);
    return true;
}

TEST_CASE(TestGameClockCurrentPhase) {
    domain::GameClock clock;
    auto phase = clock.CurrentPhase();
    ASSERT_TRUE(phase == domain::TimePhase::Dawn
             || phase == domain::TimePhase::Morning
             || phase == domain::TimePhase::Noon
             || phase == domain::TimePhase::Afternoon
             || phase == domain::TimePhase::Evening
             || phase == domain::TimePhase::Night);
    return true;
}

}  // namespace tests
}  // namespace CloudSeamanor
