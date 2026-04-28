#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::domain {

struct TeaBushDefinition {
    std::string id;
    std::string name;
    std::string tea_id;
    int harvest_interval_days = 5;
    std::string unlock_condition;
};

class TeaBushTable {
public:
    bool LoadFromFile(const std::string& file_path);
    [[nodiscard]] const TeaBushDefinition* Get(const std::string& id) const;
    [[nodiscard]] const std::vector<TeaBushDefinition>& All() const noexcept { return bushes_; }

private:
    std::vector<TeaBushDefinition> bushes_;
    std::unordered_map<std::string, std::size_t> index_;
};

[[nodiscard]] TeaBushTable& GetGlobalTeaBushTable();

}  // namespace CloudSeamanor::domain

