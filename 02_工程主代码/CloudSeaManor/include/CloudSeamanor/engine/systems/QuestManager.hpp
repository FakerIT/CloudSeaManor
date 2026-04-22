#pragma once

#include "CloudSeamanor/GameWorldState.hpp"
#include "CloudSeamanor/Inventory.hpp"
#include "CloudSeamanor/Stamina.hpp"

#include <functional>
#include <string>
#include <vector>

namespace CloudSeamanor::engine {

class QuestManager {
public:
    bool LoadFromCsv(const std::string& path, std::vector<RuntimeQuest>& out_quests) const;
    void RefreshByTimeslot(std::vector<RuntimeQuest>& quests, int current_hour) const;
    void EvaluateProgress(
        std::vector<RuntimeQuest>& quests,
        CloudSeamanor::domain::Inventory& inventory,
        int spirit_farm_level,
        int& gold,
        CloudSeamanor::domain::StaminaSystem* stamina,
        const std::function<void(const std::string&, int)>& grant_item,
        const std::function<void(int)>& grant_favor,
        const std::function<void(const std::string&)>& on_completed) const;
};

} // namespace CloudSeamanor::engine

