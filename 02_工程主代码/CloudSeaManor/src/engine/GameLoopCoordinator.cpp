#include "CloudSeamanor/engine/GameLoopCoordinator.hpp"
#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::engine {

// ============================================================================
// 【GameLoopCoordinator】构造函数
// ============================================================================
GameLoopCoordinator::GameLoopCoordinator() {
    phase_names_.resize(static_cast<std::size_t>(LoopPhase::Count));
    for (int i = 0; i < static_cast<int>(LoopPhase::Count); ++i) {
        phase_names_[i] = GetPhaseName(static_cast<LoopPhase>(i));
        phase_stats_[static_cast<LoopPhase>(i)].phase_name = phase_names_[i];
    }
}

GameLoopCoordinator::GameLoopCoordinator(const LoopConfig& config)
    : config_(config) {
    phase_names_.resize(static_cast<std::size_t>(LoopPhase::Count));
    for (int i = 0; i < static_cast<int>(LoopPhase::Count); ++i) {
        phase_names_[i] = GetPhaseName(static_cast<LoopPhase>(i));
        phase_stats_[static_cast<LoopPhase>(i)].phase_name = phase_names_[i];
    }
}

// ============================================================================
// 【配置】
// ============================================================================
void GameLoopCoordinator::SetConfig(const LoopConfig& config) {
    config_ = config;
}

// ============================================================================
// 【系统注册】
// ============================================================================
void GameLoopCoordinator::RegisterSystem(
    LoopPhase phase,
    const std::string& system_name,
    SystemUpdateDelegate update_fn,
    ConditionDelegate condition_fn,
    int priority) {

    RegisteredSystem rs;
    rs.phase = phase;
    rs.system_name = system_name;
    rs.update_fn = std::move(update_fn);
    rs.condition_fn = std::move(condition_fn);
    rs.priority = priority;
    rs.enabled = true;

    systems_.push_back(std::move(rs));
    SortSystemsByPhaseAndPriority_();

    // 初始化系统统计
    SystemLoopStats stats;
    stats.system_name = system_name;
    stats.phase = phase;
    system_stats_.push_back(stats);
}

void GameLoopCoordinator::RegisterSystem(
    LoopPhase phase,
    const std::string& system_name,
    SystemUpdateDelegate update_fn,
    int priority) {
    RegisterSystem(phase, system_name, std::move(update_fn), {}, priority);
}

void GameLoopCoordinator::UnregisterSystem(const std::string& system_name) {
    systems_.erase(
        std::remove_if(systems_.begin(), systems_.end(),
            [&system_name](const RegisteredSystem& rs) {
                return rs.system_name == system_name;
            }),
        systems_.end());

    system_stats_.erase(
        std::remove_if(system_stats_.begin(), system_stats_.end(),
            [&system_name](const SystemLoopStats& stats) {
                return stats.system_name == system_name;
            }),
        system_stats_.end());
}

void GameLoopCoordinator::EnableSystem(const std::string& system_name, bool enabled) {
    for (auto& rs : systems_) {
        if (rs.system_name == system_name) {
            rs.enabled = enabled;
            break;
        }
    }
}

// ============================================================================
// 【排序】
// ============================================================================
void GameLoopCoordinator::SortSystemsByPhaseAndPriority_() {
    std::sort(systems_.begin(), systems_.end(),
        [](const RegisteredSystem& a, const RegisteredSystem& b) {
            if (a.phase != b.phase) return a.phase < b.phase;
            return a.priority < b.priority;
        });
}

void GameLoopCoordinator::CollectSystemsForPhase_(
    LoopPhase phase,
    std::vector<const RegisteredSystem*>& out) const {
    out.clear();
    for (const auto& rs : systems_) {
        if (rs.phase == phase) {
            out.push_back(&rs);
        }
    }
}

// ============================================================================
// 【主循环更新】
// ============================================================================
void GameLoopCoordinator::Update(float delta_seconds) {
    if (config_.enable_profiling) {
        frame_start_ = Clock::now();
    }

    // 按阶段顺序执行所有系统
    for (const auto phase : kPhaseOrder) {
        UpdatePhase(phase, delta_seconds);
    }

    if (config_.enable_profiling) {
        auto now = Clock::now();
        auto elapsed = std::chrono::duration<float, std::milli>(now - frame_start_).count();
        last_total_time_ms_ = elapsed;
    }
}

void GameLoopCoordinator::UpdatePhase(LoopPhase phase, float delta_seconds) {
    std::vector<const RegisteredSystem*> phase_systems;
    CollectSystemsForPhase_(phase, phase_systems);

    if (phase_systems.empty()) return;

    auto& phase_stat = phase_stats_[phase];
    if (config_.enable_profiling) {
        auto phase_start = Clock::now();
        for (const auto* rs : phase_systems) {
            if (!rs->ShouldUpdate()) continue;

            auto sys_start = Clock::now();
            rs->update_fn(delta_seconds);
            auto sys_end = Clock::now();

            float sys_time = std::chrono::duration<float, std::milli>(sys_end - sys_start).count();

            // 更新系统统计
            for (auto& stats : system_stats_) {
                if (stats.system_name == rs->system_name) {
                    stats.last_time_ms = sys_time;
                    stats.total_time_ms += sys_time;
                    stats.call_count++;
                    break;
                }
            }
        }
        auto phase_end = Clock::now();
        float phase_time = std::chrono::duration<float, std::milli>(phase_end - phase_start).count();

        phase_stat.total_time_ms += phase_time;
        phase_stat.call_count++;
        phase_stat.max_time_ms = std::max(phase_stat.max_time_ms, phase_time);
        phase_stat.min_time_ms = std::min(phase_stat.min_time_ms, phase_time);
    } else {
        for (const auto* rs : phase_systems) {
            if (rs->ShouldUpdate()) {
                rs->update_fn(delta_seconds);
            }
        }
    }
}

// ============================================================================
// 【性能统计】
// ============================================================================
const PhaseStats& GameLoopCoordinator::GetPhaseStats(LoopPhase phase) const {
    static PhaseStats empty;
    auto it = phase_stats_.find(phase);
    return it != phase_stats_.end() ? it->second : empty;
}

const std::vector<SystemLoopStats>& GameLoopCoordinator::GetSystemStats() const {
    return system_stats_;
}

float GameLoopCoordinator::GetTotalFrameTimeMs() const {
    return last_total_time_ms_;
}

bool GameLoopCoordinator::IsFrameBusy() const {
    return last_total_time_ms_ > config_.idle_threshold_ms;
}

void GameLoopCoordinator::ResetStats() {
    for (auto& [phase, stats] : phase_stats_) {
        stats.call_count = 0;
        stats.total_time_ms = 0.0f;
        stats.max_time_ms = 0.0f;
        stats.min_time_ms = 999999.0f;
    }
    for (auto& stats : system_stats_) {
        stats.call_count = 0;
        stats.total_time_ms = 0.0f;
        stats.last_time_ms = 0.0f;
    }
    last_total_time_ms_ = 0.0f;
}

// ============================================================================
// 【调试报告】
// ============================================================================
std::string GameLoopCoordinator::GetDebugReport() const {
    std::ostringstream oss;
    oss << "=== GameLoopCoordinator Debug Report ===\n";
    oss << "Total Frame Time: " << last_total_time_ms_ << " ms\n\n";

    oss << "Phase Stats:\n";
    for (const auto phase : kPhaseOrder) {
        const auto& stats = GetPhaseStats(phase);
        oss << "  " << GetPhaseName(phase) << ": "
            << "avg=" << stats.AverageTimeMs() << " ms, "
            << "max=" << stats.max_time_ms << " ms, "
            << "min=" << stats.min_time_ms << " ms\n";
    }

    oss << "\nSystem Stats:\n";
    for (const auto& stats : system_stats_) {
        if (stats.call_count > 0) {
            oss << "  " << stats.system_name << ": "
                << "avg=" << (stats.total_time_ms / stats.call_count) << " ms\n";
        }
    }

    return oss.str();
}

// ============================================================================
// 【阶段名称】
// ============================================================================
const char* GetPhaseName(LoopPhase phase) {
    switch (phase) {
    case LoopPhase::Time:     return "Time";
    case LoopPhase::Input:    return "Input";
    case LoopPhase::World:    return "World";
    case LoopPhase::Combat:   return "Combat";
    case LoopPhase::Runtime:  return "Runtime";
    case LoopPhase::Particles: return "Particles";
    default:                  return "Unknown";
    }
}

} // namespace CloudSeamanor::engine
