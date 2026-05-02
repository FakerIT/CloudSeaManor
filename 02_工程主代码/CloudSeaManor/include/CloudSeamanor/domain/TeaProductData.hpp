#pragma once

#include <string>
#include <unordered_map>

namespace CloudSeamanor::domain {

struct TeaProductProfile {
    std::string item_id;
    std::string quality;
    int sell_price = 0;
    std::string buff_effect_id;
    std::string gift_preference;
};

class TeaProductTable {
public:
    bool LoadFromFile(const std::string& file_path);
    [[nodiscard]] const TeaProductProfile* Get(const std::string& item_id) const;

private:
    std::unordered_map<std::string, TeaProductProfile> profiles_;
};

[[nodiscard]] TeaProductTable& GetGlobalTeaProductTable();

}  // namespace CloudSeamanor::domain
