#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

struct HungerDefinition {
    std::string item_id;
    std::string item_name;
    int hunger_restore = 0;
    float quality_multiplier = 1.0f;
    std::string source_tag;
};

class HungerTable {
public:
    bool LoadFromFile(const std::string& file_path);
    [[nodiscard]] const HungerDefinition* Get(const std::string& item_id) const;

private:
    std::vector<HungerDefinition> entries_;
    std::unordered_map<std::string, std::size_t> index_;
};

[[nodiscard]] HungerTable& GetGlobalHungerTable();

}  // namespace CloudSeamanor::domain

