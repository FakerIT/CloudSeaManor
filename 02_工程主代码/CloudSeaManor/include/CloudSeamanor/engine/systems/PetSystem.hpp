#pragma once

#include "CloudSeamanor/engine/GameWorldState.hpp"

namespace CloudSeamanor::engine {

class PetSystem {
public:
    void Update(GameWorldState& world_state, float delta_seconds) const;
};

} // namespace CloudSeamanor::engine
