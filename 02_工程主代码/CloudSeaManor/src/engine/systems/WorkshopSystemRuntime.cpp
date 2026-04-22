#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp"

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/TextRenderUtils.hpp"

#include <unordered_map>

namespace CloudSeamanor::engine {

// ============================================================================
// 【WorkshopSystemRuntime::WorkshopSystemRuntime】构造函数
// ============================================================================
WorkshopSystemRuntime::WorkshopSystemRuntime(
    GameWorldSystems& systems,
    GameWorldState& world_state,
    HintCallback hint_callback
)
    : systems_(systems)
    , world_state_(world_state)
    , hint_callback_(std::move(hint_callback))
{
}

// ============================================================================
// 【WorkshopSystemRuntime::Update】每帧更新工坊
// ============================================================================
void WorkshopSystemRuntime::Update(float delta_seconds) {
    const float cloud_density = systems_.GetCloud().CurrentSpiritDensity();
    std::unordered_map<std::string, int> outputs;
    systems_.UpdateWorkshop(delta_seconds, cloud_density, outputs);

    for (const auto& [item_id, count] : outputs) {
        if (item_id == "TeaPack") {
            world_state_.GetTeaMachine().queued_output += count;
            if (hint_callback_) {
                hint_callback_("灵茶加工完成！按 E 领取", 3.0f);
            }
            continue;
        }
        world_state_.GetInventory().AddItem(item_id, count);
        if (hint_callback_) {
            hint_callback_(
                "工坊产出 " + ItemDisplayName(item_id) + " x" +
                std::to_string(count) + "。", 3.0f);
        }
    }
}

// ============================================================================
// 【WorkshopSystemRuntime::UpdateProgressBar】更新工坊进度条 UI
// ============================================================================
void WorkshopSystemRuntime::UpdateProgressBar() const {
    ::CloudSeamanor::engine::UpdateWorkshopProgressBar(
        world_state_, systems_.GetWorkshop());
}

// ============================================================================
// 【WorkshopSystemRuntime::DomainSystem】获取领域系统引用
// ============================================================================
CloudSeamanor::domain::WorkshopSystem& WorkshopSystemRuntime::DomainSystem() {
    return systems_.GetWorkshop();
}

const CloudSeamanor::domain::WorkshopSystem& WorkshopSystemRuntime::DomainSystem() const {
    return systems_.GetWorkshop();
}

// ============================================================================
// 【WorkshopSystemRuntime::GetProgress】获取机器进度
// ============================================================================
float WorkshopSystemRuntime::GetProgress(const std::string& machine_id) const {
    return systems_.GetWorkshop().GetMachineProgress(machine_id);
}

// ============================================================================
// 【WorkshopSystemRuntime::GetMachine】获取机器状态
// ============================================================================
const CloudSeamanor::domain::MachineState* WorkshopSystemRuntime::GetMachine(
    const std::string& machine_id
) const {
    return systems_.GetWorkshop().GetMachine(machine_id);
}

}  // namespace CloudSeamanor::engine
