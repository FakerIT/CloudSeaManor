#pragma once

// ============================================================================
// 【LevelUpSystem】技能升级系统
// ============================================================================
// 处理技能升级时的 UI 显示和动画效果。
//
// 主要职责：
// - 管理技能升级时的覆盖层显示
// - 处理升级动画效果
// - 提供升级事件通知
//
// 设计原则：
// - 独立的升级 UI 管理
// - 通过依赖注入访问所需系统
// - 返回升级结果供调用者使用
// ============================================================================

#include "CloudSeamanor/SkillSystem.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor {
namespace engine {

// ============================================================================
// 【LevelUpEvent】升级事件
// ============================================================================
struct LevelUpEvent {
    bool triggered = false;
    CloudSeamanor::domain::SkillType skill_type;
    int new_level;
    std::string skill_name;
    float bonus_value;
};

// ============================================================================
// 【LevelUpCallbacks】升级系统回调函数
// ============================================================================
struct LevelUpCallbacks {
    std::function<void(const std::string&, float)> push_hint;
    std::function<void(const std::string&)> log_info;
    std::function<void()> update_hud;
    std::function<void()> refresh_window_title;
};

// ============================================================================
// 【LevelUpSystem】技能升级系统类
// ============================================================================
class LevelUpSystem {
public:
    LevelUpSystem();

    void Initialize(const LevelUpCallbacks& callbacks);

    // 检查技能是否升级
    bool CheckSkillLevelUp(
        CloudSeamanor::domain::SkillSystem& skills,
        CloudSeamanor::domain::SkillType skill_type,
        float cloud_density,
        float extra_buff,
        float beast_share
    );

    // 更新升级覆盖层状态
    void Update(float delta_seconds);

    // 是否正在显示升级覆盖层
    [[nodiscard]] bool IsOverlayActive() const;

    // 获取升级文本
    [[nodiscard]] std::string GetLevelUpText() const;

    // 获取当前升级的技能类型
    [[nodiscard]] CloudSeamanor::domain::SkillType GetCurrentSkillType() const;

    // 获取升级覆盖层剩余时间
    [[nodiscard]] float GetOverlayTimer() const;

    // 获取待处理的升级事件
    [[nodiscard]] std::vector<LevelUpEvent> GetPendingEvents() const;

    // 清除待处理事件
    void ClearPendingEvents();

private:
    LevelUpCallbacks callbacks_;
    bool overlay_active_ = false;
    float overlay_timer_ = 0.0f;
    CloudSeamanor::domain::SkillType current_skill_type_ = CloudSeamanor::domain::SkillType::SpiritFarm;
    std::string current_level_up_text_;
    std::vector<LevelUpEvent> pending_events_;
};

LevelUpCallbacks CreateDefaultLevelUpCallbacks(
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info,
    const std::function<void()>& update_hud,
    const std::function<void()>& refresh_window_title
);

} // namespace engine
} // namespace CloudSeamanor
