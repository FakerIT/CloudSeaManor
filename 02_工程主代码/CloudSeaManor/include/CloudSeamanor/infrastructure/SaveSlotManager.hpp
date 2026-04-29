#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

struct SaveSlotMetadata {
    int slot_index = 1;
    bool exists = false;
    bool metadata_corrupted = false;
    std::string display_name;
    std::string saved_at_text;
    int day = 0;
    std::string season_text;
    std::string thumbnail_path;
    std::string summary_text;
    int main_plot_stage = 0;
    int key_achievement_count = 0;
    std::string last_battle_outcome;
};

class SaveSlotManager {
public:
    explicit SaveSlotManager(std::filesystem::path root_dir = std::filesystem::path("saves"));

    [[nodiscard]] bool IsValidSlot(int slot_index) const;
    [[nodiscard]] std::filesystem::path BuildSlotPath(int slot_index) const;
    [[nodiscard]] std::filesystem::path BuildMetaPath(int slot_index) const;

    void EnsureDirectories() const;
    bool WriteMetadata(int slot_index,
                       const SaveSlotMetadata& metadata) const;
    [[nodiscard]] SaveSlotMetadata ReadMetadata(int slot_index) const;
    [[nodiscard]] std::vector<SaveSlotMetadata> ReadAllMetadata() const;
    bool DeleteSlot(int slot_index) const;
    bool CopySlot(int from_slot_index, int to_slot_index, bool overwrite_target) const;
    bool RenameSlotDisplayName(int slot_index, const std::string& display_name) const;

    // 参考存档一键导入：
    // - 解析 ES3 参考存档
    // - 导出归一化 JSON 到 assets/data/reference/
    // - 根据 Player.log 风险给出兼容等级与字段降级策略
    // - 将导入结果写入目标槽位文件（引用导入摘要）
    bool ImportReferenceSaveToSlot(
        int slot_index,
        const std::filesystem::path& es3_path,
        const std::filesystem::path& player_log_path,
        const std::filesystem::path& normalized_export_dir = std::filesystem::path("assets/data/reference")) const;

private:
    std::filesystem::path root_dir_;
};

}  // namespace CloudSeamanor::infrastructure
