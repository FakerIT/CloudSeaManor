#include "CloudSeamanor/engine/systems/TutorialSystem.hpp"

#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/infrastructure/GameConstants.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【TutorialSystem::TutorialSystem】构造函数
// ============================================================================
TutorialSystem::TutorialSystem(
    GameWorldState& world_state,
    HintCallback hint_callback
)
    : world_state_(world_state)
    , hint_callback_(std::move(hint_callback))
{
}

// ============================================================================
// 【TutorialSystem::Reset】重置教程进度
// ============================================================================
void TutorialSystem::Reset() {
    world_state_.MutableTutorial().intro_move_hint_shown = false;
    world_state_.MutableTutorial().intro_interact_hint_shown = false;
    world_state_.MutableTutorial().intro_crop_hint_shown = false;
    world_state_.MutableTutorial().intro_save_hint_shown = false;
    world_state_.MutableTutorial().show_debug_overlay = true;
    world_state_.MutableTutorial().daily_cloud_report_day_shown = -1;
    world_state_.MutableTutorial().tutorial_bubble_completed_mask = 0;
    world_state_.MutableTutorial().tutorial_bubble_step = 1;
}

// ============================================================================
// 【TutorialSystem::Update】每帧检查教程提示
// ============================================================================
void TutorialSystem::Update(float delta_seconds) {
    (void)delta_seconds;
    auto& tutorial = world_state_.MutableTutorial();
    auto& interaction = world_state_.MutableInteraction();
    const float session_time = world_state_.GetSessionTime();

    if (!tutorial.intro_move_hint_shown && session_time > 2.5f) {
        tutorial.intro_move_hint_shown = true;
        if (hint_callback_) {
            hint_callback_(
                "使用 WASD 移动。明亮的描边表示这些对象可以交互；先熟悉山庄，不需要赶时间。",
                GameConstants::Ui::HintDuration::TutorialMove);
        }
    }

    if (!tutorial.intro_interact_hint_shown && interaction.highlighted_index >= 0) {
        tutorial.intro_interact_hint_shown = true;
        if (hint_callback_) {
            hint_callback_(
                "按 E 与高亮对象交互。采集会消耗体力。",
                GameConstants::Ui::HintDuration::TutorialInteract);
        }
    }

    if (!tutorial.intro_crop_hint_shown && interaction.highlighted_plot_index >= 0) {
        tutorial.intro_crop_hint_shown = true;
        if (hint_callback_) {
            hint_callback_(
                "种植流程：翻土 -> 播种 -> 浇水 -> 等待 -> 收获。"
                "今天做不完也没关系，明天还能继续。",
                GameConstants::Ui::HintDuration::TutorialCrop);
        }
    }

    if (!tutorial.intro_save_hint_shown && session_time > 18.0f) {
        tutorial.intro_save_hint_shown = true;
        if (hint_callback_) {
            hint_callback_(
                "提示：F6 保存，F9 读取；22:00 之后按 T 可以睡觉。"
                "山庄没有硬性期限，错过的内容之后还能再体验。",
                GameConstants::Ui::HintDuration::TutorialSave);
        }
    }
}

// ============================================================================
// 【TutorialSystem::ToggleDebugOverlay】切换调试面板
// ============================================================================
void TutorialSystem::ToggleDebugOverlay() {
    auto& tutorial = world_state_.MutableTutorial();
    tutorial.show_debug_overlay = !tutorial.show_debug_overlay;
    if (hint_callback_) {
        hint_callback_(
            tutorial.show_debug_overlay ? "调试面板已开启。" : "调试面板已隐藏。",
            2.0f);
    }
}

// ============================================================================
// 【TutorialSystem::IsDebugOverlayVisible】是否显示调试面板
// ============================================================================
bool TutorialSystem::IsDebugOverlayVisible() const {
    return world_state_.GetTutorial().show_debug_overlay;
}

}  // namespace CloudSeamanor::engine
