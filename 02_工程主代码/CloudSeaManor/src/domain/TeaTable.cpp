#include "CloudSeamanor/domain/TeaTable.hpp"

#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

bool TeaTable::LoadFromFile(const std::string& file_path) {
    if (!teas_.empty()) {
        return true;
    }
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("无法打开灵茶数据表：" + file_path);
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
        TeaDefinition def;
        int col = 0;

        while (std::getline(ss, field, ',')) {
            switch (col) {
            case 0: def.id = field; break;
            case 1: def.name = field; break;
            case 2: def.tier = std::stoi(field); break;
            case 3: def.season = field; break;
            case 4: def.growth_days = std::stoi(field); break;
            case 5: def.base_yield = std::stoi(field); break;
            case 6: def.base_price = std::stoi(field); break;
            case 7: def.buff_effect_id = field; break;
            case 8: def.unlock_condition = field; break;
            case 9: {
                // 兼容处理 is_legendary 可能是 "0"/"1" 或 "true"/"false"
                def.is_legendary = (field == "1" || field == "true");
                break;
            }
            case 10: def.description = field; break;
            case 11: def.lore = field; break;
            case 12: def.color_hex = field; break;
            case 13: def.tea_type = field; break;
            case 14: def.fertilizer_multiplier = std::stof(field); break;
            case 15: def.cloud_min = field; break;
            case 16: def.harvest_time = field; break;
            default: break;
            }
            ++col;
        }

        // 解析季节标签
        def.season_tags = ParseSeasons_(def.season);
        // 根据层级添加标签
        def.tags.push_back("tier_" + std::to_string(def.tier));
        if (def.is_legendary) {
            def.tags.push_back("legendary");
        }
        // 根据茶类添加标签
        if (!def.tea_type.empty()) {
            def.tags.push_back(def.tea_type);
        }

        if (!def.id.empty()) {
            teas_.push_back(def);
            index_[def.id] = teas_.size() - 1;
        }
    }

    infrastructure::Logger::Info("已加载 " + std::to_string(teas_.size()) + " 条灵茶数据。");
    return !teas_.empty();
}

const TeaDefinition* TeaTable::Get(const std::string& id) const {
    const auto it = index_.find(id);
    if (it == index_.end()) return nullptr;
    return &teas_[it->second];
}

bool TeaTable::Exists(const std::string& id) const {
    return index_.find(id) != index_.end();
}

std::vector<const TeaDefinition*> TeaTable::GetBySeason(const std::string& season) const {
    std::vector<const TeaDefinition*> result;
    for (const auto& tea : teas_) {
        if (tea.season == "all" ||
            tea.season == season ||
            tea.season.find(season) != std::string::npos) {
            result.push_back(&tea);
        }
    }
    return result;
}

std::vector<const TeaDefinition*> TeaTable::GetByTier(int tier) const {
    std::vector<const TeaDefinition*> result;
    for (const auto& tea : teas_) {
        if (tea.tier == tier) {
            result.push_back(&tea);
        }
    }
    return result;
}

std::vector<const TeaDefinition*> TeaTable::GetByTeaType(const std::string& tea_type) const {
    std::vector<const TeaDefinition*> result;
    for (const auto& tea : teas_) {
        if (tea.tea_type == tea_type) {
            result.push_back(&tea);
        }
    }
    return result;
}

std::vector<const TeaDefinition*> TeaTable::GetByTag(const std::string& tag) const {
    std::vector<const TeaDefinition*> result;
    for (const auto& tea : teas_) {
        if (std::find(tea.tags.begin(), tea.tags.end(), tag) != tea.tags.end()) {
            result.push_back(&tea);
        }
    }
    return result;
}

std::vector<const TeaDefinition*> TeaTable::GetLegendaryTeas() const {
    std::vector<const TeaDefinition*> result;
    for (const auto& tea : teas_) {
        if (tea.is_legendary) {
            result.push_back(&tea);
        }
    }
    return result;
}

std::vector<const TeaDefinition*> TeaTable::GetAvailableTeas(
    const std::unordered_map<std::string, int>& favor_levels,
    const std::vector<std::string>& contract_completed,
    int main_chapter) const {
    std::vector<const TeaDefinition*> result;
    for (const auto& tea : teas_) {
        if (IsUnlocked(tea.unlock_condition, favor_levels, contract_completed, main_chapter)) {
            result.push_back(&tea);
        }
    }
    return result;
}

bool TeaTable::IsUnlocked(
    const std::string& condition,
    const std::unordered_map<std::string, int>& favor_levels,
    const std::vector<std::string>& contract_completed,
    int main_chapter) const {
    if (condition.empty() || condition == "default") {
        return true;
    }

    // favor_2 表示好感度达到2心
    if (condition.rfind("favor_", 0) == 0) {
        std::string npc_id = "default";
        int required_level = std::stoi(condition.substr(6));
        auto it = favor_levels.find(npc_id);
        return (it != favor_levels.end() && it->second >= required_level);
    }

    // contract_cloud_1 表示需要完成云守契约第一章
    if (condition.rfind("contract_", 0) == 0) {
        std::string contract_id = condition;
        return std::find(contract_completed.begin(), contract_completed.end(), contract_id) != contract_completed.end();
    }

    // main_ch2 表示需要完成主线第二章
    if (condition.rfind("main_ch", 0) == 0) {
        int required_chapter = std::stoi(condition.substr(7));
        return main_chapter >= required_chapter;
    }

    return true;
}

int TeaTable::CalculateSellPrice(const std::string& tea_id, int quality_tier) const {
    const auto* tea = Get(tea_id);
    if (!tea) return 0;

    // 品质倍率：普通1.0, 优质1.5, 稀有2.0, 灵品3.0, 圣品5.0
    static const float quality_multipliers[] = {1.0f, 1.5f, 2.0f, 3.0f, 5.0f};
    float quality_mult = (quality_tier >= 0 && quality_tier <= 4) ? quality_multipliers[quality_tier] : 1.0f;

    // 传说灵茶有额外加成
    float legendary_bonus = tea->is_legendary ? 1.5f : 1.0f;

    return static_cast<int>(tea->base_price * quality_mult * legendary_bonus);
}

int TeaTable::CalculateHarvestTime(const std::string& tea_id, const std::string& cloud_state) const {
    const auto* tea = Get(tea_id);
    if (!tea) return 0;

    // 云海加成：浓云海生长+30%，大潮生长+50%
    float cloud_bonus = 1.0f;
    if (cloud_state == "dense") {
        cloud_bonus = 0.7f;  // 70%的时间即可收获
    } else if (cloud_state == "tide") {
        cloud_bonus = 0.5f;
    }

    return static_cast<int>(tea->growth_days * cloud_bonus);
}

const char* TeaTable::TierToText(int tier) {
    switch (tier) {
        case 1: return "入门";
        case 2: return "中级";
        case 3: return "高级";
        case 4: return "传说";
        default: return "未知";
    }
}

const char* TeaTable::TeaTypeToText(const std::string& tea_type) {
    if (tea_type == "green") return "绿茶";
    if (tea_type == "white") return "白茶";
    if (tea_type == "oolong") return "乌龙";
    if (tea_type == "black") return "黑茶";
    if (tea_type == "red") return "红茶";
    if (tea_type == "purple") return "紫茶";
    if (tea_type == "gold") return "金茶";
    if (tea_type.empty()) return "混合";
    return "未知";
}

const char* TeaTable::SeasonToText(const std::string& season) {
    if (season == "spring") return "春";
    if (season == "summer") return "夏";
    if (season == "autumn") return "秋";
    if (season == "winter") return "冬";
    if (season == "all") return "全季";
    return "未知";
}

const char* TeaTable::HarvestTimeToText(const std::string& harvest_time) {
    if (harvest_time == "dawn") return "清晨";
    if (harvest_time == "day") return "白天";
    if (harvest_time == "dusk") return "黄昏";
    if (harvest_time == "night") return "夜间";
    if (harvest_time == "any") return "任意";
    return "未知";
}

std::vector<std::string> TeaTable::ParseSeasons_(const std::string& raw) {
    std::vector<std::string> seasons;
    std::istringstream ss(raw);
    std::string season;
    while (std::getline(ss, season, ';')) {
        if (!season.empty()) {
            seasons.push_back(season);
        }
    }
    if (seasons.empty()) {
        seasons.push_back(raw);
    }
    return seasons;
}

std::vector<std::string> TeaTable::ParseTags_(const std::string& raw) {
    std::vector<std::string> tags;
    std::istringstream ss(raw);
    std::string tag;
    while (std::getline(ss, tag, ';')) {
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }
    return tags;
}

TeaTable& GetGlobalTeaTable() {
    static TeaTable table;
    return table;
}

}  // namespace CloudSeamanor::domain
