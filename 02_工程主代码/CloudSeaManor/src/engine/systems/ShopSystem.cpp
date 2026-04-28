#include "CloudSeamanor/engine/systems/ShopSystem.hpp"

#include <algorithm>

namespace CloudSeamanor::engine {

const PriceTableEntry* ShopSystem::SelectShopStallItem(
    const std::vector<PriceTableEntry>& price_table,
    bool pet_adopted) const {
    if (!pet_adopted) {
        for (const auto& entry : price_table) {
            if (entry.buy_from == "shop" && entry.buy_price > 0 && entry.category == "pet") {
                return &entry;
            }
        }
    }
    for (const auto& entry : price_table) {
        if (entry.buy_from == "shop" && entry.buy_price > 0) {
            return &entry;
        }
    }
    return nullptr;
}

const PriceTableEntry* ShopSystem::SelectGeneralStoreItem(
    const std::vector<PriceTableEntry>& price_table,
    const std::vector<std::string>& daily_stock) const {
    for (const auto& entry : price_table) {
        if (entry.buy_from != "general_store" || entry.buy_price <= 0) {
            continue;
        }
        if (std::find(daily_stock.begin(), daily_stock.end(), entry.item_id) != daily_stock.end()) {
            return &entry;
        }
    }
    return nullptr;
}

const PriceTableEntry* ShopSystem::SelectBySource(
    const std::vector<PriceTableEntry>& price_table,
    const std::string& source) const {
    for (const auto& entry : price_table) {
        if (entry.buy_from == source && entry.buy_price > 0) {
            return &entry;
        }
    }
    return nullptr;
}

const PriceTableEntry* ShopSystem::SelectFirstSellable(
    const std::vector<PriceTableEntry>& price_table,
    const std::string& source,
    const CloudSeamanor::domain::Inventory& inventory) const {
    for (const auto& entry : price_table) {
        if (entry.buy_from == source && inventory.CountOf(entry.item_id) > 0) {
            return &entry;
        }
    }
    return nullptr;
}

std::vector<const PriceTableEntry*> ShopSystem::CollectSellableBySource(
    const std::vector<PriceTableEntry>& price_table,
    const std::string& source,
    const CloudSeamanor::domain::Inventory& inventory) const {
    std::vector<const PriceTableEntry*> options;
    for (const auto& entry : price_table) {
        if (entry.buy_from != source) {
            continue;
        }
        if (inventory.CountOf(entry.item_id) <= 0 || entry.sell_price <= 0) {
            continue;
        }
        options.push_back(&entry);
    }
    std::sort(options.begin(), options.end(), [](const PriceTableEntry* lhs, const PriceTableEntry* rhs) {
        if (lhs->sell_price != rhs->sell_price) {
            return lhs->sell_price > rhs->sell_price;
        }
        return lhs->item_id < rhs->item_id;
    });
    return options;
}

bool ShopSystem::CanPurchaseByWeeklyLimit(
    const PriceTableEntry& selected,
    const std::unordered_map<std::string, int>& weekly_buy_count) const {
    const auto it = weekly_buy_count.find(selected.item_id);
    if (it == weekly_buy_count.end()) {
        return true;
    }
    return it->second < WeeklyPurchaseLimitPerItem();
}

int ShopSystem::WeeklyPurchaseLimitPerItem() const {
    return 7;
}

bool ShopSystem::TryPurchase(
    const PriceTableEntry& selected,
    int& gold,
    CloudSeamanor::domain::Inventory& inventory,
    std::unordered_map<std::string, int>& weekly_buy_count,
    const std::function<bool(const PriceTableEntry&)>& custom_grant) const {
    if (selected.buy_price <= 0 || gold < selected.buy_price) {
        return false;
    }
    gold -= selected.buy_price;
    bool handled = false;
    if (custom_grant) {
        handled = custom_grant(selected);
    }
    if (!handled) {
        inventory.AddItem(selected.item_id, 1);
    }
    weekly_buy_count[selected.item_id] += 1;
    return true;
}

bool ShopSystem::TrySellOne(
    const PriceTableEntry& selected,
    CloudSeamanor::domain::CropQuality quality,
    CloudSeamanor::domain::Inventory& inventory,
    int& gold,
    std::unordered_map<std::string, int>& weekly_sell_count,
    int& out_income) const {
    if (!inventory.TryRemoveItem(selected.item_id, 1)) {
        return false;
    }
    const int multiplier = QualitySellMultiplierPercent(quality);
    out_income = std::max(1, selected.sell_price * multiplier / 100);
    gold += out_income;
    weekly_sell_count[selected.item_id] += 1;
    return true;
}

bool ShopSystem::CanOpenTideShop(CloudSeamanor::domain::CloudState state) const {
    return state == CloudSeamanor::domain::CloudState::Tide;
}

int ShopSystem::QualitySellMultiplierPercent(CloudSeamanor::domain::CropQuality quality) const {
    using Q = CloudSeamanor::domain::CropQuality;
    switch (quality) {
    case Q::Normal: return 100;
    case Q::Fine: return 150;
    case Q::Rare: return 200;
    case Q::Spirit: return 300;
    case Q::Holy: return 350;
    }
    return 100;
}

} // namespace CloudSeamanor::engine
