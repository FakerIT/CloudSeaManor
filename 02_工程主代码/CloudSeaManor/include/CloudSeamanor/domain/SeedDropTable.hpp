#pragma once

// ============================================================================
// 【SeedDropTable.hpp】种子掉落表
// ============================================================================
// 管理战斗和采集中的种子掉落概率。
//
// 主要职责：
// - 从 CSV 加载种子掉落表
// - 提供战斗掉落查询
// - 提供野外/采集掉落查询
// ============================================================================

#include <string>
#include <vector>
#include <unordered_map>

namespace CloudSeamanor::domain {

struct SeedDropEntry {
    std::string crop_id;
    std::string seed_item_id;
    std::string source;       // "battle", "spirit_plant", "wild"
    float drop_rate = 0.0f; // 0.0-1.0
    int min_star = 0;        // 最低星数要求（0表示无要求）
    std::string notes;
};

class SeedDropTable {
public:
    SeedDropTable() = default;

    bool LoadFromCsv(const std::string& file_path);

    [[nodiscard]] std::vector<const SeedDropEntry*> GetBattleDrops(int enemy_star) const;
    [[nodiscard]] std::vector<const SeedDropEntry*> GetSpiritPlantDrops() const;
    [[nodiscard]] const SeedDropEntry* GetWildDrop() const;

    [[nodiscard]] bool IsLoaded() const { return loaded_; }

private:
    std::vector<SeedDropEntry> entries_;
    bool loaded_ = false;
};

}  // namespace CloudSeamanor::domain
