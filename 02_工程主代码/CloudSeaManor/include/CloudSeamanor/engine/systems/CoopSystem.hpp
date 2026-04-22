#pragma once

#include "CloudSeamanor/GameWorldState.hpp"

namespace CloudSeamanor::engine {

class CoopSystem {
public:
    void DailyUpdate(GameWorldState& world_state);
};

class BarnSystem {
public:
    void DailyUpdate(GameWorldState& world_state);
};

} // namespace CloudSeamanor::engine
