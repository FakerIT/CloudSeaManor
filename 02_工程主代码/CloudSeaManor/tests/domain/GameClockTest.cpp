// ============================================================================
// 【GameClockTest.cpp】GameClock 单元测试
// ============================================================================
// Test cases for CloudSeamanor::domain::GameClock
//
// Coverage:
// - Initialization
// - Time advancement
// - Day transitions
// - Season calculations
// - Year wrapping
// - Day phase determination
// - Text formatting
// - SleepToNextMorning
// ============================================================================

#include "../catch2Compat.hpp"
#include "CloudSeamanor/GameClock.hpp"

using CloudSeamanor::domain::DayPhase;
using CloudSeamanor::domain::GameClock;
using CloudSeamanor::domain::Season;

namespace {

constexpr int kMinutesPerDay = GameClock::kMinutesPerDay;

}  // namespace

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_CASE("GameClock::Initialize - default values")
{
    GameClock clock;
    clock.Initialize();

    CHECK_EQ(clock.Day(), 1);
    CHECK_EQ(clock.Hour(), 6);
    CHECK_EQ(clock.Minute(), 0);
}

TEST_CASE("GameClock::Initialize - custom day and time")
{
    GameClock clock;
    clock.Initialize(5, 14 * 60.0f);

    CHECK_EQ(clock.Day(), 5);
    CHECK_EQ(clock.Hour(), 14);
    CHECK_EQ(clock.Minute(), 0);
}

TEST_CASE("GameClock::Initialize - fractional minutes")
{
    GameClock clock;
    clock.Initialize(1, 9 * 60.0f + 30.0f);

    CHECK_EQ(clock.Hour(), 9);
    CHECK_EQ(clock.Minute(), 30);
}

// ============================================================================
// Tick / Time Advancement Tests
// ============================================================================

TEST_CASE("GameClock::Tick - advances minutes")
{
    GameClock clock;
    clock.Initialize(1, 6 * 60.0f);

    clock.Tick(60.0f);

    CHECK_EQ(clock.Minute(), 1);
}

TEST_CASE("GameClock::Tick - multiple minutes")
{
    GameClock clock;
    clock.Initialize(1, 6 * 60.0f);

    clock.Tick(600.0f);

    CHECK_EQ(clock.Hour(), 7);
    CHECK_EQ(clock.Minute(), 10);
}

TEST_CASE("GameClock::Tick - advances hour")
{
    GameClock clock;
    clock.Initialize(1, 6 * 60.0f);

    clock.Tick(3600.0f);

    CHECK_EQ(clock.Hour(), 7);
}

TEST_CASE("GameClock::Tick - hour boundary")
{
    GameClock clock;
    clock.Initialize(1, 6 * 60.0f + 30.0f);

    clock.Tick(1800.0f);

    CHECK_EQ(clock.Hour(), 7);
}

// ============================================================================
// Day Transition Tests
// ============================================================================

TEST_CASE("GameClock::Tick - midnight transition")
{
    GameClock clock;
    clock.Initialize(1, 23 * 60.0f + 59.0f);

    clock.Tick(120.0f);

    CHECK_EQ(clock.Day(), 2);
    CHECK_EQ(clock.Hour(), 0);
    CHECK_EQ(clock.Minute(), 1);
}

TEST_CASE("GameClock::Tick - multiple days advance")
{
    GameClock clock;
    clock.Initialize(1, 23 * 60.0f);

    clock.Tick(7200.0f);

    CHECK_EQ(clock.Day(), 2);
    CHECK_EQ(clock.Hour(), 1);
}

TEST_CASE("GameClock::SleepToNextMorning - advances day")
{
    GameClock clock;
    clock.Initialize(1, 22 * 60.0f);

    clock.SleepToNextMorning();

    CHECK_EQ(clock.Day(), 2);
    CHECK_EQ(clock.Hour(), 6);
}

TEST_CASE("GameClock::SleepToNextMorning - time reset to morning")
{
    GameClock clock;
    clock.Initialize(1, 22 * 60.0f);

    clock.SleepToNextMorning();

    CHECK_EQ(clock.Minute(), 0);
}

TEST_CASE("GameClock::AdvanceDay - forces next day")
{
    GameClock clock;
    clock.Initialize(5, 20 * 60.0f);

    clock.AdvanceDay();

    CHECK_EQ(clock.Day(), 6);
}

// ============================================================================
// Season Calculation Tests
// ============================================================================

TEST_CASE("GameClock::Season - spring days 1-28")
{
    GameClock clock;

    clock.Initialize(1);
    CHECK_EQ(clock.Season(), Season::Spring);

    clock.Initialize(28);
    CHECK_EQ(clock.Season(), Season::Spring);
}

TEST_CASE("GameClock::Season - summer days 29-56")
{
    GameClock clock;

    clock.Initialize(29);
    CHECK_EQ(clock.Season(), Season::Summer);

    clock.Initialize(56);
    CHECK_EQ(clock.Season(), Season::Summer);
}

TEST_CASE("GameClock::Season - autumn days 57-84")
{
    GameClock clock;

    clock.Initialize(57);
    CHECK_EQ(clock.Season(), Season::Autumn);

    clock.Initialize(84);
    CHECK_EQ(clock.Season(), Season::Autumn);
}

TEST_CASE("GameClock::Season - winter days 85-112")
{
    GameClock clock;

    clock.Initialize(85);
    CHECK_EQ(clock.Season(), Season::Winter);

    clock.Initialize(112);
    CHECK_EQ(clock.Season(), Season::Winter);
}

TEST_CASE("GameClock::SeasonIndex - returns 0-3")
{
    GameClock clock;

    clock.Initialize(1);
    CHECK_EQ(clock.SeasonIndex(), 0);

    clock.Initialize(29);
    CHECK_EQ(clock.SeasonIndex(), 1);

    clock.Initialize(57);
    CHECK_EQ(clock.SeasonIndex(), 2);

    clock.Initialize(85);
    CHECK_EQ(clock.SeasonIndex(), 3);
}

TEST_CASE("GameClock::SeasonName - returns Chinese names")
{
    CHECK_THAT(GameClock::SeasonName(Season::Spring), Equals("春"));
    CHECK_THAT(GameClock::SeasonName(Season::Summer), Equals("夏"));
    CHECK_THAT(GameClock::SeasonName(Season::Autumn), Equals("秋"));
    CHECK_THAT(GameClock::SeasonName(Season::Winter), Equals("冬"));
}

// ============================================================================
// Year Wrapping Tests
// ============================================================================

TEST_CASE("GameClock::Year - year 1 days 1-112")
{
    GameClock clock;
    clock.Initialize(50);

    CHECK_EQ(clock.Year(), 1);
}

TEST_CASE("GameClock::Year - year 2 day 113")
{
    GameClock clock;
    clock.Initialize(113);

    CHECK_EQ(clock.Year(), 2);
}

TEST_CASE("GameClock::Year - year wrapping")
{
    GameClock clock;
    clock.Initialize(112);

    clock.Tick(60.0f);

    CHECK_EQ(clock.Year(), 2);
    CHECK_EQ(clock.Day(), 1);
    CHECK_EQ(clock.Season(), Season::Spring);
}

TEST_CASE("GameClock::DayInYear - returns 1-112")
{
    GameClock clock;

    clock.Initialize(1);
    CHECK_EQ(clock.DayInYear(), 1);

    clock.Initialize(112);
    CHECK_EQ(clock.DayInYear(), 112);

    clock.Initialize(113);
    CHECK_EQ(clock.DayInYear(), 1);
}

TEST_CASE("GameClock::DayInSeason - returns 1-28")
{
    GameClock clock;

    clock.Initialize(1);
    CHECK_EQ(clock.DayInSeason(), 1);

    clock.Initialize(28);
    CHECK_EQ(clock.DayInSeason(), 28);

    clock.Initialize(29);
    CHECK_EQ(clock.DayInSeason(), 1);
}

// ============================================================================
// Day Phase Tests
// ============================================================================

TEST_CASE("GameClock::CurrentPhase - morning 06-12")
{
    GameClock clock;

    clock.Initialize(1, 6 * 60.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Morning);

    clock.Initialize(1, 11 * 60.0f + 59.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Morning);
}

TEST_CASE("GameClock::CurrentPhase - afternoon 12-18")
{
    GameClock clock;

    clock.Initialize(1, 12 * 60.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Afternoon);

    clock.Initialize(1, 17 * 60.0f + 59.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Afternoon);
}

TEST_CASE("GameClock::CurrentPhase - evening 18-22")
{
    GameClock clock;

    clock.Initialize(1, 18 * 60.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Evening);

    clock.Initialize(1, 21 * 60.0f + 59.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Evening);
}

TEST_CASE("GameClock::CurrentPhase - night 22-06")
{
    GameClock clock;

    clock.Initialize(1, 22 * 60.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Night);

    clock.Initialize(1, 23 * 60.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Night);

    clock.Initialize(2, 5 * 60.0f);
    CHECK_EQ(clock.CurrentPhase(), DayPhase::Night);
}

TEST_CASE("GameClock::PhaseName - returns Chinese names")
{
    CHECK_THAT(GameClock::PhaseName(DayPhase::Morning), Equals("清晨"));
    CHECK_THAT(GameClock::PhaseName(DayPhase::Afternoon), Equals("午后"));
    CHECK_THAT(GameClock::PhaseName(DayPhase::Evening), Equals("傍晚"));
    CHECK_THAT(GameClock::PhaseName(DayPhase::Night), Equals("夜间"));
}

TEST_CASE("GameClock::IsNightTime - true at night")
{
    GameClock clock;
    clock.Initialize(1, 23 * 60.0f);

    CHECK_TRUE(clock.IsNightTime());
}

TEST_CASE("GameClock::IsNightTime - false during day")
{
    GameClock clock;
    clock.Initialize(1, 12 * 60.0f);

    CHECK_FALSE(clock.IsNightTime());
}

TEST_CASE("GameClock::IsRestTime - true after 22:00")
{
    GameClock clock;
    clock.Initialize(1, 22 * 60.0f);

    CHECK_TRUE(clock.IsRestTime());
}

TEST_CASE("GameClock::IsRestTime - true before 06:00")
{
    GameClock clock;
    clock.Initialize(1, 5 * 60.0f);

    CHECK_TRUE(clock.IsRestTime());
}

TEST_CASE("GameClock::IsRestTime - false during day")
{
    GameClock clock;
    clock.Initialize(1, 12 * 60.0f);

    CHECK_FALSE(clock.IsRestTime());
}

// ============================================================================
// Text Formatting Tests
// ============================================================================

TEST_CASE("GameClock::TimeText - formats HH:MM")
{
    GameClock clock;

    clock.Initialize(1, 6 * 60.0f);
    CHECK_THAT(clock.TimeText(), Equals("06:00"));

    clock.Initialize(1, 9 * 60.0f + 5.0f);
    CHECK_THAT(clock.TimeText(), Equals("09:05"));

    clock.Initialize(1, 14 * 60.0f + 30.0f);
    CHECK_THAT(clock.TimeText(), Equals("14:30"));
}

TEST_CASE("GameClock::DateText - includes season and day")
{
    GameClock clock;

    clock.Initialize(1);
    CHECK_THAT(clock.DateText(), Contains("春"));
    CHECK_THAT(clock.DateText(), Contains("第1天"));

    clock.Initialize(29);
    CHECK_THAT(clock.DateText(), Contains("夏"));
    CHECK_THAT(clock.DateText(), Contains("第1天"));
}

TEST_CASE("GameClock::FullDateText - includes year")
{
    GameClock clock;
    clock.Initialize(113);

    auto text = clock.FullDateText();
    CHECK_THAT(text, Contains("第2年"));
}

// ============================================================================
// SetState / State Restoration Tests
// ============================================================================

TEST_CASE("GameClock::SetState - restores day and time")
{
    GameClock clock;
    clock.Initialize(1, 6 * 60.0f);

    clock.SetState(10, 14 * 60.0f);

    CHECK_EQ(clock.Day(), 10);
    CHECK_EQ(clock.Hour(), 14);
}

TEST_CASE("GameClock::SetState - after day transition")
{
    GameClock clock;
    clock.Initialize(1, 6 * 60.0f);

    clock.SetState(50, 20 * 60.0f);

    CHECK_EQ(clock.Day(), 50);
    CHECK_EQ(clock.Hour(), 20);
}

// ============================================================================
// MinutesUntilPhase Tests
// ============================================================================

TEST_CASE("GameClock::MinutesUntilPhase - morning to afternoon")
{
    GameClock clock;
    clock.Initialize(1, 8 * 60.0f);

    auto minutes = clock.MinutesUntilPhase(DayPhase::Afternoon);

    CHECK_EQ(minutes, 4 * 60.0f);
}

TEST_CASE("GameClock::MinutesUntilPhase - already past")
{
    GameClock clock;
    clock.Initialize(1, 15 * 60.0f);

    auto minutes = clock.MinutesUntilPhase(DayPhase::Morning);

    CHECK_TRUE(minutes < 0);
}
