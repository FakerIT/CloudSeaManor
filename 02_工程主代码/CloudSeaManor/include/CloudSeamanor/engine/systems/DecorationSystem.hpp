#pragma once

#include "CloudSeamanor/Inventory.hpp"

#include <functional>
#include <string>

namespace CloudSeamanor::engine {

class DecorationSystem {
public:
    bool TryCraftDecoration(
        CloudSeamanor::domain::Inventory& inventory,
        int& decoration_score,
        const std::function<void(const std::string&)>& on_notify,
        const std::function<void(const std::string&)>& on_achievement) const;
};

} // namespace CloudSeamanor::engine

