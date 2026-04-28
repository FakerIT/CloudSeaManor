#include "CloudSeamanor/engine/BattleZoneLoader.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <fstream>
#include <sstream>

namespace CloudSeamanor::engine {

namespace {

std::vector<std::string> SplitCsvLine_(const std::string& line) {
    std::vector<std::string> out;
    std::string token;
    std::istringstream iss(line);
    while (std::getline(iss, token, ',')) {
        out.push_back(token);
    }
    return out;
}

bool LooksLikeHeaderRow_(const std::vector<std::string>& cols) {
    if (cols.empty()) {
        return true;
    }
    return cols[0] == "id" || cols[0] == "name";
}

}  // namespace

bool BattleZoneLoader::LoadFromCsv(
    const std::string& file_path,
    std::unordered_map<std::string, BattleZone>& out_zone_table) const {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        CloudSeamanor::infrastructure::Logger::Warning("BattleZoneLoader: 无法打开区域表: " + file_path);
        return false;
    }

    out_zone_table.clear();
    std::string line;
    if (!std::getline(file, line)) {
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto cols = SplitCsvLine_(line);
        if (cols.size() < 4 || LooksLikeHeaderRow_(cols)) {
            continue;
        }

        BattleZone zone;
        zone.id = cols[0];
        zone.name = cols[1];
        zone.background_sprite_id = cols[2];
        try {
            zone.ambient_pollution_rate = std::stof(cols[3]);
        } catch (const std::exception&) {
            zone.ambient_pollution_rate = 1.0f;
        }
        zone.is_spirit_realm = zone.id.find("spirit") != std::string::npos;

        if (!zone.id.empty()) {
            out_zone_table[zone.id] = zone;
        }
    }

    CloudSeamanor::infrastructure::Logger::Info(
        "BattleZoneLoader: 区域表加载完成, 数量=" + std::to_string(out_zone_table.size()));
    return !out_zone_table.empty();
}

}  // namespace CloudSeamanor::engine

