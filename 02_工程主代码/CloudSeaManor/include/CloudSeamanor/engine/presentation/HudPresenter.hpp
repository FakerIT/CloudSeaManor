#pragma once

#include "CloudSeamanor/engine/GameRuntime.hpp"
#include "CloudSeamanor/engine/InputManager.hpp"
#include "CloudSeamanor/engine/PixelGameHud.hpp"

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
