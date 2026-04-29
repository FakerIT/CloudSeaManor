#include "CloudSeamanor/domain/TeaBushTable.hpp"

#include "CloudSeamanor/infrastructure/Logger.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

bool TeaBushTable::LoadFromFile(const std::string& file_path) {
    if (!bushes_.empty()) {
        return true;
    }
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("无法打开灵茶灌木数据表：" + file_path);
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
        TeaBushDefinition def;
        int col = 0;
        while (std::getline(ss, field, ',')) {
            switch (col) {
            case 0: def.id = field; break;
            case 1: def.name = field; break;
            case 2: def.tea_id = field; break;
            case 3: def.harvest_interval_days = std::stoi(field); break;
            case 4: def.unlock_condition = field; break;
            default: break;
            }
            ++col;
        }
        if (!def.id.empty()) {
            bushes_.push_back(def);
            index_[def.id] = bushes_.size() - 1;
        }
    }
    infrastructure::Logger::Info("已加载 " + std::to_string(bushes_.size()) + " 条灵茶灌木数据。");
    return !bushes_.empty();
}

const TeaBushDefinition* TeaBushTable::Get(const std::string& id) const {
    const auto it = index_.find(id);
    if (it == index_.end()) return nullptr;
    return &bushes_[it->second];
}

TeaBushTable& GetGlobalTeaBushTable() {
    static TeaBushTable table;
    return table;
}

}  // namespace CloudSeamanor::domain

