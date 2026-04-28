#include "CloudSeamanor/PickupSystem.hpp"

#include "CloudSeamanor/GameAppText.hpp"

#include <cmath>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PickupSystem】构造函数
// ============================================================================
PickupSystem::PickupSystem() {
}

// ============================================================================
// 【Initialize】初始化
// ============================================================================
void PickupSystem::Initialize(const PickupCallbacks& callbacks) {
    callbacks_ = callbacks;
}

// ============================================================================
// 【SpawnPickup】生成单个掉落物
// ============================================================================
void PickupSystem::SpawnPickup(CloudSeamanor::domain::Vec2f position, const std::string& item_id, int amount) {
    pickups_.emplace_back(position, item_id, amount);

    // 设置视觉效果
    auto& pickup = pickups_.back();
    auto& shape = pickup.Shape();
    shape.setFillColor(PickupColorFor(item_id));
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(sf::Color(72, 48, 24));
}

// ============================================================================
// 【SpawnPickups】批量生成掉落物
// ============================================================================
void PickupSystem::SpawnPickups(const std::vector<std::tuple<CloudSeamanor::domain::Vec2f, std::string, int> >& items) {
    for (const auto& [position, item_id, amount] : items) {
        SpawnPickup(position, item_id, amount);
    }
}

// ============================================================================
// 【Update】更新
// ============================================================================
void PickupSystem::Update(
    float delta_seconds,
    float world_tip_pulse,
    const CloudSeamanor::domain::Player& player,
    CloudSeamanor::domain::Inventory& inventory
) {
    (void)delta_seconds;
    for (auto it = pickups_.begin(); it != pickups_.end();) {
        auto& shape = it->Shape();
        const sf::Vector2f anchor = shape.getPosition();

        // 更新视觉效果（旋转和缩放动画）
        shape.setOrigin({0.0f, 0.0f});
        shape.setRotation(sf::degrees(std::sin(world_tip_pulse + anchor.x * 0.01f) * 4.0f));
        shape.setScale({
            1.0f + std::max(0.0f, std::sin(world_tip_pulse + anchor.x * 0.02f)) * 0.05f,
            1.0f + std::max(0.0f, std::sin(world_tip_pulse + anchor.x * 0.02f)) * 0.05f
        });

        // 检测拾取
        if (it->IsCollectedBy(player.Bounds())) {
            inventory.AddItem(it->ItemId(), it->Amount());

            std::string message = "已获得 " + ItemDisplayName(it->ItemId()) + " x" + std::to_string(it->Amount()) + "。";
            if (callbacks_.push_hint) {
                callbacks_.push_hint(message, 2.2f);
            }
            if (callbacks_.log_info) {
                callbacks_.log_info("已拾取掉落物：" + it->ItemId());
            }

            it = pickups_.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// 【Clear】清空
// ============================================================================
void PickupSystem::Clear() {
    pickups_.clear();
}

} // namespace CloudSeamanor::engine
