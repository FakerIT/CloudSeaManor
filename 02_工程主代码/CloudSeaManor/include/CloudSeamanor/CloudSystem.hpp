#pragma once

// ============================================================================
// 【CloudSystem】云海天气与灵气管理系统
// ============================================================================
// 统一管理游戏的云海天气状态、灵气值、玩家影响和大潮机制。
//
// 主要职责：
// - 管理4种云海天气状态（晴朗、薄雾、浓云海、大潮）
// - 维护灵气值（影响大潮概率）
// - 追踪玩家行为对云海的累计影响
// - 生成次日天气预报（22:00后可见）
// - 计算不同天气下的灵气收益
//
// 与其他系统的关系：
// - 依赖：无（纯领域层）
// - 被依赖：GameApp（主循环）、GameAppHud（天气显示）、
//           GameAppSave（存档/读档）
//
// 设计原则：
// - 所有天气状态均为正向加成，无惩罚天气
// - 大潮是最高奖励日，传说灵茶/灵兽必出
// - 新手保护期前14天强制薄雾以上
// - 灵气值和玩家行为共同影响天气概率
//
// 天气效果：
// - 晴朗：基础生长
// - 薄雾：作物+15%，灵茶品质+1级，灵气+12
// - 浓云海：作物+40%，稀有掉落+50%，灵气+25
// - 大潮：传说必出，灵界掉落翻倍，灵气+80
// ============================================================================

#include <string>

namespace CloudSeamanor::domain {

// ============================================================================
// 【CloudState】云海天气状态枚举
// ============================================================================
// 四种状态均为正向加成，大潮是最高奖励日。
//
// 状态说明：
// - Clear（晴朗）：基础生长效率
// - Mist（薄雾）：轻度加成，适合新手期
// - DenseCloud（浓云海）：中高加成，稀有产出提升
// - Tide（大潮）：最高奖励日，传说必出
//
// 使用方式：
// - UI根据状态显示不同背景和特效
// - 游戏逻辑根据状态计算加成倍率
// - 存档系统保存/恢复状态
enum class CloudState {
    Clear,       // 晴空（基础状态）
    Mist,        // 薄雾（轻度加成）
    DenseCloud,   // 浓云海（中高加成）
    Tide,        // 大潮（最高奖励）
};

// ============================================================================
// 【CloudSystem】云海系统领域对象
// ============================================================================
// 负责云海状态生成、灵气联动、玩家影响、预报机制。
//
// 设计决策：
// - 天气概率由灵气值和玩家行为共同决定
// - 大潮概率随灵气值和正向影响累积提升
// - 预报在22:00后可见，支持玩家规划次日
// - 所有数值可配置化（通过GameConfig）
//
// 使用示例：
// @code
// CloudSystem cloud;
// cloud.AdvanceToNextDay(clock.Day());  // 推进到新的一天
// cloud.UpdateForecastVisibility(clock.Day(), clock.Hour());  // 更新预报
// if (cloud.IsDenseOrTide()) { /* 高效种田 */ }
// @endcode
class CloudSystem {
public:
    // ========================================================================
    // 【AdvanceToNextDay】推进到新的一天
    // ========================================================================
    // @param day 当前游戏天数（从1开始）
    // @note 结算本日灵气收益
    // @note 根据天数和灵气值生成今日天气和明日预报
    // @note 预报默认隐藏，等22:00后公布
    // @calledby GameApp::SleepToNextMorning()
    void AdvanceToNextDay(int day);

    // ========================================================================
    // 【UpdateForecastVisibility】更新天气预报可见性
    // ========================================================================
    // @param day 当前游戏天数
    // @param hour 当前小时（0-23）
    // @note 22:00后预报可见，之前显示"22:00后公布"
    // @calledby GameApp::Update()
    void UpdateForecastVisibility(int day, int hour);

    // ========================================================================
    // 【ApplyPlayerInfluence】应用玩家行为对云海的影响
    // ========================================================================
    // @param influence_value 影响值（正值=正向影响，负值=负向影响）
    // @note 累计值影响后续天气概率
    // @note 正向行为：种树、修缮、恢复地脉
    // @calledby GameApp::SleepToNextMorning()
    void ApplyPlayerInfluence(int influence_value);

    // ========================================================================
    // 【CycleDebugState】调试：循环切换天气（不含大潮）
    // ========================================================================
    // @note 顺序：Clear -> Mist -> DenseCloud -> Clear
    // @note 用于开发期手动验证各天气效果
    void CycleDebugState();

    // ========================================================================
    // 【ForceTide】调试：强制设置大潮
    // ========================================================================
    // @note 用于测试大潮状态下的各种效果
    // @calledby GameApp::ProcessEvents()（F7快捷键）
    void ForceTide();

    // ========================================================================
    // 【SetStates】设置天气状态（用于读档）
    // ========================================================================
    // @param current_state 今日天气
    // @param forecast_state 明日预报
    // @calledby GameAppSave::LoadGame()
    void SetStates(CloudState current_state, CloudState forecast_state);

    // ========================================================================
    // 【状态查询接口】
    // ========================================================================
    [[nodiscard]] CloudState CurrentState() const noexcept { return current_state_; }
    [[nodiscard]] CloudState ForecastState() const noexcept { return forecast_state_; }
    [[nodiscard]] bool IsForecastVisible() const noexcept { return forecast_visible_; }

    // ========================================================================
    // 【SpiritEnergy】灵气值查询
    // ========================================================================
    [[nodiscard]] int SpiritEnergy() const noexcept { return spirit_energy_; }

    // ========================================================================
    // 【SetSpiritEnergy】设置灵气值
    // ========================================================================
    // @param value 新的灵气值（将被限制为非负数）
    void SetSpiritEnergy(int value);

    // ========================================================================
    // 【SpiritEnergyGain】根据当前天气获取灵气增量
    // ========================================================================
    // @return 今日灵气增量
    // @note Clear=5, Mist=12, DenseCloud=25, Tide=80
    [[nodiscard]] int SpiritEnergyGain() const noexcept;

    // ========================================================================
    // 【TotalPlayerInfluence】累计玩家影响力
    // ========================================================================
    [[nodiscard]] int TotalPlayerInfluence() const noexcept { return total_player_influence_; }

    // ========================================================================
    // 【状态判断接口】
    // ========================================================================
    [[nodiscard]] bool IsTide() const noexcept {
        return current_state_ == CloudState::Tide;
    }

    [[nodiscard]] bool IsDenseOrTide() const noexcept {
        return current_state_ == CloudState::DenseCloud || current_state_ == CloudState::Tide;
    }

    [[nodiscard]] bool IsMistOrBetter() const noexcept {
        return current_state_ != CloudState::Clear;
    }

    [[nodiscard]] float CurrentSpiritDensity() const noexcept {
        switch (current_state_) {
        case CloudState::Clear:    return 0.0f;
        case CloudState::Mist:     return 0.3f;
        case CloudState::DenseCloud: return 0.7f;
        case CloudState::Tide:    return 1.0f;
        }
        return 0.0f;
    }

    // ========================================================================
    // 【文本接口】
    // ========================================================================
    [[nodiscard]] std::string CurrentStateText() const;
    [[nodiscard]] std::string ForecastStateText() const;
    [[nodiscard]] std::string CurrentStateHint() const;
    [[nodiscard]] std::string AmbientSfxId() const;

    // ========================================================================
    // 【存档接口】
    // ========================================================================
    // 保存当前云海系统状态，用于存档
    [[nodiscard]] std::string SaveState() const;

    // 恢复云海系统状态，用于读档
    void LoadState(const std::string& state);

private:
    // ========================================================================
    // 【StateForDay】根据日期和参数生成天气状态
    // ========================================================================
    // @param day 游戏天数
    // @param spirit_energy 当前灵气值
    // @param player_influence 累计玩家影响
    // @return 今日应该使用的天气状态
    //
    // 概率模型：
    // - 前14天：新手保护，强制Mist或DenseCloud
    // - 基础概率：Clear=40%, Mist=35%, DenseCloud=20%, Tide=5%
    // - 灵气加成：高灵气提升DenseCloud和Tide概率
    // - 玩家影响：正向影响进一步提升稀有天气概率
    [[nodiscard]] CloudState StateForDay(
        int day,
        int spirit_energy,
        int player_influence
    ) const noexcept;

    // ========================================================================
    // 【ToText】天气状态转可读文本
    // ========================================================================
    [[nodiscard]] static std::string ToText(CloudState state);

    // ========================================================================
    // 【ToHint】天气状态转简短提示
    // ========================================================================
    [[nodiscard]] static std::string ToHint(CloudState state);

    // ========================================================================
    // 成员变量
    // ========================================================================
    CloudState current_state_ = CloudState::Clear;      // 今日天气
    CloudState forecast_state_ = CloudState::Mist;     // 明日预报
    bool forecast_visible_ = false;                    // 预报是否可见

    int spirit_energy_ = 100;           // 灵气值（影响大潮概率）
    int total_player_influence_ = 0;   // 累计玩家影响力
};

}  // namespace CloudSeamanor::domain
