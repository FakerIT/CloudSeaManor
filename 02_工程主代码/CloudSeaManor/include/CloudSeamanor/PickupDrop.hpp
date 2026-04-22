#pragma once

// ============================================================================
// 【PickupDrop】场景可拾取物品系统
// ============================================================================
// 统一管理场景中可拾取物品的创建、显示和拾取判定。
//
// 主要职责：
// - 管理可拾取物品的位置、外观和数量
// - 提供玩家拾取检测
// - 支持动画和视觉反馈
//
// 与其他系统的关系：
// - 依赖：SFML（Graphics）
// - 被依赖：GameAppFarming（作物收获）、GameAppNpc（礼物掉落）、
//           GameApp（拾取处理）、GameAppHud（拾取提示）
//
// 设计说明：
// - 物品在场景中以可视化形式存在
// - 玩家接近时自动拾取
// - 支持后续扩展动画、飘字反馈等
// ============================================================================

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "CloudSeamanor/MathTypes.hpp"

#include <string>

namespace CloudSeamanor::domain {

// ============================================================================
// 【PickupDrop】可拾取物品对象
// ============================================================================
// 表示场景中一个可以被玩家拾取的物品实体。
//
// 设计决策：
// - 同时具有数据属性（物品ID、数量）和世界表现（位置、形状）
// - 使用AABB碰撞检测判断拾取
// - 暴露SFML图形对象用于渲染和动画
//
// 使用示例：
// @code
// PickupDrop drop({100, 200}, "green_tea", 5);
// if (drop.IsCollectedBy(player_bounds)) {
//     inventory.AddItem(drop.ItemId(), drop.Amount());
// }
// @endcode
class PickupDrop {
public:
    // ========================================================================
    // 【PickupDrop】构造函数
    // ========================================================================
    // @param position 物品出现位置
    // @param item_id 物品标识符
    // @param amount 物品数量
    PickupDrop(sf::Vector2f position, std::string item_id, int amount);

    // ========================================================================
    // 【IsCollectedBy】检测是否被玩家拾取
    // ========================================================================
    // @param player_bounds 玩家碰撞包围盒
    // @return true 如果玩家与物品发生碰撞
    [[nodiscard]] bool IsCollectedBy(const RectF& player_bounds) const noexcept;

    // ========================================================================
    // 【ItemId】获取物品标识符
    // ========================================================================
    [[nodiscard]] const std::string& ItemId() const noexcept { return item_id_; }

    // ========================================================================
    // 【Amount】获取物品数量
    // ========================================================================
    [[nodiscard]] int Amount() const noexcept { return amount_; }

    // ========================================================================
    // 【Shape】获取渲染用图形
    // ========================================================================
    // @return 可修改的图形引用，用于动画等效果
    [[nodiscard]] const sf::RectangleShape& Shape() const noexcept { return shape_; }
    [[nodiscard]] sf::RectangleShape& Shape() noexcept { return shape_; }

    // ========================================================================
    // 【SetPosition】设置物品位置
    // ========================================================================
    // @param position 新的位置
    void SetPosition(const sf::Vector2f& position) {
        shape_.setPosition(position);
    }

    // ========================================================================
    // 【GetPosition】获取物品位置
    // ========================================================================
    [[nodiscard]] sf::Vector2f GetPosition() const noexcept {
        return shape_.getPosition();
    }

    // ========================================================================
    // 【SetAmount】设置物品数量
    // ========================================================================
    void SetAmount(int amount) {
        if (amount > 0) {
            amount_ = amount;
        }
    }

private:
    // ========================================================================
    // 成员变量
    // ========================================================================
    sf::RectangleShape shape_;  // 渲染用图形
    std::string item_id_;       // 物品标识符
    int amount_ = 1;            // 物品数量
};

}  // namespace CloudSeamanor::domain
