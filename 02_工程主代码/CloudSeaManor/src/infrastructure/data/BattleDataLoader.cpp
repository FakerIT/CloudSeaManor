#include "CloudSeamanor/infrastructure/data/BattleDataLoader.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <filesystem>

namespace CloudSeamanor::infrastructure::data {

namespace {

std::string JoinPath_(const std::string& root, const std::string& rel) {
    return (std::filesystem::path(root) / rel).generic_string();
}

}  // namespace

CloudSeamanor::Result<CloudSeamanor::infrastructure::JsonValue> BattleDataLoader::LoadJson_(
    CloudSeamanor::infrastructure::ResourceManager& rm,
    const std::string& text_id,
    const std::string& path) const {
    if (!rm.HasText(text_id)) {
        if (!rm.LoadText(text_id, path)) {
            return CloudSeamanor::Result<CloudSeamanor::infrastructure::JsonValue>(
                "BattleDataLoader: failed to load json text_id=" + text_id + " path=" + path);
        }
    }
    const std::string& text = rm.GetText(text_id);
    if (text.empty()) {
        return CloudSeamanor::Result<CloudSeamanor::infrastructure::JsonValue>(
            "BattleDataLoader: empty json text_id=" + text_id + " path=" + path);
    }
    const auto json = CloudSeamanor::infrastructure::JsonValue::Parse(text);
    if (!json.IsObject()) {
        return CloudSeamanor::Result<CloudSeamanor::infrastructure::JsonValue>(
            "BattleDataLoader: invalid json root (expect object) text_id=" + text_id);
    }
    return json;
}

CloudSeamanor::Result<BattleDataRegistry> BattleDataLoader::LoadAll(
    CloudSeamanor::infrastructure::ResourceManager& rm,
    const std::string& battle_data_root) const {
    BattleDataRegistry reg;

    // NOTE: This is an integration skeleton. We intentionally load + parse the files first.
    // Field-level mapping and validations will be added as the battle systems migrate to JSON truth.
    const auto balance = LoadJson_(rm, "battle_balance_json", JoinPath_(battle_data_root, "balance.json"));
    if (!balance.Ok()) {
        infrastructure::Logger::Warning(balance.Error());
    }
    const auto weapons = LoadJson_(rm, "battle_weapons_json", JoinPath_(battle_data_root, "weapons.json"));
    if (!weapons.Ok()) {
        infrastructure::Logger::Warning(weapons.Error());
    }
    const auto quest_skills = LoadJson_(rm, "battle_quest_skills_json", JoinPath_(battle_data_root, "quest_skills.json"));
    if (!quest_skills.Ok()) {
        infrastructure::Logger::Warning(quest_skills.Error());
    }
    const auto pet_skills = LoadJson_(rm, "battle_pet_skills_json", JoinPath_(battle_data_root, "pet_skills.json"));
    if (!pet_skills.Ok()) {
        infrastructure::Logger::Warning(pet_skills.Error());
    }

    // MVP fallback: we consider it OK if at least one of the json files exists.
    // The engine will keep using CSV until a full migration is done.
    if (!balance.Ok() && !weapons.Ok() && !quest_skills.Ok() && !pet_skills.Ok()) {
        return CloudSeamanor::Result<BattleDataRegistry>(
            "BattleDataLoader: no battle json found under root=" + battle_data_root);
    }
    return reg;
}

}  // namespace CloudSeamanor::infrastructure::data

