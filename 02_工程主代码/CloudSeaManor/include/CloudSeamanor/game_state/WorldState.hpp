#pragma once

#include "CloudSeamanor/CloudGuardianContract.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/DynamicLifeSystem.hpp"
#include "CloudSeamanor/FestivalSystem.hpp"
#include "CloudSeamanor/GameClock.hpp"

namespace CloudSeamanor::game_state {

struct WorldState {
    CloudSeamanor::domain::GameClock clock;
    CloudSeamanor::domain::CloudSystem cloud_system;
    CloudSeamanor::domain::CloudGuardianContract contract;
    CloudSeamanor::domain::FestivalSystem festivals;
    CloudSeamanor::domain::DynamicLifeSystem dynamic_life;
    CloudSeamanor::domain::CloudState last_cloud_state =
        CloudSeamanor::domain::CloudState::Mist;
};

}  // namespace CloudSeamanor::game_state
