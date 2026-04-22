#include "catch2Compat.hpp"

#include "CloudSeamanor/engine/systems/ShopSystem.hpp"

using CloudSeamanor::domain::CloudState;
using CloudSeamanor::domain::CropQuality;
using CloudSeamanor::domain::Inventory;
using CloudSeamanor::engine::PriceTableEntry;
using CloudSeamanor::engine::ShopSystem;

namespace {

PriceTableEntry MakeEntry(
    const std::string& id,
    int buy_price,
    int sell_price,
    const std::string& source,
    const std::string& category = "") {
    PriceTableEntry e;
    e.item_id = id;
    e.buy_price = buy_price;
    e.sell_price = sell_price;
    e.buy_from = source;
    e.category = category;
    return e;
}

}  // namespace

TEST_CASE("ShopSystem::SelectShopStallItem prefers pet when not adopted") {
    ShopSystem shop;
    const std::vector<PriceTableEntry> table = {
        MakeEntry("TeaSeed", 20, 10, "shop", "seed"),
        MakeEntry("PetLicenseCat", 200, 0, "shop", "pet")
    };

    const PriceTableEntry* selected = shop.SelectShopStallItem(table, false);
    REQUIRE(selected != nullptr);
    CHECK_THAT(selected->item_id, Equals("PetLicenseCat"));
}

TEST_CASE("ShopSystem::SelectShopStallItem selects first purchasable after adoption") {
    ShopSystem shop;
    const std::vector<PriceTableEntry> table = {
        MakeEntry("TeaSeed", 20, 10, "shop", "seed"),
        MakeEntry("PetLicenseCat", 200, 0, "shop", "pet")
    };

    const PriceTableEntry* selected = shop.SelectShopStallItem(table, true);
    REQUIRE(selected != nullptr);
    CHECK_THAT(selected->item_id, Equals("TeaSeed"));
}

TEST_CASE("ShopSystem::SelectGeneralStoreItem respects daily stock") {
    ShopSystem shop;
    const std::vector<PriceTableEntry> table = {
        MakeEntry("TeaSeed", 20, 10, "general_store", "seed"),
        MakeEntry("TurnipSeed", 30, 15, "general_store", "seed")
    };
    const std::vector<std::string> stock = {"TurnipSeed"};

    const PriceTableEntry* selected = shop.SelectGeneralStoreItem(table, stock);
    REQUIRE(selected != nullptr);
    CHECK_THAT(selected->item_id, Equals("TurnipSeed"));
}

TEST_CASE("ShopSystem::TryPurchase fails on insufficient gold") {
    ShopSystem shop;
    Inventory inventory;
    int gold = 10;
    std::unordered_map<std::string, int> buy_count;

    const bool ok = shop.TryPurchase(
        MakeEntry("TeaSeed", 20, 0, "shop"),
        gold,
        inventory,
        buy_count,
        nullptr);

    CHECK_FALSE(ok);
    CHECK_EQ(gold, 10);
    CHECK_EQ(inventory.CountOf("TeaSeed"), 0);
}

TEST_CASE("ShopSystem::TryPurchase adds item and increments weekly counter") {
    ShopSystem shop;
    Inventory inventory;
    int gold = 100;
    std::unordered_map<std::string, int> buy_count;

    const bool ok = shop.TryPurchase(
        MakeEntry("TeaSeed", 20, 0, "shop"),
        gold,
        inventory,
        buy_count,
        nullptr);

    CHECK_TRUE(ok);
    CHECK_EQ(gold, 80);
    CHECK_EQ(inventory.CountOf("TeaSeed"), 1);
    CHECK_EQ(buy_count["TeaSeed"], 1);
}

TEST_CASE("ShopSystem::TrySellOne uses quality multiplier and consumes inventory") {
    ShopSystem shop;
    Inventory inventory;
    inventory.AddItem("TeaLeaf", 1);
    int gold = 0;
    int income = 0;
    std::unordered_map<std::string, int> sell_count;

    const bool ok = shop.TrySellOne(
        MakeEntry("TeaLeaf", 0, 10, "purchaser"),
        CropQuality::Rare,
        inventory,
        gold,
        sell_count,
        income);

    CHECK_TRUE(ok);
    CHECK_EQ(income, 20);
    CHECK_EQ(gold, 20);
    CHECK_EQ(inventory.CountOf("TeaLeaf"), 0);
    CHECK_EQ(sell_count["TeaLeaf"], 1);
}

TEST_CASE("ShopSystem::CanOpenTideShop only true on tide") {
    ShopSystem shop;
    CHECK_FALSE(shop.CanOpenTideShop(CloudState::Clear));
    CHECK_FALSE(shop.CanOpenTideShop(CloudState::Mist));
    CHECK_TRUE(shop.CanOpenTideShop(CloudState::Tide));
}
