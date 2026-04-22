#include "CloudSeamanor/AllDefine.hpp"

#include "CloudSeamanor/LevelUpSystem.hpp"

namespace CloudSeamanor {
namespace engine {

LevelUpSystem::LevelUpSystem() {
}

void LevelUpSystem::Initialize(const LevelUpCallbacks& callbacks) {
    callbacks_ = callbacks;
}

bool LevelUpSystem::CheckSkillLevelUp(
    CloudSeamanor::domain::SkillSystem& skills,
    CloudSeamanor::domain::SkillType skill_type,
    float cloud_density,
    float extra_buff,
    float beast_share
) {
    bool leveled_up = skills.AddExp(skill_type, 0.0f, cloud_density, extra_buff, beast_share);

    if (leveled_up) {
        const int new_level = skills.GetLevel(skill_type);
        const std::string skill_name = skills.GetSkillName(skill_type);

        overlay_active_ = true;
        overlay_timer_ = 2.5f;
        current_skill_type_ = skill_type;
        current_level_up_text_ = skill_name + " Lv." + std::to_string(new_level);

        LevelUpEvent event;
        event.skill_type = skill_type;
        event.new_level = new_level;
        event.skill_name = skill_name;
        event.bonus_value = skills.GetBonus(skill_type);
        pending_events_.push_back(event);

        if (callbacks_.push_hint) {
            callbacks_.push_hint("Level Up! " + skill_name + " Lv." + std::to_string(new_level), 3.2f);
        }
        if (callbacks_.log_info) {
            callbacks_.log_info("Skill Level Up: " + skill_name + " Lv." + std::to_string(new_level));
        }
        if (callbacks_.update_hud) {
            callbacks_.update_hud();
        }
        if (callbacks_.refresh_window_title) {
            callbacks_.refresh_window_title();
        }
    }

    return leveled_up;
}

void LevelUpSystem::Update(float delta_seconds) {
    if (overlay_active_) {
        overlay_timer_ -= delta_seconds;
        if (overlay_timer_ <= 0.0f) {
            overlay_active_ = false;
            overlay_timer_ = 0.0f;
        }
    }
}

bool LevelUpSystem::IsOverlayActive() const {
    return overlay_active_;
}

std::string LevelUpSystem::GetLevelUpText() const {
    return current_level_up_text_;
}

CloudSeamanor::domain::SkillType LevelUpSystem::GetCurrentSkillType() const {
    return current_skill_type_;
}

float LevelUpSystem::GetOverlayTimer() const {
    return overlay_timer_;
}

std::vector<LevelUpEvent> LevelUpSystem::GetPendingEvents() const {
    return pending_events_;
}

void LevelUpSystem::ClearPendingEvents() {
    pending_events_.clear();
}

LevelUpCallbacks CreateDefaultLevelUpCallbacks(
    const std::function<void(const std::string&, float)>& push_hint,
    const std::function<void(const std::string&)>& log_info,
    const std::function<void()>& update_hud,
    const std::function<void()>& refresh_window_title
) {
    LevelUpCallbacks callbacks;
    callbacks.push_hint = push_hint;
    callbacks.log_info = log_info;
    callbacks.update_hud = update_hud;
    callbacks.refresh_window_title = refresh_window_title;
    return callbacks;
}

} // namespace engine
} // namespace CloudSeamanor
