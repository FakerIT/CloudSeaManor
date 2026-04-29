#pragma once

#include <functional>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>

namespace CloudSeamanor::engine {

// ============================================================================
// 【委托类型定义】
// ============================================================================
using SystemUpdateDelegate = std::function<void(float delta_seconds)>;
using ConditionDelegate = std::function<bool()>;

// ============================================================================
// 【LoopPhase】循环阶段枚举
// ============================================================================
enum class LoopPhase : std::uint8_t {
    Time = 0,      // 时间更新
    Input = 1,     // 输入处理
    World = 2,     // 世界更新
    Combat = 3,    // 战斗更新
    Runtime = 4,   // 运行时更新
    Particles = 5,// 粒子特效
    Count = 6     // 总阶段数
};

// ============================================================================
// 【PhaseStats】阶段性能统计
// ============================================================================
struct PhaseStats {
    std::string phase_name;
    int call_count = 0;
    float total_time_ms = 0.0f;
    float max_time_ms = 0.0f;
    float min_time_ms = 999999.0f;

    float AverageTimeMs() const {
        return call_count > 0 ? total_time_ms / call_count : 0.0f;
    }
};

// ============================================================================
// 【SystemLoopStats】系统性能统计
// ============================================================================
struct SystemLoopStats {
    std::string system_name;
    LoopPhase phase;
    int call_count = 0;
    float total_time_ms = 0.0f;
    float last_time_ms = 0.0f;
};

// ============================================================================
// 【LoopConfig】循环配置
// ============================================================================
struct LoopConfig {
    bool enable_profiling = false;
    bool enable_phase_skip = true;
    bool enable_idle_skip = true;
    float idle_threshold_ms = 16.67f;
    int stats_history_size = 60;
};

// ============================================================================
// 【RegisteredSystem】已注册系统
// ============================================================================
struct RegisteredSystem {
    SystemUpdateDelegate update_fn;
    ConditionDelegate condition_fn;
    LoopPhase phase;
    int priority = 0;
    std::string system_name;
    bool enabled = true;

    bool ShouldUpdate() const {
        if (!enabled) return false;
        if (condition_fn && !condition_fn()) return false;
        return true;
    }
};

// ============================================================================
// 【GameLoopCoordinator】游戏循环协调器
// ============================================================================
class GameLoopCoordinator {
public:
    GameLoopCoordinator();
    explicit GameLoopCoordinator(const LoopConfig& config);

    void SetConfig(const LoopConfig& config);
    [[nodiscard]] const LoopConfig& GetConfig() const { return config_; }

    void RegisterSystem(
        LoopPhase phase,
        const std::string& system_name,
        SystemUpdateDelegate update_fn,
        ConditionDelegate condition_fn,
        int priority = 0);

    void RegisterSystem(
        LoopPhase phase,
        const std::string& system_name,
        SystemUpdateDelegate update_fn,
        int priority = 0);

    void UnregisterSystem(const std::string& system_name);
    void EnableSystem(const std::string& system_name, bool enabled);

    void Update(float delta_seconds);
    void UpdatePhase(LoopPhase phase, float delta_seconds);

    [[nodiscard]] const PhaseStats& GetPhaseStats(LoopPhase phase) const;
    [[nodiscard]] const std::vector<SystemLoopStats>& GetSystemStats() const;
    [[nodiscard]] float GetTotalFrameTimeMs() const;
    void ResetStats();
    [[nodiscard]] std::string GetDebugReport() const;

private:
    void SortSystemsByPhaseAndPriority_();
    void CollectSystemsForPhase_(LoopPhase phase, std::vector<const RegisteredSystem*>& out) const;

    LoopConfig config_;
    std::vector<RegisteredSystem> systems_;
    std::unordered_map<LoopPhase, PhaseStats> phase_stats_;
    std::vector<SystemLoopStats> system_stats_;

    using Clock = std::chrono::high_resolution_clock;
    std::chrono::time_point<Clock> frame_start_;

    float last_total_time_ms_ = 0.0f;
};

const char* GetPhaseName(LoopPhase phase);

constexpr std::array<LoopPhase, 6> kPhaseOrder = {
    LoopPhase::Time,
    LoopPhase::Input,
    LoopPhase::World,
    LoopPhase::Combat,
    LoopPhase::Runtime,
    LoopPhase::Particles
};

} // namespace CloudSeamanor::engine
