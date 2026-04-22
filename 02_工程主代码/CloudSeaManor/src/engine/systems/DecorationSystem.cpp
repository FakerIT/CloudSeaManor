#include "CloudSeamanor/engine/systems/DecorationSystem.hpp"

#include <algorithm>

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

    static constexpr const char* kFurnitureIds[] = {
        "furniture_table",
        "furniture_chair",
        "furniture_bed",
        "furniture_bookshelf",
        "furniture_plant",
    };
    static constexpr const char* kFurnitureNames[] = {
        "木桌",
        "靠背椅",
        "床铺",
        "书架",
        "盆栽",
    };
    const int index = std::clamp(decoration_score / 5, 0, 4);
    inventory.AddItem(kFurnitureIds[index], 1);
    decoration_score += 5;
    if (on_notify) {
        on_notify(std::string("制作完成：") + kFurnitureNames[index] + "，家园美观度 +5。");
    }
    if (decoration_score >= 20 && on_achievement) {
        on_achievement("home_designer");
    }
    return true;
}

} // namespace CloudSeamanor::engine

