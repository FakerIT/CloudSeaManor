#pragma once

#include "CloudSeamanor/GameRuntime.hpp"
#include "CloudSeamanor/PixelGameHud.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace CloudSeamanor::engine {

class HudSideEffects {
public:
    enum class NotificationLevel : std::uint8_t {
        Info = 0,
        Important = 1,
    };

    struct HudEffectContext {
        std::function<void(const std::string&, NotificationLevel)> notify;
        std::function<void(const std::string&)> play_sfx;
        std::function<float()> now_seconds;

        bool allow_auto_open_festival_panel = true;
        bool allow_auto_open_mail_panel = true;
        bool allow_mail_arrival_notify = true;
        bool allow_achievement_notify = true;
        bool allow_achievement_unlock_sfx = true;

        float sfx_throttle_seconds = 0.25f;
    };

    struct State {
        int last_festival_auto_popup_day = -1;
        int last_mail_arrived_count = -1;
        int last_unlocked_achievement_count = -1;
        std::unordered_map<std::string, float> sfx_last_played_at_seconds{};
    };

    static void ApplyAll(PixelGameHud& hud, GameRuntime& runtime, State& state, const HudEffectContext& context);

private:
    static void ApplyFestivalAutoPopup(PixelGameHud& hud, GameRuntime& runtime, State& state);
    static void ApplyMailArrivalHint(
        PixelGameHud& hud,
        GameRuntime& runtime,
        State& state,
        const HudEffectContext& context);
    static void ApplyAchievementUnlockHint(
        GameRuntime& runtime,
        State& state,
        const HudEffectContext& context);
    static void PlaySfxThrottled_(
        State& state,
        const HudEffectContext& context,
        const std::string& sfx_id);
};

}  // namespace CloudSeamanor::engine
