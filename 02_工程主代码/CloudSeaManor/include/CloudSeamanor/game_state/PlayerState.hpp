#pragma once

#include "CloudSeamanor/domain/Inventory.hpp"
#include "CloudSeamanor/domain/Player.hpp"
#include "CloudSeamanor/domain/SkillSystem.hpp"
#include "CloudSeamanor/domain/Stamina.hpp"
#include "CloudSeamanor/domain/WorkshopSystem.hpp"

namespace CloudSeamanor::game_state {

struct PlayerState {
    CloudSeamanor::domain::Player player;
    CloudSeamanor::domain::Inventory inventory;
    CloudSeamanor::domain::StaminaSystem stamina;
    CloudSeamanor::domain::SkillSystem skills;
    CloudSeamanor::domain::WorkshopSystem workshop;
};

}  // namespace CloudSeamanor::game_state
