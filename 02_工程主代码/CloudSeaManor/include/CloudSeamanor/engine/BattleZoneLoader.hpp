#pragma once

#include "CloudSeamanor/engine/BattleEntities.hpp"

#include <string>
#include <unordered_map>

namespace CloudSeamanor::engine {

class BattleZoneLoader {
public:
    BattleZoneLoader() = default;

    [[nodiscard]] bool LoadFromCsv(
        const std::string& file_path,
        std::unordered_map<std::string, BattleZone>& out_zone_table) const;
};

}  // namespace CloudSeamanor::engine

