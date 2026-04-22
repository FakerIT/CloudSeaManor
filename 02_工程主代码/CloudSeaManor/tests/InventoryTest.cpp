#include "TestFramework.hpp"
#include "CloudSeamanor/Inventory.hpp"

using CloudSeamanor::engine::RegisterTest;

namespace CloudSeamanor {
namespace tests {

// ============================================================================
// Inventory 测试组
// ============================================================================

TEST_CASE(TestInventoryAddSingleItem) {
    domain::Inventory inv;
    inv.AddItem("TeaLeaf", 1);
    ASSERT_EQ(inv.CountOf("TeaLeaf"), 1);
    return true;
}

TEST_CASE(TestInventoryAddMultipleItems) {
    domain::Inventory inv;
    inv.AddItem("Wood", 5);
    inv.AddItem("Wood", 3);
    ASSERT_EQ(inv.CountOf("Wood"), 8);
    return true;
}

TEST_CASE(TestInventoryRemoveItem) {
    domain::Inventory inv;
    inv.AddItem("Turnip", 10);
    bool ok = inv.RemoveItem("Turnip", 3);
    ASSERT_TRUE(ok);
    ASSERT_EQ(inv.CountOf("Turnip"), 7);
    return true;
}

TEST_CASE(TestInventoryRemoveInsufficientItems) {
    domain::Inventory inv;
    inv.AddItem("Stone", 2);
    bool ok = inv.RemoveItem("Stone", 5);
    ASSERT_FALSE(ok);
    ASSERT_EQ(inv.CountOf("Stone"), 2);  // 数量不变
    return true;
}

TEST_CASE(TestInventoryRemoveZeroItems) {
    domain::Inventory inv;
    inv.AddItem("Berry", 5);
    bool ok = inv.RemoveItem("Berry", 0);
    ASSERT_FALSE(ok);
    ASSERT_EQ(inv.CountOf("Berry"), 5);
    return true;
}

TEST_CASE(TestInventoryHasItem) {
    domain::Inventory inv;
    inv.AddItem("Seed", 3);
    ASSERT_TRUE(inv.HasItem("Seed"));
    ASSERT_FALSE(inv.HasItem("NonExistent"));
    return true;
}

TEST_CASE(TestInventorySlotCount) {
    domain::Inventory inv;
    inv.AddItem("A", 1);
    inv.AddItem("B", 1);
    inv.AddItem("C", 1);
    ASSERT_EQ(inv.SlotCount(), 3u);
    return true;
}

TEST_CASE(TestInventoryClear) {
    domain::Inventory inv;
    inv.AddItem("X", 99);
    inv.Clear();
    ASSERT_TRUE(inv.IsEmpty());
    ASSERT_EQ(inv.CountOf("X"), 0);
    return true;
}

TEST_CASE(TestInventoryToMapAndBack) {
    domain::Inventory inv;
    inv.AddItem("Rice", 20);
    inv.AddItem("Herb", 5);

    auto map = inv.ToMap();
    ASSERT_EQ(map["Rice"], 20);
    ASSERT_EQ(map["Herb"], 5);

    domain::Inventory restored;
    restored.FromMap(map);
    ASSERT_EQ(restored.CountOf("Rice"), 20);
    ASSERT_EQ(restored.CountOf("Herb"), 5);
    return true;
}

TEST_CASE(TestInventorySummaryText) {
    domain::Inventory inv;
    inv.AddItem("TeaLeaf", 3);
    std::string summary = inv.SummaryText();
    ASSERT_TRUE(!summary.empty());
    return true;
}

}  // namespace tests
}  // namespace CloudSeamanor
