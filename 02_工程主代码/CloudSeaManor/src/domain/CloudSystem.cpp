#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/CloudSystem.hpp"

#include <algorithm>
#include <cstdlib>
#include <random>
#include <sstream>

namespace CloudSeamanor::domain {

// ============================================================================
// 【AdvanceToNextDay】推进到新的一天
// ============================================================================
void CloudSystem::AdvanceToNextDay(int day) {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::random_device{}()));
        seeded = true;
    }

    // 结算本日的灵气收益
    spirit_energy_ += SpiritEnergyGain();

    // 生成今日天气和明日预报
    current_state_ = StateForDay(day, spirit_energy_, total_player_influence_);
    forecast_state_ = StateForDay(day + 1, spirit_energy_, total_player_influence_);

    // 预报默认隐藏，等22:00后公布
    forecast_visible_ = false;
}

// ============================================================================
// 【UpdateForecastVisibility】更新天气预报可见性
// ============================================================================
void CloudSystem::UpdateForecastVisibility(int day, int hour) {
    // 22:00后公开次日预报
    forecast_state_ = StateForDay(day + 1, spirit_energy_, total_player_influence_);
    forecast_visible_ = hour >= 22;
}

// ============================================================================
// 【ApplyPlayerInfluence】应用玩家行为对云海的影响
// ============================================================================
void CloudSystem::ApplyPlayerInfluence(int influence_value) {
    total_player_influence_ += influence_value;
}

// ============================================================================
// 【CycleDebugState】调试：循环切换天气
// ============================================================================
void CloudSystem::CycleDebugState() {
    switch (current_state_) {
    case CloudState::Clear:
        current_state_ = CloudState::Mist;
        break;
    case CloudState::Mist:
        current_state_ = CloudState::DenseCloud;
        break;
    case CloudState::DenseCloud:
        current_state_ = CloudState::Clear;
        break;
    case CloudState::Tide:
        current_state_ = CloudState::Clear;
        break;
    }
}

// ============================================================================
// 【ForceTide】调试：强制设置大潮
// ============================================================================
void CloudSystem::ForceTide() {
    current_state_ = CloudState::Tide;
    forecast_state_ = CloudState::Tide;
}

// ============================================================================
// 【SetStates】设置天气状态（读档）
// ============================================================================
void CloudSystem::SetStates(CloudState current_state, CloudState forecast_state) {
    current_state_ = current_state;
    forecast_state_ = forecast_state;
}

// ============================================================================
// 【SetSpiritEnergy】设置灵气值
// ============================================================================
void CloudSystem::SetSpiritEnergy(int value) {
    spirit_energy_ = std::max(0, value);
}

// ============================================================================
// 【SpiritEnergyGain】根据当前天气获取灵气增量
// ============================================================================
int CloudSystem::SpiritEnergyGain() const noexcept {
    switch (current_state_) {
    case CloudState::Clear:       return 5;
    case CloudState::Mist:        return 12;
    case CloudState::DenseCloud:  return 25;
    case CloudState::Tide:        return 80;
    }
    return 5;
}

// ============================================================================
// 【CurrentStateText】获取当前天气文本
// ============================================================================
std::string CloudSystem::CurrentStateText() const {
    return ToText(current_state_);
}

// ============================================================================
// 【ForecastStateText】获取预报文本
// ============================================================================
std::string CloudSystem::ForecastStateText() const {
    return forecast_visible_ ? ToText(forecast_state_) : "22:00后公布";
}

// ============================================================================
// 【CurrentStateHint】获取当前天气提示
// ============================================================================
std::string CloudSystem::CurrentStateHint() const {
    return ToHint(current_state_);
}

std::string CloudSystem::AmbientSfxId() const {
    switch (current_state_) {
    case CloudState::Clear: return "";
    case CloudState::Mist: return "wind_soft.ogg";
    case CloudState::DenseCloud: return "wind_strong.ogg";
    case CloudState::Tide: return "tide_magic.ogg";
    }
    return "";
}

// ============================================================================
// 【SaveState】保存状态
// ============================================================================
std::string CloudSystem::SaveState() const {
    std::ostringstream oss;
    oss << static_cast<int>(current_state_) << ","
         << static_cast<int>(forecast_state_) << ","
         << spirit_energy_ << ","
         << total_player_influence_;
    return oss.str();
}

// ============================================================================
// 【LoadState】加载状态
// ============================================================================
void CloudSystem::LoadState(const std::string& state) {
    if (state.empty()) return;

    std::istringstream iss(state);
    int current = 0, forecast = 0;
    char comma;

    if (iss >> current >> comma >> forecast >> comma >> spirit_energy_ >> comma >> total_player_influence_) {
        current_state_ = static_cast<CloudState>(current);
        forecast_state_ = static_cast<CloudState>(forecast);
    }
}

// ============================================================================
// 【StateForDay】根据日期和参数生成天气状态
// ============================================================================
CloudState CloudSystem::StateForDay(
    int day,
    int spirit_energy,
    int player_influence
) const noexcept {
    // 新手保护：前14天强制薄雾以上
    if (day <= 14) {
        const int cycle = day % 3;
        if (cycle == 0) return CloudState::Mist;
        if (cycle == 1) return CloudState::DenseCloud;
        return CloudState::Mist;
    }

    // 灵气加成：高灵气提升浓云海/大潮概率
    const float spirit_bonus = std::clamp(
        static_cast<float>(spirit_energy) / 500.0f,
        0.0f,
        1.0f
    );

    // 玩家影响加成
    const float influence_bonus = std::clamp(
        static_cast<float>(player_influence) / 100.0f,
        -0.5f,
        0.5f
    );

    // 大潮概率：基础5%，灵气+10%，正向影响+5%
    const float tide_chance = 0.05f + spirit_bonus * 0.10f + std::max(0.0f, influence_bonus * 0.05f);
    if (tide_chance > static_cast<float>(std::rand()) / RAND_MAX) {
        return CloudState::Tide;
    }

    // 浓云海概率：基础20%，灵气+15%，正向影响+10%
    const float dense_chance = 0.20f + spirit_bonus * 0.15f + std::max(0.0f, influence_bonus * 0.10f);
    if (dense_chance > static_cast<float>(std::rand()) / RAND_MAX) {
        return CloudState::DenseCloud;
    }

    // 薄雾概率：基础35%
    const float mist_chance = 0.35f + std::max(0.0f, influence_bonus * 0.10f);
    if (mist_chance > static_cast<float>(std::rand()) / RAND_MAX) {
        return CloudState::Mist;
    }

    return CloudState::Clear;
}

// ============================================================================
// 【ToText】天气状态转可读文本
// ============================================================================
std::string CloudSystem::ToText(CloudState state) {
    switch (state) {
    case CloudState::Clear:       return "晴空";
    case CloudState::Mist:        return "薄雾";
    case CloudState::DenseCloud:  return "浓云海";
    case CloudState::Tide:        return "大潮";
    }
    return "未知";
}

// ============================================================================
// 【ToHint】天气状态转简短提示
// ============================================================================
std::string CloudSystem::ToHint(CloudState state) {
    switch (state) {
    case CloudState::Clear:
        return "今日晴空，基础正常生长";
    case CloudState::Mist:
        return "薄雾缭绕，作物生长+15%，灵茶品质+1级";
    case CloudState::DenseCloud:
        return "浓云海降临，作物生长+40%，稀有灵茶/灵兽出现率+50%";
    case CloudState::Tide:
        return "大潮涌动，传说灵茶/灵兽必出，灵界掉落翻倍！";
    }
    return "";
}

}  // namespace CloudSeamanor::domain
