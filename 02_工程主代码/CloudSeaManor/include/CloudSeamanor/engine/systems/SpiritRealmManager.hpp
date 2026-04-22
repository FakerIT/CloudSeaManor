#pragma once

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/CloudSystem.hpp"

namespace CloudSeamanor::engine {

class SpiritRealmManager {
public:
    void RefreshDailyNodes(
        GameWorldState& world_state,
        const CloudSeamanor::domain::CloudSystem& cloud_system) const;
};

} // namespace CloudSeamanor::engine

