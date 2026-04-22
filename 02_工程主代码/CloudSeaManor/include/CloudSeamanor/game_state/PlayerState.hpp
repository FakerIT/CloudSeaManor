#pragma once

#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/Player.hpp"
#include "CloudSeamanor/SkillSystem.hpp"
#include "CloudSeamanor/Stamina.hpp"
#include "CloudSeamanor/WorkshopSystem.hpp"

namespace CloudSeamanor::game_state {

struct PlayerState {
    CloudSeamanor::domain::Player player;
    CloudSeamanor::domain::Inventory inventory;
    CloudSeamanor::domain::StaminaSystem stamina;
    CloudSeamanor::domain::SkillSystem skills;
    CloudSeamanor::domain::WorkshopSystem workshop;
};

}  // namespace CloudSeamanor::game_state
