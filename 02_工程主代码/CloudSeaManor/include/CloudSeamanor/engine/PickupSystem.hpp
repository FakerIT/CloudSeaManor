#pragma once

// ============================================================================
// 【PickupSystem】物品拾取系统
// ============================================================================
// 管理游戏中可拾取物品的生成、更新和收集。
//
// 主要职责：
// - 管理掉落物列表
// - 生成新的掉落物
// - 检测玩家拾取
// - 更新掉落物视觉效果
//
// 设计原则：
// - 独立的拾取状态管理
// - 支持多种掉落物类型
// - 提供简洁的交互接口
//
// 架构决策：
// - 公开接口使用 domain::Vec2f（纯数据），SFML 类型隔离在内部
// ============================================================================

#include "CloudSeamanor/engine/PickupDrop.hpp"
#include "CloudSeamanor/domain/Player.hpp"
#include "CloudSeamanor/domain/Inventory.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

// ============================================================================
// 【PickupCallbacks】物品拾取回调函数
// ============================================================================
struct PickupCallbacks {
    std::function<void(const std::string&, float)> push_hint;
    std::function<void(const std::string&)> log_info;
};

// ============================================================================
// 【PickupSystem】物品拾取系统类
// ============================================================================
class PickupSystem {
public:
    // ========================================================================
    // 【构造函数】
    // ========================================================================
    PickupSystem();

    // ========================================================================
    // 【初始化】
    // ========================================================================
    void Initialize(const PickupCallbacks& callbacks);

    // ========================================================================
    // 【访问器】
    // ========================================================================
    [[nodiscard]] std::vector<CloudSeamanor::domain::PickupDrop>& Pickups() { return pickups_; }
    [[nodiscard]] const std::vector<CloudSeamanor::domain::PickupDrop>& Pickups() const { return pickups_; }
    [[nodiscard]] int PickupCount() const { return static_cast<int>(pickups_.size()); }

    // ========================================================================
    // 【生成掉落物】
    // ========================================================================
    // @param position 掉落位置（domain 纯数据）
    // @param item_id 物品标识符
    // @param amount 物品数量
    void SpawnPickup(CloudSeamanor::domain::Vec2f position, const std::string& item_id, int amount);
    void SpawnPickups(const std::vector<std::tuple<CloudSeamanor::domain::Vec2f, std::string, int> >& items);

    // ========================================================================
    // 【更新】
    // ========================================================================
    void Update(float delta_seconds, float world_tip_pulse, const CloudSeamanor::domain::Player& player, CloudSeamanor::domain::Inventory& inventory);

    // ========================================================================
    // 【清空】
    // ========================================================================
    void Clear();

private:
    std::vector<CloudSeamanor::domain::PickupDrop> pickups_;
    PickupCallbacks callbacks_;
};

} // namespace CloudSeamanor::engine
