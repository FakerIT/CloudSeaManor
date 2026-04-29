#include "CloudSeamanor/domain/TeaProductData.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::domain {

bool TeaProductTable::LoadFromFile(const std::string& file_path) {
    profiles_.clear();
    std::ifstream in(file_path);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::stringstream ss(line);
        std::string part;
        TeaProductProfile profile;
        int col = 0;
        while (std::getline(ss, part, ',')) {
            switch (col) {
            case 0: profile.item_id = part; break;
            case 1: profile.quality = part; break;
            case 2: profile.sell_price = std::max(0, std::atoi(part.c_str())); break;
            case 3: profile.buff_effect_id = part; break;
            case 4: profile.gift_preference = part; break;
            default: break;
            }
            ++col;
        }
        if (!profile.item_id.empty()) {
            profiles_[profile.item_id] = std::move(profile);
        }
    }
    return !profiles_.empty();
}

const TeaProductProfile* TeaProductTable::Get(const std::string& item_id) const {
    const auto it = profiles_.find(item_id);
    return it == profiles_.end() ? nullptr : &it->second;
}

TeaProductTable& GetGlobalTeaProductTable() {
    static TeaProductTable table;
    return table;
}

}  // namespace CloudSeamanor::domain
