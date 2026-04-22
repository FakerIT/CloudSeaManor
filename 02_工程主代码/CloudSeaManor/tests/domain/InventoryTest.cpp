// ============================================================================
// 【InventoryTest.cpp】Inventory 单元测试
// ============================================================================
// Test cases for CloudSeamanor::domain::Inventory
//
// Coverage:
// - AddItem basic operations
// - RemoveItem operations
// - HasItem/HasItems checks
// - CountOf queries
// - SlotCount and TotalItemCount
// - Serialization roundtrip (ToMap/FromMap)
// - Boundary conditions (empty id, negative count, etc.)
// ============================================================================

#include "catch2Compat.hpp"
#include "CloudSeamanor/Inventory.hpp"

using CloudSeamanor::domain::Inventory;
using CloudSeamanor::domain::ItemCount;

namespace {

ItemCount MakeItem(const std::string& id, int count) {
    return {id, count};
}

}  // namespace

// ============================================================================
// AddItem Tests
// ============================================================================

TEST_CASE("Inventory::AddItem - basic add")
{
    Inventory inv;
    inv.AddItem("Wood", 5);

    CHECK_EQ(inv.CountOf("Wood"), 5);
}

TEST_CASE("Inventory::AddItem - stacking same item")
{
    Inventory inv;
    inv.AddItem("Wood", 3);
    inv.AddItem("Wood", 2);

    CHECK_EQ(inv.CountOf("Wood"), 5);
    CHECK_EQ(inv.SlotCount(), 1u);
}

TEST_CASE("Inventory::AddItem - multiple different items")
{
    Inventory inv;
    inv.AddItem("TeaLeaf", 4);
    inv.AddItem("Wood", 5);
    inv.AddItem("Turnip", 2);

    CHECK_EQ(inv.CountOf("TeaLeaf"), 4);
    CHECK_EQ(inv.CountOf("Wood"), 5);
    CHECK_EQ(inv.CountOf("Turnip"), 2);
    CHECK_EQ(inv.SlotCount(), 3u);
}

TEST_CASE("Inventory::AddItem - default count is 1")
{
    Inventory inv;
    inv.AddItem("Stone");

    CHECK_EQ(inv.CountOf("Stone"), 1);
}

TEST_CASE("Inventory::AddItem - zero count is ignored")
{
    Inventory inv;
    inv.AddItem("Wood", 0);

    CHECK_EQ(inv.SlotCount(), 0u);
}

TEST_CASE("Inventory::AddItem - negative count is ignored")
{
    Inventory inv;
    inv.AddItem("Wood", 5);
    inv.AddItem("Wood", -3);

    CHECK_EQ(inv.CountOf("Wood"), 5);
}

TEST_CASE("Inventory::AddItem - empty item_id is ignored")
{
    Inventory inv;
    inv.AddItem("", 10);

    CHECK_EQ(inv.TotalItemCount(), 0);
}

TEST_CASE("Inventory::AddItems - batch add")
{
    Inventory inv;
    inv.AddItems({MakeItem("A", 3), MakeItem("B", 7), MakeItem("C", 1)});

    CHECK_EQ(inv.CountOf("A"), 3);
    CHECK_EQ(inv.CountOf("B"), 7);
    CHECK_EQ(inv.CountOf("C"), 1);
}

// ============================================================================
// RemoveItem Tests
// ============================================================================

TEST_CASE("Inventory::RemoveItem - success")
{
    Inventory inv;
    inv.AddItem("Stone", 10);
    bool ok = inv.RemoveItem("Stone", 3);

    CHECK_TRUE(ok);
    CHECK_EQ(inv.CountOf("Stone"), 7);
}

TEST_CASE("Inventory::RemoveItem - removes slot when count reaches zero")
{
    Inventory inv;
    inv.AddItem("Temp", 1);
    CHECK_EQ(inv.SlotCount(), 1u);

    inv.RemoveItem("Temp", 1);

    CHECK_EQ(inv.SlotCount(), 0u);
    CHECK_FALSE(inv.HasItem("Temp"));
}

TEST_CASE("Inventory::RemoveItem - insufficient quantity")
{
    Inventory inv;
    inv.AddItem("Stone", 2);
    bool ok = inv.RemoveItem("Stone", 5);

    CHECK_FALSE(ok);
    CHECK_EQ(inv.CountOf("Stone"), 2);
}

TEST_CASE("Inventory::RemoveItem - item not found")
{
    Inventory inv;
    bool ok = inv.RemoveItem("NonExistent", 1);

    CHECK_FALSE(ok);
}

TEST_CASE("Inventory::RemoveItem - zero count returns false")
{
    Inventory inv;
    inv.AddItem("Wood", 5);
    bool ok = inv.RemoveItem("Wood", 0);

    CHECK_FALSE(ok);
    CHECK_EQ(inv.CountOf("Wood"), 5);
}

TEST_CASE("Inventory::RemoveItem - negative count returns false")
{
    Inventory inv;
    inv.AddItem("Wood", 5);
    bool ok = inv.RemoveItem("Wood", -2);

    CHECK_FALSE(ok);
    CHECK_EQ(inv.CountOf("Wood"), 5);
}

TEST_CASE("Inventory::RemoveItems - atomic all-or-nothing")
{
    Inventory inv;
    inv.AddItem("Wood", 5);
    inv.AddItem("Stone", 3);

    bool result = inv.RemoveItems({
        MakeItem("Wood", 10),
        MakeItem("Stone", 2)
    });

    CHECK_FALSE(result);
    CHECK_EQ(inv.CountOf("Wood"), 5);
    CHECK_EQ(inv.CountOf("Stone"), 3);
}

TEST_CASE("Inventory::RemoveItems - all succeed")
{
    Inventory inv;
    inv.AddItem("Wood", 5);
    inv.AddItem("Stone", 3);

    bool result = inv.RemoveItems({
        MakeItem("Wood", 3),
        MakeItem("Stone", 2)
    });

    CHECK_TRUE(result);
    CHECK_EQ(inv.CountOf("Wood"), 2);
    CHECK_EQ(inv.CountOf("Stone"), 1);
}

TEST_CASE("Inventory::Clear - empties inventory")
{
    Inventory inv;
    inv.AddItem("Wood", 5);
    inv.AddItem("Stone", 3);

    inv.Clear();

    CHECK_EQ(inv.TotalItemCount(), 0);
    CHECK_TRUE(inv.IsEmpty());
}

// ============================================================================
// Query Tests
// ============================================================================

TEST_CASE("Inventory::CountOf - non-existent item returns 0")
{
    Inventory inv;
    inv.AddItem("Wood", 5);

    CHECK_EQ(inv.CountOf("NonExistent"), 0);
}

TEST_CASE("Inventory::HasItem - single item")
{
    Inventory inv;
    inv.AddItem("Seed", 3);

    CHECK_TRUE(inv.HasItem("Seed"));
    CHECK_FALSE(inv.HasItem("NonExistent"));
}

TEST_CASE("Inventory::HasItems - multiple items all sufficient")
{
    Inventory inv;
    inv.AddItem("A", 5);
    inv.AddItem("B", 3);

    bool result = inv.HasItems({
        MakeItem("A", 3),
        MakeItem("B", 2)
    });

    CHECK_TRUE(result);
}

TEST_CASE("Inventory::HasItems - one insufficient returns false")
{
    Inventory inv;
    inv.AddItem("A", 5);
    inv.AddItem("B", 3);

    bool result = inv.HasItems({
        MakeItem("A", 6),
        MakeItem("B", 2)
    });

    CHECK_FALSE(result);
}

TEST_CASE("Inventory::HasItems - empty bag returns false")
{
    Inventory inv;

    bool result = inv.HasItems({MakeItem("Wood", 1)});

    CHECK_FALSE(result);
}

TEST_CASE("Inventory::TotalItemCount - sum of all items")
{
    Inventory inv;
    inv.AddItem("TeaLeaf", 4);
    inv.AddItem("Wood", 5);
    inv.AddItem("Turnip", 2);

    CHECK_EQ(inv.TotalItemCount(), 11);
}

TEST_CASE("Inventory::SlotCount - distinct item types")
{
    Inventory inv;
    inv.AddItem("A", 10);
    inv.AddItem("B", 5);
    inv.AddItem("C", 3);

    CHECK_EQ(inv.SlotCount(), 3u);
}

TEST_CASE("Inventory::IsEmpty - empty inventory")
{
    Inventory inv;

    CHECK_TRUE(inv.IsEmpty());
}

TEST_CASE("Inventory::IsEmpty - non-empty inventory")
{
    Inventory inv;
    inv.AddItem("Wood", 5);

    CHECK_FALSE(inv.IsEmpty());
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST_CASE("Inventory::ToMap - converts to map correctly")
{
    Inventory inv;
    inv.AddItem("TeaLeaf", 4);
    inv.AddItem("Wood", 5);

    auto map = inv.ToMap();

    CHECK_EQ(map.size(), 2u);
    CHECK_EQ(map["TeaLeaf"], 4);
    CHECK_EQ(map["Wood"], 5);
}

TEST_CASE("Inventory::FromMap - restores from map")
{
    Inventory inv;
    std::unordered_map<std::string, int> data;
    data["TeaLeaf"] = 4;
    data["Wood"] = 5;

    inv.FromMap(data);

    CHECK_EQ(inv.CountOf("TeaLeaf"), 4);
    CHECK_EQ(inv.CountOf("Wood"), 5);
}

TEST_CASE("Inventory::ToMap/FromMap - roundtrip preserves data")
{
    Inventory original;
    original.AddItem("TeaSeed", 4);
    original.AddItem("Wood", 5);
    original.AddItem("TurnipSeed", 2);

    auto map = original.ToMap();

    Inventory restored;
    restored.FromMap(map);

    CHECK_EQ(restored.CountOf("TeaSeed"), 4);
    CHECK_EQ(restored.CountOf("Wood"), 5);
    CHECK_EQ(restored.CountOf("TurnipSeed"), 2);
    CHECK_EQ(restored.TotalItemCount(), original.TotalItemCount());
}

TEST_CASE("Inventory::FromMap - clears existing data")
{
    Inventory inv;
    inv.AddItem("Old", 100);

    std::unordered_map<std::string, int> data;
    data["New"] = 50;

    inv.FromMap(data);

    CHECK_EQ(inv.CountOf("Old"), 0);
    CHECK_EQ(inv.CountOf("New"), 50);
}

// ============================================================================
// SummaryText Tests
// ============================================================================

TEST_CASE("Inventory::SummaryText - empty inventory")
{
    Inventory inv;
    auto text = inv.SummaryText();

    CHECK_THAT(text, Contains("Empty"));
}

TEST_CASE("Inventory::SummaryText - with items")
{
    Inventory inv;
    inv.AddItem("Wood", 5);
    inv.AddItem("TeaLeaf", 3);

    auto text = inv.SummaryText();

    CHECK_THAT(text, Contains("Wood"));
    CHECK_THAT(text, Contains("5"));
    CHECK_THAT(text, Contains("TeaLeaf"));
    CHECK_THAT(text, Contains("3"));
}
