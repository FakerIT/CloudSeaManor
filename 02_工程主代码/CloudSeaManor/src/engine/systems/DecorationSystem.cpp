#include "CloudSeamanor/engine/systems/DecorationSystem.hpp"

namespace CloudSeamanor::engine {

bool DecorationSystem::TryCraftDecoration(
    CloudSeamanor::domain::Inventory& inventory,
    int& decoration_score,
    const std::function<void(const std::string&)>& on_notify,
    const std::function<void(const std::string&)>& on_achievement) const {
    if (!inventory.TryRemoveItem("Wood", 2)) {
        if (on_notify) {
            on_notify("装饰需要木材 x2。");
        }
        return false;
    }

    decoration_score += 5;
    if (on_notify) {
        on_notify("装饰完成，家园美观度 +5。");
    }
    if (decoration_score >= 20 && on_achievement) {
        on_achievement("home_designer");
    }
    return true;
}

} // namespace CloudSeamanor::engine

