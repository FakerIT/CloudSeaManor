#pragma once

#include "CloudSeamanor/game_state/PlayerState.hpp"
#include "CloudSeamanor/game_state/RenderState.hpp"
#include "CloudSeamanor/game_state/RuntimeState.hpp"
#include "CloudSeamanor/game_state/WorldState.hpp"

namespace CloudSeamanor::engine {

struct GameAppState {
    game_state::WorldState world;
    game_state::PlayerState player;
    game_state::RenderState render;
    game_state::RuntimeState runtime;
};

} // namespace CloudSeamanor::engine
