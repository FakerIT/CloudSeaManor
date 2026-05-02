#pragma once

// ============================================================================
// 【WorkshopSystemRuntime】工坊运行时协调器
// ============================================================================
// Responsibilities:
// - Per-frame workshop update (progress, outputs)
// - Workshop progress bar UI update
// - Domain system access passthrough
// ============================================================================

#include "CloudSeamanor/engine/GameWorldSystems.hpp"

#include <functional>
#include <string>

namespace CloudSeamanor::engine {

// HintCallback 在命名空间级别定义，供所有相关函数使用
using HintCallback = std::function<void(const std::string&, float)>;

class GameWorldState;

class WorkshopSystemRuntime {
public:
    WorkshopSystemRuntime(
        GameWorldSystems& systems,
        GameWorldState& world_state,
        HintCallback hint_callback
    );

    void Update(float delta_seconds);
    void UpdateProgressBar() const;

    [[nodiscard]] CloudSeamanor::domain::WorkshopSystem& DomainSystem();
    [[nodiscard]] const CloudSeamanor::domain::WorkshopSystem& DomainSystem() const;
    [[nodiscard]] float GetProgress(const std::string& machine_id) const;
    [[nodiscard]] const CloudSeamanor::domain::MachineState* GetMachine(
        const std::string& machine_id) const;

private:
    GameWorldSystems& systems_;
    GameWorldState& world_state_;
    HintCallback hint_callback_;
};

}  // namespace CloudSeamanor::engine
