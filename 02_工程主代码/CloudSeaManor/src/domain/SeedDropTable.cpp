// ============================================================================
// 【SeedDropTable.cpp】种子掉落表实现
// ============================================================================

#include "CloudSeamanor/SeedDropTable.hpp"
#include "CloudSeamanor/Logger.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

bool SeedDropTable::LoadFromCsv(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        infrastructure::Logger::Warning("SeedDropTable: Failed to open " + file_path);
        return false;
    }

    entries_.clear();
    std::string line;

    // Skip header
    if (!std::getline(file, line)) {
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream ss(line);
        std::string field;
        SeedDropEntry entry;

        int col = 0;
        while (std::getline(ss, field, ',')) {
            switch (col) {
            case 0: entry.crop_id = field; break;
            case 1: entry.seed_item_id = field; break;
            case 2: entry.source = field; break;
            case 3: entry.drop_rate = std::stof(field); break;
            case 4: entry.min_star = std::stoi(field); break;
            case 5: entry.notes = field; break;
            }
            ++col;
        }

        if (!entry.seed_item_id.empty()) {
            entries_.push_back(entry);
        }
    }

    loaded_ = true;
    infrastructure::Logger::Info("SeedDropTable: Loaded " + std::to_string(entries_.size()) + " entries from " + file_path);
    return true;
}

std::vector<const SeedDropEntry*> SeedDropTable::GetBattleDrops(int enemy_star) const {
    std::vector<const SeedDropEntry*> drops;
    for (const auto& entry : entries_) {
        if (entry.source == "battle" && enemy_star >= entry.min_star) {
            drops.push_back(&entry);
        }
    }
    return drops;
}

std::vector<const SeedDropEntry*> SeedDropTable::GetSpiritPlantDrops() const {
    std::vector<const SeedDropEntry*> drops;
    for (const auto& entry : entries_) {
        if (entry.source == "spirit_plant") {
            drops.push_back(&entry);
        }
    }
    return drops;
}

const SeedDropEntry* SeedDropTable::GetWildDrop() const {
    for (const auto& entry : entries_) {
        if (entry.source == "wild") {
            return &entry;
        }
    }
    return nullptr;
}

}  // namespace CloudSeamanor::domain
