#pragma once

// ============================================================================
// 【BattleDataLoader】battle data loader (infrastructure)
// ============================================================================
// Responsibilities:
// - Load battle JSON files via ResourceManager text channel.
// - Parse via infrastructure::JsonValue.
// - Build indexed registries for engine/runtime usage.
// ============================================================================

#include "CloudSeamanor/JsonValue.hpp"
#include "CloudSeamanor/ResourceManager.hpp"
#include "CloudSeamanor/domain/battle/BattleData.hpp"
#include "CloudSeamanor/Result.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::infrastructure::data {

struct BattleDataRegistry {
    CloudSeamanor::domain::battle::ElementRules element_rules{};
    CloudSeamanor::domain::battle::BattleTuning tuning{};

    std::unordered_map<std::string, CloudSeamanor::domain::battle::WeaponData> weapons_by_id{};
    std::unordered_map<std::string, CloudSeamanor::domain::battle::QuestSkillData> quest_skills_by_id{};
    std::unordered_map<std::string, CloudSeamanor::domain::battle::PetSkillData> pet_skills_by_id{};
    std::unordered_map<std::string, CloudSeamanor::domain::battle::SpiritPetData> pets_by_id{};
};

class BattleDataLoader {
public:
    BattleDataLoader() = default;

    CloudSeamanor::Result<BattleDataRegistry> LoadAll(
        CloudSeamanor::infrastructure::ResourceManager& rm,
        const std::string& battle_data_root = "assets/data/battle") const;

private:
    [[nodiscard]] CloudSeamanor::Result<CloudSeamanor::infrastructure::JsonValue> LoadJson_(
        CloudSeamanor::infrastructure::ResourceManager& rm,
        const std::string& text_id,
        const std::string& path) const;
};

}  // namespace CloudSeamanor::infrastructure::data

