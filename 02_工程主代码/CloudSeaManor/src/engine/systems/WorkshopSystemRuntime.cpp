#include "CloudSeamanor/engine/systems/WorkshopSystemRuntime.hpp"

#include "CloudSeamanor/app/GameAppText.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/TeaProductData.hpp"
#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/TextRenderUtils.hpp"

#include <unordered_map>

namespace CloudSeamanor::engine {

namespace {
int DetectWorkshopToolLevel_(const CloudSeamanor::domain::Inventory& inventory) {
    if (inventory.CountOf("hoe_spirit") > 0 || inventory.CountOf("watering_can_spirit") > 0) {
        return 4;
    }
    if (inventory.CountOf("hoe_gold") > 0 || inventory.CountOf("watering_can_gold") > 0) {
        return 3;
    }
    if (inventory.CountOf("hoe_silver") > 0 || inventory.CountOf("watering_can_silver") > 0) {
        return 2;
    }
    return 1;
}
}  // namespace

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
    static const bool tea_table_loaded =
        CloudSeamanor::domain::GetGlobalTeaProductTable().LoadFromFile("assets/data/tea/tea_products.csv");
    (void)tea_table_loaded;

    const float cloud_density = systems_.GetCloud().CurrentSpiritDensity();
    const int skill_level =
        systems_.GetSkills().GetLevel(CloudSeamanor::domain::SkillType::SpiritFarm);
    const int tool_level = DetectWorkshopToolLevel_(world_state_.GetInventory());
    std::unordered_map<std::string, int> outputs;
    systems_.UpdateWorkshop(delta_seconds, cloud_density, skill_level, tool_level, outputs);

    for (const auto& [item_id, count] : outputs) {
        if (item_id == "TeaPack") {
            world_state_.MutableTeaMachine().queued_output += count;
            if (hint_callback_) {
                hint_callback_("灵茶加工完成！按 E 领取", 3.0f);
            }
            continue;
        }
        world_state_.MutableInventory().AddItem(item_id, count);
        if (hint_callback_) {
            if (const auto* tea = CloudSeamanor::domain::GetGlobalTeaProductTable().Get(item_id)) {
                hint_callback_(
                    "工坊产出 " + ItemDisplayName(item_id) + " x" + std::to_string(count)
                    + "（品质:" + tea->quality
                    + "，售价:" + std::to_string(tea->sell_price)
                    + "，自用:" + tea->buff_effect_id
                    + "，赠礼:" + tea->gift_preference + "）", 3.2f);
            } else {
                hint_callback_(
                    "工坊产出 " + ItemDisplayName(item_id) + " x" +
                    std::to_string(count) + "。", 3.0f);
            }
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
