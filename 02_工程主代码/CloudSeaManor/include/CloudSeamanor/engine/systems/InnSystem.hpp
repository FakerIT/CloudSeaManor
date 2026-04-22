#pragma once

#include "CloudSeamanor/GameWorldState.hpp"

namespace CloudSeamanor::engine {

class InnSystem {
public:
    void DailySettlement(GameWorldState& world_state, int house_level);
};

} // namespace CloudSeamanor::engine
