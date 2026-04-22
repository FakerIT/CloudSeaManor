#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/Player.hpp"

#include <algorithm>
#include <sstream>

namespace CloudSeamanor::domain {

// ============================================================================
// 【FacingFromVector】方向向量转朝向枚举
// ============================================================================
// 根据输入方向向量推导玩家朝向。
//
// 判定优先级：斜向 > 纯方向
// 零向量时返回默认朝向（Down）。
FacingDirection FacingFromVector(const Vec2f& direction) {
    if (direction.x == 0.0f && direction.y == 0.0f) {
        return FacingDirection::Down;
    }

    const bool left = direction.x < 0.0f;
    const bool right = direction.x > 0.0f;
    const bool up = direction.y < 0.0f;
    const bool down = direction.y > 0.0f;

    if (up && right) return FacingDirection::UpRight;
    if (up && left) return FacingDirection::UpLeft;
    if (down && right) return FacingDirection::DownRight;
    if (down && left) return FacingDirection::DownLeft;
    if (up) return FacingDirection::Up;
    if (down) return FacingDirection::Down;
    if (left) return FacingDirection::Left;
    return FacingDirection::Right;
}

// ============================================================================
// 【Player】构造函数
// ============================================================================
Player::Player() {
    // default position/size already set in members
}

// ============================================================================
// 【Move】移动玩家（带碰撞检测）
// ============================================================================
void Player::Move(
    const Vec2f& delta,
    const RectF& bounds,
    const std::vector<RectF>& obstacles
) {
    // 分轴处理移动，简化碰撞回退逻辑
    MoveAxis(delta.x, 0.0f, obstacles);
    MoveAxis(0.0f, delta.y, obstacles);

    // 限制在世界边界内
    Vec2f next_position = position_;
    next_position.x = std::clamp(next_position.x,
                                 bounds.position.x,
                                 bounds.position.x + bounds.size.x - size_.x);
    next_position.y = std::clamp(next_position.y,
                                 bounds.position.y,
                                 bounds.position.y + bounds.size.y - size_.y);
    position_ = next_position;
}

// ============================================================================
// 【SetMovementState】设置移动状态和朝向
// ============================================================================
void Player::SetMovementState(const Vec2f& direction, bool is_moving) {
    is_moving_ = is_moving;

    // 只有移动时才更新朝向
    if (is_moving) {
        facing_ = FacingFromVector(direction);
    }
}

// ============================================================================
// 【SetPosition】设置玩家位置
// ============================================================================
void Player::SetPosition(const Vec2f& position) {
    position_ = position;
}

// ============================================================================
// 【Bounds】获取碰撞包围盒
// ============================================================================
RectF Player::Bounds() const noexcept {
    return RectF{position_, size_};
}

// ============================================================================
// 【FacingText】朝向转可读文本
// ============================================================================
std::string Player::FacingText() const {
    switch (facing_) {
    case FacingDirection::Down: return "下";
    case FacingDirection::DownLeft: return "左下";
    case FacingDirection::Left: return "左";
    case FacingDirection::UpLeft: return "左上";
    case FacingDirection::Up: return "上";
    case FacingDirection::UpRight: return "右上";
    case FacingDirection::Right: return "右";
    case FacingDirection::DownRight: return "右下";
    }
    return "未知";
}

// ============================================================================
// 【SavePosition】保存位置用于存档
// ============================================================================
std::string Player::SavePosition() const {
    std::ostringstream oss;
    oss << position_.x << "," << position_.y;
    return oss.str();
}

// ============================================================================
// 【LoadPosition】从存档恢复位置
// ============================================================================
void Player::LoadPosition(float x, float y) {
    position_ = Vec2f{x, y};
}

// ============================================================================
// 【MoveAxis】单轴移动（内部）
// ============================================================================
void Player::MoveAxis(float dx, float dy, const std::vector<RectF>& obstacles) {
    // 无位移时跳过
    if (dx == 0.0f && dy == 0.0f) {
        return;
    }

    // 先尝试移动
    position_.x += dx;
    position_.y += dy;

    // 碰撞检测
    if (!CollidesWithAny(obstacles)) {
        return;
    }

    // 碰撞则回退
    position_.x -= dx;
    position_.y -= dy;
}

// ============================================================================
// 【CollidesWithAny】检测碰撞（内部）
// ============================================================================
bool Player::CollidesWithAny(const std::vector<RectF>& obstacles) const noexcept {
    const RectF player_bounds = Bounds();

    for (const auto& obstacle : obstacles) {
        if (Intersection(player_bounds, obstacle).has_value()) {
            return true;
        }
    }
    return false;
}

}  // namespace CloudSeamanor::domain
