#include "CloudSeamanor/engine/SpiritRealmDeepSystem.hpp"
#include "CloudSeamanor/infrastructure/DataRegistry.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::engine {

// ============================================================================
// 【LoadLayerConfigs】加载层级配置
// ============================================================================
void SpiritRealmDeepSystem::LoadLayerConfigs(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    std::vector<std::string> headers;
    bool first_line = true;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> cells;

        while (std::getline(ss, cell, ',')) {
            cells.push_back(cell);
        }

        if (first_line) {
            headers = cells;
            first_line = false;
            continue;
        }

        SpiritRealmLayerConfig config;
        for (std::size_t i = 0; i < headers.size() && i < cells.size(); ++i) {
            const auto& h = headers[i];
            const auto& v = cells[i];

            if (h == "Id") config.id = v;
            else if (h == "Name") config.name = v;
            else if (h == "MinLevel") config.min_level = std::stoi(v);
            else if (h == "RequiredPacts") config.required_pacts = std::stoi(v);
            else if (h == "AmbientPollutionRate") config.ambient_pollution_rate = std::stof(v);
            else if (h == "ToxicCloudDamage") config.toxic_cloud_damage = std::stof(v);
            else if (h == "DropMultiplier") config.drop_multiplier = std::stof(v);
            else if (h == "BgmId") config.bgm_id = v;
            else if (h == "BackgroundSprite") config.background_sprite = v;
            else if (h == "UnlockCondition") config.unlock_condition = v;
        }

        layer_configs_[std::stoi(config.id.substr(5))] = config;  // layer0 -> 0
    }
}

// ============================================================================
// 【LoadBossConfigs】加载首领配置
// ============================================================================
void SpiritRealmDeepSystem::LoadBossConfigs(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    std::vector<std::string> headers;
    bool first_line = true;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> cells;

        while (std::getline(ss, cell, ',')) {
            cells.push_back(cell);
        }

        if (first_line) {
            headers = cells;
            first_line = false;
            continue;
        }

        BossConfig config;
        for (std::size_t i = 0; i < headers.size() && i < cells.size(); ++i) {
            const auto& h = headers[i];
            const auto& v = cells[i];

            if (h == "Id") config.id = v;
            else if (h == "Name") config.name = v;
            else if (h == "Layer") config.layer = std::stoi(v);
            else if (h == "MinLevel") config.min_level = std::stoi(v);
            else if (h == "MaxPollution") config.max_pollution = std::stof(v);
            else if (h == "Health") config.health = std::stof(v);
            else if (h == "AttackDamage") config.attack_damage = std::stof(v);
            else if (h == "AttackInterval") config.attack_interval = std::stof(v);
            else if (h == "Speed") config.speed = std::stof(v);
            else if (h == "SizeRadius") config.size_radius = std::stof(v);
            else if (h == "ElementType") config.element_type = v;
            else if (h == "SpriteId") config.sprite_id = v;
            else if (h == "PhaseCount") config.phase_count = std::stoi(v);
            else if (h == "SkillIds") {
                std::stringstream ss2(v);
                std::string skill;
                while (std::getline(ss2, skill, ';')) {
                    if (!skill.empty()) config.skill_ids.push_back(skill);
                }
            }
            else if (h == "RewardExp") config.reward_exp = std::stof(v);
            else if (h == "RewardItems") {
                std::stringstream ss2(v);
                std::string item;
                while (std::getline(ss2, item, ';')) {
                    auto pos = item.find('×');
                    if (pos != std::string::npos) {
                        std::string item_id = item.substr(0, pos);
                        int count = std::stoi(item.substr(pos + 1));
                        config.reward_items.push_back({item_id, count});
                    }
                }
            }
            else if (h == "DialogueOnSpawn") config.dialogue_on_spawn = v;
        }

        bosses_.push_back(config);
    }
}

// ============================================================================
// 【GetLayerConfig】获取层级配置
// ============================================================================
const SpiritRealmLayerConfig* SpiritRealmDeepSystem::GetLayerConfig(int layer) const {
    auto it = layer_configs_.find(layer);
    if (it != layer_configs_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// 【CanEnterLayer】检查是否可进入
// ============================================================================
bool SpiritRealmDeepSystem::CanEnterLayer(int layer, int player_level, int completed_pacts) const {
    const auto* config = GetLayerConfig(layer);
    if (!config) return false;

    if (player_level < config->min_level) return false;
    if (completed_pacts < config->required_pacts) return false;

    return true;
}

// ============================================================================
// 【GetUnlockedLayers】获取已解锁的层级
// ============================================================================
std::vector<const SpiritRealmLayerConfig*> SpiritRealmDeepSystem::GetUnlockedLayers(
    int player_level, int completed_pacts) const {
    std::vector<const SpiritRealmLayerConfig*> result;
    for (const auto& [layer, config] : layer_configs_) {
        if (CanEnterLayer(layer, player_level, completed_pacts)) {
            result.push_back(&config);
        }
    }
    return result;
}

// ============================================================================
// 【GetToxicCloudDamage】获取毒云伤害
// ============================================================================
float SpiritRealmDeepSystem::GetToxicCloudDamage(int layer) const {
    const auto* config = GetLayerConfig(layer);
    if (config) {
        return config->toxic_cloud_damage;
    }
    return 0.0f;
}

// ============================================================================
// 【CalculateToxicDamage】计算毒云累积伤害
// ============================================================================
float SpiritRealmDeepSystem::CalculateToxicDamage(float damage_per_second, float duration_seconds) const {
    return damage_per_second * duration_seconds;
}

// ============================================================================
// 【GetBossForLayer】获取层级首领
// ============================================================================
const BossConfig* SpiritRealmDeepSystem::GetBossForLayer(int layer) const {
    for (const auto& boss : bosses_) {
        if (boss.layer == layer) {
            return &boss;
        }
    }
    return nullptr;
}

// ============================================================================
// 【IsBossDefeated】检查首领是否已击败
// ============================================================================
bool SpiritRealmDeepSystem::IsBossDefeated(const std::string& boss_id) const {
    auto it = defeated_bosses_.find(boss_id);
    return it != defeated_bosses_.end() && it->second;
}

// ============================================================================
// 【MarkBossDefeated】标记首领为已击败
// ============================================================================
void SpiritRealmDeepSystem::MarkBossDefeated(const std::string& boss_id) {
    defeated_bosses_[boss_id] = true;
}

// ============================================================================
// 【ResetBossStates】重置首领状态
// ============================================================================
void SpiritRealmDeepSystem::ResetBossStates() {
    defeated_bosses_.clear();
}

// ============================================================================
// 【GetDefeatedBossIds】获取已击败首领列表
// ============================================================================
std::vector<std::string> SpiritRealmDeepSystem::GetDefeatedBossIds() const {
    std::vector<std::string> result;
    for (const auto& [id, defeated] : defeated_bosses_) {
        if (defeated) {
            result.push_back(id);
        }
    }
    return result;
}

// ============================================================================
// 【LoadDefeatedBosses】从存档恢复首领状态
// ============================================================================
void SpiritRealmDeepSystem::LoadDefeatedBosses(const std::vector<std::string>& defeated_ids) {
    defeated_bosses_.clear();
    for (const auto& id : defeated_ids) {
        defeated_bosses_[id] = true;
    }
}

}  // namespace CloudSeamanor::engine
