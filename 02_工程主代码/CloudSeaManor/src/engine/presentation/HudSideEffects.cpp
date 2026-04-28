#include "CloudSeamanor/engine/presentation/HudSideEffects.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

void HudSideEffects::ApplyAll(PixelGameHud& hud, GameRuntime& runtime, State& state, const HudEffectContext& context) {
    if (context.allow_auto_open_festival_panel) {
        ApplyFestivalAutoPopup(hud, runtime, state);
    }
    ApplyMailArrivalHint(hud, runtime, state, context);
    ApplyAchievementUnlockHint(runtime, state, context);
}

void HudSideEffects::ApplyFestivalAutoPopup(PixelGameHud& hud, GameRuntime& runtime, State& state) {
    const auto& world_state = runtime.WorldState();
    const int current_day = world_state.GetClock().Day();
    if (const auto* today_festival = runtime.Systems().GetFestivals().GetTodayFestival()) {
        if (current_day != state.last_festival_auto_popup_day) {
            if (!hud.IsDialogueOpen() && !hud.IsAnyPanelOpen()) {
                hud.ToggleFestival();
                infrastructure::Logger::Info("Festival panel auto-opened: " + today_festival->id);
                state.last_festival_auto_popup_day = current_day;
            }
        }
    }
}

void HudSideEffects::ApplyMailArrivalHint(
    PixelGameHud& hud,
    GameRuntime& runtime,
    State& state,
    const HudEffectContext& context) {
    const auto& world_state = runtime.WorldState();
    const int current_day = world_state.GetClock().Day();
    int arrived_count = 0;
    for (const auto& order : world_state.GetMailOrders()) {
        if (order.deliver_day <= current_day) {
            ++arrived_count;
        }
    }

    if (state.last_mail_arrived_count < 0) {
        state.last_mail_arrived_count = arrived_count;
        return;
    }
    if (arrived_count > state.last_mail_arrived_count) {
        if (context.allow_mail_arrival_notify && context.notify) {
            const std::string message = runtime.Config().GetString(
                "hud_mail_arrived_notice_prefix", "新邮件已送达 x")
                + std::to_string(arrived_count)
                + runtime.Config().GetString("hud_mail_arrived_notice_suffix", "（可在邮件面板领取）");
            context.notify(message, NotificationLevel::Important);
        }
        if (context.allow_auto_open_mail_panel && !hud.IsDialogueOpen() && !hud.IsAnyPanelOpen()) {
            hud.ToggleMail();
        }
    }
    state.last_mail_arrived_count = arrived_count;
}

void HudSideEffects::ApplyAchievementUnlockHint(
    GameRuntime& runtime,
    State& state,
    const HudEffectContext& context) {
    const auto& achievements = runtime.WorldState().GetAchievements();
    int unlocked_count = 0;
    for (const auto& [_, unlocked] : achievements) {
        if (unlocked) {
            ++unlocked_count;
        }
    }

    if (state.last_unlocked_achievement_count < 0) {
        state.last_unlocked_achievement_count = unlocked_count;
        return;
    }
    if (unlocked_count > state.last_unlocked_achievement_count) {
        const int new_count = std::max(0, unlocked_count - state.last_unlocked_achievement_count);
        if (context.allow_achievement_notify && context.notify) {
            const std::string message = runtime.Config().GetString(
                "hud_achievement_unlock_notice_prefix", "新成就达成 x")
                + std::to_string(new_count)
                + runtime.Config().GetString("hud_achievement_unlock_notice_suffix", "（可在成就面板查看）");
            context.notify(message, NotificationLevel::Important);
        }
        if (context.allow_achievement_unlock_sfx) {
            PlaySfxThrottled_(state, context, "achievement_unlock");
        }
    }
    state.last_unlocked_achievement_count = unlocked_count;
}

void HudSideEffects::PlaySfxThrottled_(
    State& state,
    const HudEffectContext& context,
    const std::string& sfx_id) {
    if (!context.play_sfx) {
        return;
    }
    if (!context.now_seconds || context.sfx_throttle_seconds <= 0.0f) {
        context.play_sfx(sfx_id);
        return;
    }
    const float now = context.now_seconds();
    const auto it = state.sfx_last_played_at_seconds.find(sfx_id);
    if (it != state.sfx_last_played_at_seconds.end()) {
        const float elapsed = now - it->second;
        if (elapsed < context.sfx_throttle_seconds) {
            return;
        }
    }
    state.sfx_last_played_at_seconds[sfx_id] = now;
    context.play_sfx(sfx_id);
}

}  // namespace CloudSeamanor::engine
