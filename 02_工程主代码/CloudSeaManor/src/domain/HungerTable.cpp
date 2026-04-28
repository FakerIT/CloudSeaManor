#include "CloudSeamanor/HungerTable.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

bool HungerTable::LoadFromFile(const std::string& file_path) {
    if (!entries_.empty()) {
        return true;
    }
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("无法打开饱食数据表：" + file_path);
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
        HungerDefinition def;
        int col = 0;
        while (std::getline(ss, field, ',')) {
            switch (col) {
            case 0: def.item_id = field; break;
            case 1: def.item_name = field; break;
            case 2: def.hunger_restore = std::stoi(field); break;
            case 3: def.quality_multiplier = std::stof(field); break;
            case 4: def.source_tag = field; break;
            default: break;
            }
            ++col;
        }
        if (!def.item_id.empty()) {
            entries_.push_back(def);
            index_[def.item_id] = entries_.size() - 1;
        }
    }
    infrastructure::Logger::Info("已加载 " + std::to_string(entries_.size()) + " 条饱食数据。");
    return !entries_.empty();
}

const HungerDefinition* HungerTable::Get(const std::string& item_id) const {
    const auto it = index_.find(item_id);
    if (it == index_.end()) return nullptr;
    return &entries_[it->second];
}

HungerTable& GetGlobalHungerTable() {
    static HungerTable table;
    return table;
}

}  // namespace CloudSeamanor::domain

