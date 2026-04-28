#include "CloudSeamanor/BuffEffectTable.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

bool BuffEffectTable::LoadFromFile(const std::string& file_path) {
    if (!buffs_.empty()) {
        return true;
    }
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("无法打开Buff效果数据表：" + file_path);
        return false;
    }
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string field;
        BuffEffectDefinition def;
        int col = 0;

        while (std::getline(ss, field, ',')) {
            switch (col) {
            case 0: def.id = field; break;
            case 1: def.name = field; break;
            case 2: def.duration_seconds = std::stoi(field); break;
            case 3: def.description = field; break;
            case 4: def.stamina_recovery_multiplier = std::stof(field); break;
            case 5: def.stamina_cost_multiplier = std::stof(field); break;
            case 6: def.work_efficiency_bonus = std::stof(field); break;
            case 7: def.trade_price_bonus = std::stof(field); break;
            case 8: def.favor_gain_bonus = std::stof(field); break;
            case 9: def.spirit_rate_bonus = std::stof(field); break;
            case 10: def.craft_success_bonus = std::stof(field); break;
            case 11: def.insight_trigger_bonus = std::stof(field); break;
            case 12: def.skill_mastery_bonus = std::stof(field); break;
            case 13: def.defense_bonus = std::stof(field); break;
            case 14: def.move_speed_bonus = std::stof(field); break;
            case 15: def.color_hex = field; break;
            default: break;
            }
            ++col;
        }

        if (!def.id.empty()) {
            buffs_.push_back(def);
            index_[def.id] = buffs_.size() - 1;
        }
    }

    infrastructure::Logger::Info("已加载 " + std::to_string(buffs_.size()) + " 条Buff效果数据。");
    return !buffs_.empty();
}

const BuffEffectDefinition* BuffEffectTable::Get(const std::string& id) const {
    const auto it = index_.find(id);
    if (it == index_.end()) return nullptr;
    return &buffs_[it->second];
}

bool BuffEffectTable::Exists(const std::string& id) const {
    return index_.find(id) != index_.end();
}

std::vector<const BuffEffectDefinition*> BuffEffectTable::GetByCategory(const std::string& category) const {
    std::vector<const BuffEffectDefinition*> result;

    for (const auto& buff : buffs_) {
        if (category == "stamina" &&
            (buff.stamina_recovery_multiplier != 1.0f || buff.stamina_cost_multiplier != 1.0f)) {
            result.push_back(&buff);
        } else if (category == "trade" && buff.trade_price_bonus != 0.0f) {
            result.push_back(&buff);
        } else if (category == "favor" && buff.favor_gain_bonus != 0.0f) {
            result.push_back(&buff);
        } else if (category == "spirit" && buff.spirit_rate_bonus != 0.0f) {
            result.push_back(&buff);
        } else if (category == "craft" && (buff.craft_success_bonus != 0.0f || buff.insight_trigger_bonus != 0.0f)) {
            result.push_back(&buff);
        } else if (category == "mastery" && buff.skill_mastery_bonus != 0.0f) {
            result.push_back(&buff);
        } else if (category == "defense" && buff.defense_bonus != 0.0f) {
            result.push_back(&buff);
        } else if (category == "speed" && buff.move_speed_bonus != 0.0f) {
            result.push_back(&buff);
        } else if (category == "work" && buff.work_efficiency_bonus != 0.0f) {
            result.push_back(&buff);
        }
    }

    return result;
}

BuffEffectTable& GetGlobalBuffEffectTable() {
    static BuffEffectTable table;
    return table;
}

} // namespace CloudSeamanor::domain
