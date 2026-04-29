#pragma once

// ============================================================================
// 【Interactable】场景可交互对象系统
// ============================================================================
// 统一管理场景中可交互对象的位置、类型、显示和交互判定。
//
// 主要职责：
// - 管理可交互对象的创建、显示、高亮
// - 提供玩家与对象的交互范围检测
// - 根据对象类型提供不同的视觉反馈
//
// 与其他系统的关系：
// - 依赖：SFML（Graphics）
// - 被依赖：GameAppScene（场景加载）、GameApp（交互处理）
// - 被依赖：GameAppHud（交互提示显示）
//
// 设计说明：
// - 使用AABB矩形碰撞检测
// - 高亮状态由外部控制，对象自身负责视觉刷新
// - 支持扩展检测范围（extra_range）提升交互手感
// ============================================================================

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "CloudSeamanor/MathTypes.hpp"

#include <string>

namespace CloudSeamanor::engine {

// ============================================================================
// 【InteractableType】可交互对象类型枚举
// ============================================================================
// 用于区分不同类型的交互对象，提供不同视觉样式和行为提示。
//
// 未来扩展方向：
// - 茶园采摘点
// - 工坊机器
// - 储物箱
// - NPC对话点
// - 传送门
enum class InteractableType {
    GatheringNode,  // 采集点（绿色）
    Workstation,   // 工作台（橙色）
    Storage,      // 储物点（灰色）
};

// ============================================================================
// 【Interactable】可交互对象领域对象
// ============================================================================
// 表示场景中一个可交互的实体对象。
//
// 设计决策：
// - 使用AABB矩形作为交互范围
// - 高亮状态由外部控制，对象内部处理视觉表现
// - 支持扩展检测范围提升交互手感
//
// 使用示例：
// @code
// Interactable node({100, 200}, {40, 40}, InteractableType::GatheringNode, "灵草", "herb", 2);
// if (node.IsPlayerInRange(player_bounds)) {
//     node.SetHighlighted(true);
// }
// @endcode
class Interactable {
public:
    // ========================================================================
    // 【Interactable】构造函数
    // ========================================================================
    // @param position 对象左上角世界坐标
    // @param size 对象尺寸
    // @param type 对象类型
    // @param label 显示标签
    // @param reward_item 奖励物品ID（可选）
    // @param reward_amount 奖励物品数量（可选）
    Interactable(
        sf::Vector2f position,
        sf::Vector2f size,
        InteractableType type,
        std::string label,
        std::string reward_item = "",
        int reward_amount = 1,
        std::string enemy_id = ""  // BOSS 刷新区专用：指定敌人ID
    );

    // ========================================================================
    // 【SetHighlighted】设置高亮状态
    // ========================================================================
    // @param highlighted 是否高亮
    // @note 高亮时使用白色粗描边，增强交互提示
    void SetHighlighted(bool highlighted);

    // ========================================================================
    // 【IsPlayerInRange】检测玩家是否在交互范围内
    // ========================================================================
    // @param player_bounds 玩家碰撞包围盒
    // @param extra_range 额外检测范围（默认18像素）
    // @return true 如果玩家进入扩展后的交互区域
    [[nodiscard]] bool IsPlayerInRange(
        const CloudSeamanor::domain::RectF& player_bounds,
        float extra_range = 18.0f
    ) const noexcept;

    // ========================================================================
    // 【Bounds】获取碰撞包围盒
    // ========================================================================
    [[nodiscard]] sf::FloatRect Bounds() const noexcept;

    // ========================================================================
    // 【Shape】获取渲染用图形
    // ========================================================================
    [[nodiscard]] const sf::RectangleShape& Shape() const noexcept { return shape_; }
    [[nodiscard]] sf::RectangleShape& Shape() noexcept { return shape_; }

    // ========================================================================
    // 【访问器】
    // ========================================================================
    [[nodiscard]] const std::string& Label() const noexcept { return label_; }
    [[nodiscard]] const std::string& RewardItem() const noexcept { return reward_item_; }
    [[nodiscard]] int RewardAmount() const noexcept { return reward_amount_; }
    [[nodiscard]] InteractableType Type() const noexcept { return type_; }
    [[nodiscard]] const std::string& EnemyId() const noexcept { return enemy_id_; }

    // ========================================================================
    // 【TypeText】类型转可读文本
    // ========================================================================
    [[nodiscard]] std::string TypeText() const;

private:
    // ========================================================================
    // 【RefreshVisualState】刷新视觉状态（内部）
    // ========================================================================
    // @param highlighted 是否高亮
    // @note 根据类型设置颜色和描边，高亮时使用白色粗描边
    void RefreshVisualState(bool highlighted);

    // ========================================================================
    // 成员变量
    // ========================================================================
    sf::RectangleShape shape_;                  // 渲染用图形
    InteractableType type_ = InteractableType::GatheringNode;  // 对象类型
    std::string label_;                       // 显示标签
    std::string reward_item_;                  // 奖励物品ID
    int reward_amount_ = 1;                   // 奖励物品数量
    std::string enemy_id_;                    // BOSS刷新区专用敌人ID
};

}  // namespace CloudSeamanor::engine

namespace CloudSeamanor::domain {
using InteractableType = CloudSeamanor::engine::InteractableType;
using Interactable = CloudSeamanor::engine::Interactable;
}  // namespace CloudSeamanor::domain
