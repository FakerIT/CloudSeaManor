#pragma once

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"
#include "CloudSeamanor/CloudSystem.hpp"
#include "CloudSeamanor/Inventory.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CloudSeamanor::engine {

class ShopSystem {
public:
    [[nodiscard]] const PriceTableEntry* SelectShopStallItem(
        const std::vector<PriceTableEntry>& price_table,
        bool pet_adopted) const;
    [[nodiscard]] const PriceTableEntry* SelectGeneralStoreItem(
        const std::vector<PriceTableEntry>& price_table,
        const std::vector<std::string>& daily_stock) const;
    [[nodiscard]] const PriceTableEntry* SelectBySource(
        const std::vector<PriceTableEntry>& price_table,
        const std::string& source) const;
    [[nodiscard]] const PriceTableEntry* SelectFirstSellable(
        const std::vector<PriceTableEntry>& price_table,
        const std::string& source,
        const CloudSeamanor::domain::Inventory& inventory) const;

    [[nodiscard]] bool TryPurchase(
        const PriceTableEntry& selected,
        int& gold,
        CloudSeamanor::domain::Inventory& inventory,
        std::unordered_map<std::string, int>& weekly_buy_count,
        const std::function<bool(const PriceTableEntry&)>& custom_grant) const;
    [[nodiscard]] bool TrySellOne(
        const PriceTableEntry& selected,
        CloudSeamanor::domain::CropQuality quality,
        CloudSeamanor::domain::Inventory& inventory,
        int& gold,
        std::unordered_map<std::string, int>& weekly_sell_count,
        int& out_income) const;
    [[nodiscard]] bool CanOpenTideShop(CloudSeamanor::domain::CloudState state) const;

private:
    [[nodiscard]] int QualitySellMultiplierPercent(CloudSeamanor::domain::CropQuality quality) const;
};

} // namespace CloudSeamanor::engine
