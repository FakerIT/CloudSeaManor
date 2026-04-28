#pragma once

#include "CloudSeamanor/GameRuntime.hpp"
#include "CloudSeamanor/InputManager.hpp"
#include "CloudSeamanor/PixelGameHud.hpp"

namespace CloudSeamanor::engine {

class HudPresenter {
public:
    static void UpdateCoreViews(
        PixelGameHud& hud,
        GameRuntime& runtime,
        const InputManager& input,
        float delta_seconds);
};

}  // namespace CloudSeamanor::engine
