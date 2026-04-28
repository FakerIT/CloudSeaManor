#include "CloudSeamanor/engine/systems/PickupSystemRuntime.hpp"

#include "CloudSeamanor/GameAppText.hpp"
#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/PickupSystem.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【PickupSystemRuntime::PickupSystemRuntime】构造函数
// ============================================================================
PickupSystemRuntime::PickupSystemRuntime(
    PickupSystem& pickup_system,
    GameWorldState& world_state,
    HintCallback hint_callback
)
    : pickup_system_(pickup_system)
    , world_state_(world_state)
    , hint_callback_(std::move(hint_callback))
{
}

// ============================================================================
// 【PickupSystemRuntime::Initialize】初始化（重新注册回调）
// ============================================================================
void PickupSystemRuntime::Initialize() {
    PickupCallbacks cbs{
        [this](const std::string& msg, float dur) {
            if (hint_callback_) hint_callback_(msg, dur);
        },
        CloudSeamanor::infrastructure::Logger::Info
    };
    pickup_system_.Initialize(cbs);
}

// ============================================================================
// 【PickupSystemRuntime::Update】每帧更新拾取
// ============================================================================
void PickupSystemRuntime::Update(float delta_seconds) {
    pickup_system_.Update(
        delta_seconds,
        world_state_.GetWorldTipPulse(),
        world_state_.GetPlayer(),
        world_state_.MutableInventory());
}

// ============================================================================
// 【PickupSystemRuntime::CollectNearby】收集范围内拾取物
// ============================================================================
void PickupSystemRuntime::CollectNearby() {
    for (auto it = world_state_.MutablePickups().begin();
         it != world_state_.MutablePickups().end();) {
        if (it->IsCollectedBy(world_state_.GetPlayer().Bounds())) {
            world_state_.MutableInventory().AddItem(it->ItemId(), it->Amount());
            if (hint_callback_) {
                hint_callback_(
                    "已获得 " + ItemDisplayName(it->ItemId()) + " x" +
                    std::to_string(it->Amount()) + "。", 2.2f);
            }
            it = world_state_.MutablePickups().erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace CloudSeamanor::engine
