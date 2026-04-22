#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace CloudSeamanor::infrastructure {

struct SaveSlotMetadata {
    int slot_index = 1;
    bool exists = false;
    std::string saved_at_text;
    int day = 0;
    std::string season_text;
    std::string thumbnail_path;
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

private:
    std::filesystem::path root_dir_;
};

}  // namespace CloudSeamanor::infrastructure
