#pragma once

// ============================================================================
// 【SceneVisualFactory】场景视觉工厂
// ============================================================================
// 将 domain 层纯数据对象转换为 engine 层 SFML 渲染对象的工厂函数。
// 这是 domain 与 engine 之间的唯一渲染适配层（Adapter）。
//
// 设计原则：
// - domain 层完全无 SFML 依赖
// - 所有 SFML 对象在 engine 层创建和管理
// - 工厂函数接受 domain 数据 + 渲染参数，返回可渲染的 SFML 对象
//
// 使用示例：
// @code
// // 从 domain 数据创建渲染对象
// auto shape = SceneVisualFactory::CreatePickupShape(drop, {22, 22},
//     sf::Color(255, 218, 110), sf::Color(128, 88, 24));
//
// // 刷新已有对象的视觉状态
// SceneVisualFactory::RefreshInteractableShape(shape, interactable.VisualState());
// @endcode
// ============================================================================

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>

#include "CloudSeamanor/PickupDrop.hpp"
#include "CloudSeamanor/Interactable.hpp"

namespace CloudSeamanor::engine::SceneVisualFactory {

// ============================================================================
// 【CreatePickupShape】从 PickupDrop 创建渲染用矩形
// ============================================================================
// @param drop   PickupDrop 纯数据对象
// @param size   矩形尺寸（默认 22x22）
// @param fill  填充颜色
// @param outline 描边颜色
// @return 配置好的 sf::RectangleShape
inline sf::RectangleShape CreatePickupShape(
    const domain::PickupDrop& drop,
    const sf::Vector2f& size,
    const sf::Color& fill,
    const sf::Color& outline) {
    sf::RectangleShape shape(size);
    shape.setPosition({drop.Position().x, drop.Position().y});
    shape.setFillColor(fill);
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(outline);
    return shape;
}

// ============================================================================
// 【CreatePickupShape】使用默认尺寸创建
// ============================================================================
inline sf::RectangleShape CreatePickupShape(
    const domain::PickupDrop& drop,
    const sf::Color& fill,
    const sf::Color& outline) {
    return CreatePickupShape(drop, {22.0f, 22.0f}, fill, outline);
}

// ============================================================================
// 【RefreshPickupShape】刷新已有 PickupDrop 渲染对象的视觉状态
// ============================================================================
// @param shape 已有 sf::RectangleShape
// @param drop  PickupDrop 纯数据（提供新位置）
// @param size  矩形尺寸
// @param fill  填充颜色
// @param outline 描边颜色
inline void RefreshPickupShape(
    sf::RectangleShape& shape,
    const domain::PickupDrop& drop,
    const sf::Vector2f& size,
    const sf::Color& fill,
    const sf::Color& outline) {
    shape.setSize(size);
    shape.setPosition({drop.Position().x, drop.Position().y});
    shape.setFillColor(fill);
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(outline);
}

// ============================================================================
// 【RefreshPickupShape】使用默认尺寸刷新
// ============================================================================
inline void RefreshPickupShape(
    sf::RectangleShape& shape,
    const domain::PickupDrop& drop,
    const sf::Color& fill,
    const sf::Color& outline) {
    RefreshPickupShape(shape, drop, {22.0f, 22.0f}, fill, outline);
}

// ============================================================================
// 【CreateInteractableShape】从 Interactable 创建渲染用矩形
// ============================================================================
// @param obj   Interactable 纯数据对象
// @param visual 视觉状态数据（RGB 颜色）
// @return 配置好的 sf::RectangleShape
inline sf::RectangleShape CreateInteractableShape(
    const domain::Interactable& obj,
    const domain::InteractableVisualState& visual) {
    sf::RectangleShape shape({obj.Size().x, obj.Size().y});
    shape.setPosition({obj.Position().x, obj.Position().y});
    shape.setFillColor({visual.fill_r, visual.fill_g, visual.fill_b});
    shape.setOutlineThickness(visual.outline_thickness);
    shape.setOutlineColor({visual.outline_r, visual.outline_g, visual.outline_b});
    return shape;
}

// ============================================================================
// 【RefreshInteractableShape】刷新已有 Interactable 渲染对象的视觉状态
// ============================================================================
// @param shape  已有 sf::RectangleShape
// @param obj   Interactable 纯数据（提供位置）
// @param visual 视觉状态数据
inline void RefreshInteractableShape(
    sf::RectangleShape& shape,
    const domain::Interactable& obj,
    const domain::InteractableVisualState& visual) {
    shape.setSize({obj.Size().x, obj.Size().y});
    shape.setPosition({obj.Position().x, obj.Position().y});
    shape.setFillColor({visual.fill_r, visual.fill_g, visual.fill_b});
    shape.setOutlineThickness(visual.outline_thickness);
    shape.setOutlineColor({visual.outline_r, visual.outline_g, visual.outline_b});
}

// ============================================================================
// 【UpdatePickupPosition】仅更新位置（用于浮动动画等）
// ============================================================================
inline void UpdatePickupPosition(sf::RectangleShape& shape, float x, float y) {
    shape.setPosition({x, y});
}

// ============================================================================
// 【AnimatePickup】更新拾取物动画（旋转 + 轻微缩放）
// ============================================================================
// @param shape  已有 sf::RectangleShape
// @param pickup 纯数据对象（提供位置）
// @param pulse  世界脉动时间（用于动画相位偏移）
// @param dt     上一帧时间增量（保留扩展用，此处未使用）
inline void AnimatePickup(
    sf::RectangleShape& shape,
    const domain::PickupDrop& pickup,
    float pulse,
    float /*dt*/
) {
    const auto pos = pickup.Position();
    const float anchor_x = pos.x;

    shape.setOrigin({0.0f, 0.0f});
    shape.setRotation(sf::degrees(std::sin(pulse + anchor_x * 0.01f) * 4.0f));
    shape.setScale({
        1.0f + std::max(0.0f, std::sin(pulse + anchor_x * 0.02f)) * 0.05f,
        1.0f + std::max(0.0f, std::sin(pulse + anchor_x * 0.02f)) * 0.05f
    });
}

}  // namespace CloudSeamanor::engine::SceneVisualFactory
