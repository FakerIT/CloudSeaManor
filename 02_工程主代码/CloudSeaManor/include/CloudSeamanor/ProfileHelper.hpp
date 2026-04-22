// ============================================================================
// 性能分析辅助工具
// 用于云海山庄游戏的性能监控和分析
// ============================================================================

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace CloudSeamanor::profiling {

// ============================================================================
// 【ProfileScope】RAII 性能分析范围计时器
// ============================================================================
class ProfileScope {
public:
    explicit ProfileScope(const char* name, float threshold_ms = 1.0f)
        : name_(name)
        , threshold_ms_(threshold_ms)
        , start_(std::chrono::high_resolution_clock::now())
    {
    }

    ~ProfileScope() {
        auto end = std::chrono::high_resolution_clock::now();
        float elapsed_ms = std::chrono::duration<float, std::milli>(end - start_).count();
        if (elapsed_ms > threshold_ms_) {
            std::cerr << "[PERF] " << name_ << " 耗时: " 
                      << std::fixed << elapsed_ms << "ms\n";
        }
    }

private:
    const char* name_;
    float threshold_ms_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// ============================================================================
// 【FrameTimeMonitor】帧时间监控器
// ============================================================================
class FrameTimeMonitor {
public:
    explicit FrameTimeMonitor(size_t history_size = 120)
        : history_size_(history_size)
        , frame_times_(history_size, 0.0f)
        , frame_index_(0)
    {
    }

    void RecordFrame(float delta_seconds) {
        frame_times_[frame_index_ % history_size_] = delta_seconds;
        ++frame_index_;
    }

    float GetAverageFrameTime() const {
        size_t count = std::min(frame_index_, history_size_);
        if (count == 0) return 0.0f;
        
        float sum = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            sum += frame_times_[i];
        }
        return sum / static_cast<float>(count);
    }

    float GetFPS() const {
        float avg_time = GetAverageFrameTime();
        if (avg_time <= 0.0f) return 0.0f;
        return 1.0f / avg_time;
    }

    float GetMinFrameTime() const {
        size_t count = std::min(frame_index_, history_size_);
        if (count == 0) return 0.0f;
        
        auto it = std::min_element(frame_times_.begin(), frame_times_.begin() + count);
        return *it;
    }

    float GetMaxFrameTime() const {
        size_t count = std::min(frame_index_, history_size_);
        if (count == 0) return 0.0f;
        
        auto it = std::max_element(frame_times_.begin(), frame_times_.begin() + count);
        return *it;
    }

    float GetPercentileFrameTime(float percentile) const {
        size_t count = std::min(frame_index_, history_size_);
        if (count == 0) return 0.0f;
        
        std::vector<float> sorted(frame_times_.begin(), frame_times_.begin() + count);
        std::sort(sorted.begin(), sorted.end());
        
        size_t index = static_cast<size_t>(percentile * count);
        index = std::min(index, count - 1);
        return sorted[index];
    }

    std::string GetStatsString() const {
        std::ostringstream oss;
        oss << "FPS: " << static_cast<int>(GetFPS())
            << " | 平均: " << std::fixed << GetAverageFrameTime() * 1000.0f << "ms"
            << " | 最低: " << GetMinFrameTime() * 1000.0f << "ms"
            << " | 最高: " << GetMaxFrameTime() * 1000.0f << "ms"
            << " | P95: " << GetPercentileFrameTime(0.95f) * 1000.0f << "ms";
        return oss.str();
    }

private:
    size_t history_size_;
    std::vector<float> frame_times_;
    size_t frame_index_;
};

// ============================================================================
// 【PerformanceCounter】通用性能计数器
// ============================================================================
class PerformanceCounter {
public:
    void Start(const std::string& name) {
        start_times_[name] = std::chrono::high_resolution_clock::now();
    }

    void Stop(const std::string& name) {
        auto start_it = start_times_.find(name);
        if (start_it == start_times_.end()) {
            return;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        float elapsed_ms = std::chrono::duration<float, std::milli>(end - start_it->second).count();
        
        auto& data = counters_[name];
        data.total_time += elapsed_ms;
        data.call_count++;
        data.min_time = std::min(data.min_time, elapsed_ms);
        data.max_time = std::max(data.max_time, elapsed_ms);
        
        start_times_.erase(start_it);
    }

    void Report() const {
        std::cout << "\n========== 性能报告 ==========\n";
        for (const auto& [name, data] : counters_) {
            float avg_time = data.total_time / data.call_count;
            std::cout << name << ":\n"
                      << "  调用次数: " << data.call_count << "\n"
                      << "  总耗时: " << std::fixed << data.total_time << "ms\n"
                      << "  平均: " << avg_time << "ms\n"
                      << "  最低: " << data.min_time << "ms\n"
                      << "  最高: " << data.max_time << "ms\n\n";
        }
        std::cout << "==============================\n";
    }

    void Reset() {
        counters_.clear();
        start_times_.clear();
    }

private:
    struct CounterData {
        float total_time = 0.0f;
        float min_time = std::numeric_limits<float>::max();
        float max_time = 0.0f;
        size_t call_count = 0;
    };

    std::unordered_map<std::string, CounterData> counters_;
    std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> start_times_;
};

// ============================================================================
// 【宏定义】便捷使用
// ============================================================================

// 性能分析范围宏
#define PROFILE_SCOPE(name) \
    CloudSeamanor::profiling::ProfileScope _profile_scope_##__LINE__(name)

// 带阈值的性能分析宏(仅当超过阈值时才输出)
#define PROFILE_SCOPE_THRESHOLD(name, threshold_ms) \
    CloudSeamanor::profiling::ProfileScope _profile_scope_##__LINE__(name, threshold_ms)

// 全局性能计数器(使用静态局部变量)
inline CloudSeamanor::profiling::PerformanceCounter& GetGlobalProfiler() {
    static CloudSeamanor::profiling::PerformanceCounter instance;
    return instance;
}

#define PROFILE_START(name) GetGlobalProfiler().Start(name)
#define PROFILE_STOP(name) GetGlobalProfiler().Stop(name)
#define PROFILE_REPORT() GetGlobalProfiler().Report()
#define PROFILE_RESET() GetGlobalProfiler().Reset()

} // namespace CloudSeamanor::profiling
