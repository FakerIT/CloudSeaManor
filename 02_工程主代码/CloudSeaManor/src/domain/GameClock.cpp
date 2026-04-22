#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/GameClock.hpp"

#include <iomanip>
#include <sstream>

namespace CloudSeamanor::domain {

// ============================================================================
// 【Initialize】初始化时钟
// ============================================================================
void GameClock::Initialize(int day, float minutes) {
    SetState(day, minutes);
}

// ============================================================================
// 【Tick】推进游戏时间
// ============================================================================
void GameClock::Tick(float delta_seconds) {
    minutes_in_day_ += delta_seconds;

    // 处理跨天
    while (minutes_in_day_ >= kMinutesPerDay) {
        minutes_in_day_ -= kMinutesPerDay;
        ++day_;
    }

    // 处理跨年
    AdvanceYear_();
    UpdateSeasonChangedEvent_();
}

// ============================================================================
// 【SleepToNextMorning】睡到第二天早晨
// ============================================================================
void GameClock::SleepToNextMorning() {
    ++day_;
    minutes_in_day_ = static_cast<float>(kMorningStart * kMinutesPerHour);
    AdvanceYear_();
    UpdateSeasonChangedEvent_();
}

// ============================================================================
// 【AdvanceDay】推进到下一天
// ============================================================================
void GameClock::AdvanceDay() {
    ++day_;
    AdvanceYear_();
    UpdateSeasonChangedEvent_();
}

// ============================================================================
// 【SetState】恢复时钟状态（读档）
// ============================================================================
void GameClock::SetState(int day, float minutes) {
    day_ = std::max(1, day);
    minutes_in_day_ = minutes;

    // 修正越界
    while (minutes_in_day_ < 0.0f) {
        minutes_in_day_ += kMinutesPerDay;
    }
    while (minutes_in_day_ >= kMinutesPerDay) {
        minutes_in_day_ -= kMinutesPerDay;
    }
    cached_season_ = Season();
    pending_season_changed_event_ = {};
}

// ============================================================================
// 【Season】获取当前季节
// ============================================================================
CloudSeamanor::domain::Season GameClock::Season() const noexcept {
    return static_cast<CloudSeamanor::domain::Season>(SeasonIndex());
}

// ============================================================================
// 【DayPhase】获取当前时段
// ============================================================================
DayPhase GameClock::CurrentDayPhase() const noexcept {
    const int h = Hour();
    if (h >= 6 && h < 12) return DayPhase::Morning;
    if (h >= 12 && h < 18) return DayPhase::Afternoon;
    if (h >= 18 && h < 22) return DayPhase::Evening;
    return DayPhase::Night;
}

// ============================================================================
// 【SeasonIndex】获取季节索引
// ============================================================================
int GameClock::SeasonIndex() const noexcept {
    return ((day_ - 1) % kDaysPerYear) / kDaysPerSeason;
}

// ============================================================================
// 【DayInSeason】当前是本季节第几天
// ============================================================================
int GameClock::DayInSeason() const noexcept {
    return ((day_ - 1) % kDaysPerSeason) + 1;
}

// ============================================================================
// 【CurrentPhase】获取当前时段
// ============================================================================
DayPhase GameClock::CurrentPhase() const noexcept {
    const int hour = Hour();

    if (hour < kMorningStart) {
        return DayPhase::Night;
    }
    if (hour < kAfternoonStart) {
        return DayPhase::Morning;
    }
    if (hour < kEveningStart) {
        return DayPhase::Afternoon;
    }
    if (hour < kNightStart) {
        return DayPhase::Evening;
    }
    return DayPhase::Night;
}

// ============================================================================
// 【TimeText】获取时间文本
// ============================================================================
std::string GameClock::TimeText() const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << Hour() << ':'
         << std::setfill('0') << std::setw(2) << Minute();
    return oss.str();
}

// ============================================================================
// 【DateText】获取日期文本
// ============================================================================
std::string GameClock::DateText() const {
    std::ostringstream oss;
    oss << SeasonName(Season()) << " 第" << DayInSeason() << "天";
    return oss.str();
}

// ============================================================================
// 【FullDateText】获取完整日期文本
// ============================================================================
std::string GameClock::FullDateText() const {
    std::ostringstream oss;
    oss << "第" << Year() << "年第" << DayInYear() << "天 "
         << SeasonName(Season());
    return oss.str();
}

// ============================================================================
// 【PhaseText】获取时段文本
// ============================================================================
std::string GameClock::PhaseText() const {
    return PhaseName(CurrentPhase());
}

// ============================================================================
// 【MinutesUntilPhase】距离指定时段的分钟数
// ============================================================================
float GameClock::MinutesUntilPhase(DayPhase phase) const noexcept {
    int target_hour = 0;
    switch (phase) {
    case DayPhase::Morning:   target_hour = kMorningStart; break;
    case DayPhase::Afternoon: target_hour = kAfternoonStart; break;
    case DayPhase::Evening:   target_hour = kEveningStart; break;
    case DayPhase::Night:    target_hour = kNightStart; break;
    }

    int current_hour = Hour();
    int hour_diff = target_hour - current_hour;

    // 跨天处理
    if (hour_diff <= 0) {
        hour_diff += kHoursPerDay;
    }

    return static_cast<float>(hour_diff * kMinutesPerHour);
}

// ============================================================================
// 【SeasonName】获取季节名称
// ============================================================================
std::string GameClock::SeasonName(CloudSeamanor::domain::Season s) {
    switch (s) {
    case CloudSeamanor::domain::Season::Spring: return "春";
    case CloudSeamanor::domain::Season::Summer: return "夏";
    case CloudSeamanor::domain::Season::Autumn: return "秋";
    case CloudSeamanor::domain::Season::Winter: return "冬";
    }
    return "未知";
}

// ============================================================================
// 【PhaseName】获取时段名称
// ============================================================================
std::string GameClock::PhaseName(DayPhase phase) {
    switch (phase) {
    case DayPhase::Morning:    return "清晨";
    case DayPhase::Afternoon:  return "午后";
    case DayPhase::Evening:   return "傍晚";
    case DayPhase::Night:     return "夜晚";
    }
    return "未知";
}

GameClock::SeasonChangedEvent GameClock::ConsumeSeasonChangedEvent() noexcept {
    const SeasonChangedEvent ev = pending_season_changed_event_;
    pending_season_changed_event_ = {};
    return ev;
}

// ============================================================================
// 【AdvanceYear_】推进年份（内部）
// ============================================================================
void GameClock::AdvanceYear_() {
    while (day_ > kDaysPerYear) {
        day_ -= kDaysPerYear;
    }
}

void GameClock::UpdateSeasonChangedEvent_() noexcept {
    const auto current = Season();
    if (current == cached_season_) {
        return;
    }
    pending_season_changed_event_.changed = true;
    pending_season_changed_event_.from = cached_season_;
    pending_season_changed_event_.to = current;
    cached_season_ = current;
}

}  // namespace CloudSeamanor::domain
