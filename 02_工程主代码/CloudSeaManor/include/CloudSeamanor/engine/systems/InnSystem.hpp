#pragma once

#include "CloudSeamanor/engine/GameWorldState.hpp"

namespace CloudSeamanor::engine {

class InnSystem {
public:
    void DailySettlement(GameWorldState& world_state, int house_level);
private:
    void RefreshOrderPool_(GameWorldState& world_state, int house_level) const;
};

} // namespace CloudSeamanor::engine
