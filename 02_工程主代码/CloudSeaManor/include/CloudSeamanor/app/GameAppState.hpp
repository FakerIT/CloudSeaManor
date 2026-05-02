#pragma once

#include "CloudSeamanor/game_state/PlayerState.hpp"
#include "CloudSeamanor/game_state/RenderState.hpp"
#include "CloudSeamanor/engine/GameWorldState.hpp"

namespace CloudSeamanor::engine {

struct GameAppState {
    GameWorldState world;
    game_state::PlayerState player;
    game_state::RenderState render;
};

} // namespace CloudSeamanor::engine
