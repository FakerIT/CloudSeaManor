#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/Interactable.hpp"
#include "CloudSeamanor/SfmlAdapter.hpp"

#include <SFML/Graphics/Color.hpp>

#include <utility>

namespace CloudSeamanor::domain {

// ============================================================================
// 【Interactable】构造函数
// ============================================================================
Interactable::Interactable(
    sf::Vector2f position,
    sf::Vector2f size,
    InteractableType type,
    std::string label,
    std::string reward_item,
    int reward_amount
) : type_(type),
    label_(std::move(label)),
    reward_item_(std::move(reward_item)),
    reward_amount_(reward_amount) {
    shape_.setPosition(position);
    shape_.setSize(size);
    RefreshVisualState(false);
}

// ============================================================================
// 【SetHighlighted】设置高亮状态
// ============================================================================
void Interactable::SetHighlighted(bool highlighted) {
    RefreshVisualState(highlighted);
}

// ============================================================================
// 【IsPlayerInRange】检测玩家是否在交互范围内
// ============================================================================
bool Interactable::IsPlayerInRange(
    const RectF& player_bounds,
    float extra_range
) const noexcept {
    // 扩展交互检测区域，提升交互手感
    RectF expanded = adapter::ToDomain(Bounds());
    expanded.position.x -= extra_range;
    expanded.position.y -= extra_range;
    expanded.size.x += extra_range * 2.0f;
    expanded.size.y += extra_range * 2.0f;

    return Intersection(expanded, player_bounds).has_value();
}

// ============================================================================
// 【Bounds】获取碰撞包围盒
// ============================================================================
sf::FloatRect Interactable::Bounds() const noexcept {
    return shape_.getGlobalBounds();
}

// ============================================================================
// 【TypeText】类型转可读文本
// ============================================================================
std::string Interactable::TypeText() const {
    switch (type_) {
    case InteractableType::GatheringNode: return "采集点";
    case InteractableType::Workstation:   return "工作台";
    case InteractableType::Storage:       return "储物点";
    }
    return "未知";
}

// ============================================================================
// 【RefreshVisualState】刷新视觉状态
// ============================================================================
void Interactable::RefreshVisualState(bool highlighted) {
    // 根据类型设置颜色
    sf::Color base_fill;
    sf::Color outline;

    switch (type_) {
    case InteractableType::GatheringNode:
        base_fill = sf::Color(98, 168, 88);
        outline = sf::Color(46, 96, 40);
        break;
    case InteractableType::Workstation:
        base_fill = sf::Color(176, 136, 72);
        outline = sf::Color(94, 68, 34);
        break;
    case InteractableType::Storage:
        base_fill = sf::Color(118, 128, 156);
        outline = sf::Color(64, 72, 88);
        break;
    }

    shape_.setFillColor(base_fill);
    shape_.setOutlineColor(outline);
    shape_.setOutlineThickness(2.0f);

    // 高亮时使用白色粗描边
    if (highlighted) {
        shape_.setOutlineColor(sf::Color::White);
        shape_.setOutlineThickness(4.0f);
    }
}

}  // namespace CloudSeamanor::domain
