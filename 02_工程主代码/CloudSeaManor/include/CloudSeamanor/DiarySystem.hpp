#pragma once

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"

#include <string>
#include <vector>

namespace CloudSeamanor::domain {

struct DiaryDefinition {
    std::string id;
    std::string title;
    std::string summary;
};

class DiarySystem {
public:
    bool LoadFromFile(const std::string& csv_path = "assets/data/diary/entries.csv");
    [[nodiscard]] std::vector<DiaryDefinition> CreateDefaultDefinitions() const;
    bool UnlockOnce(const DiaryDefinition& def, int current_day, std::vector<CloudSeamanor::engine::DiaryEntryState>& entries) const;
    [[nodiscard]] const DiaryDefinition* FindById(const std::string& id) const;

private:
    mutable std::vector<DiaryDefinition> cache_;
};

} // namespace CloudSeamanor::domain
