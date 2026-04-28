// ============================================================================
// 【CropDataTest.cpp】CropData 单元测试
// ============================================================================
// Test cases for CloudSeamanor::domain::CropTable and related types
//
// Coverage:
// - CropDefinition structure
// - CropTable loading and queries
// - CropQuality calculation
// - Quality text conversion
// - Harvest multiplier calculation
// - Stage growth threshold
// ============================================================================

#include "../catch2Compat.hpp"
#include "CloudSeamanor/CropData.hpp"
#include "CloudSeamanor/CloudSystem.hpp"

using CloudSeamanor::domain::CropDefinition;
using CloudSeamanor::domain::CropQuality;
using CloudSeamanor::domain::CropTable;
using CloudSeamanor::domain::CloudState;

namespace {

CropTable& GetTable() {
    return CloudSeamanor::domain::GetGlobalCropTable();
}

}  // namespace

// ============================================================================
// CropDefinition Tests
// ============================================================================

TEST_CASE("CropDefinition - default values")
{
    CropDefinition def;
    CHECK_EQ(def.id, "");
    CHECK_EQ(def.name, "");
    CHECK_EQ(def.seed_item_id, "");
    CHECK_EQ(def.harvest_item_id, "");
    CHECK_FLOAT_EQ(def.growth_time, 80.0f, 0.001f);
    CHECK_EQ(def.stages, 4);
    CHECK_EQ(def.base_harvest, 2);
    CHECK_EQ(def.stamina_cost, 6);
}

TEST_CASE("CropDefinition - custom values")
{
    CropDefinition def;
    def.id = "tea_leaf";
    def.name = "灵茶叶";
    def.seed_item_id = "tea_seed";
    def.harvest_item_id = "tea_leaf";
    def.growth_time = 120.0f;
    def.stages = 5;
    def.base_harvest = 3;
    def.stamina_cost = 8;

    CHECK_THAT(def.id, Equals("tea_leaf"));
    CHECK_THAT(def.name, Equals("灵茶叶"));
    CHECK_FLOAT_EQ(def.growth_time, 120.0f, 0.001f);
    CHECK_EQ(def.stages, 5);
    CHECK_EQ(def.base_harvest, 3);
    CHECK_EQ(def.stamina_cost, 8);
}

// ============================================================================
// CropTable Query Tests
// ============================================================================

TEST_CASE("CropTable::Exists - returns false for non-existent crop")
{
    CropTable table;
    CHECK_FALSE(table.Exists("non_existent"));
}

TEST_CASE("CropTable::Get - returns nullptr for non-existent crop")
{
    CropTable table;
    CHECK_TRUE(table.Get("non_existent") == nullptr);
}

TEST_CASE("CropTable::AllCrops - returns empty vector initially")
{
    CropTable table;
    CHECK_EQ(table.AllCrops().size(), 0u);
}

TEST_CASE("CropTable::GetByTag - returns empty for non-existent tag")
{
    CropTable table;
    auto result = table.GetByTag("legendary");
    CHECK_EQ(result.size(), 0u);
}

// ============================================================================
// CropQuality Calculation Tests
// ============================================================================

TEST_CASE("CropTable::CalculateQuality - Clear produces Normal")
{
    auto quality = CropTable::CalculateQuality(CloudState::Clear);
    CHECK_EQ(quality, CropQuality::Normal);
}

TEST_CASE("CropTable::CalculateQuality - DenseCloud produces Fine")
{
    auto quality = CropTable::CalculateQuality(CloudState::DenseCloud);
    CHECK_EQ(quality, CropQuality::Fine);
}

TEST_CASE("CropTable::CalculateQuality - Tide produces Rare")
{
    auto quality = CropTable::CalculateQuality(CloudState::Tide);
    CHECK_EQ(quality, CropQuality::Rare);
}

TEST_CASE("CropTable::CalculateQuality - Tide + SpiritTier produces Spirit")
{
    auto quality = CropTable::CalculateQuality(CloudState::Tide, true);
    CHECK_EQ(quality, CropQuality::Spirit);
}

TEST_CASE("CropTable::CalculateQuality - SpiritTier without Tide produces Spirit")
{
    auto quality = CropTable::CalculateQuality(CloudState::Mist, true);
    CHECK_EQ(quality, CropQuality::Spirit);
}

// ============================================================================
// Quality Text Conversion Tests
// ============================================================================

TEST_CASE("CropTable::QualityToText - all quality levels")
{
    CHECK_THAT(CropTable::QualityToText(CropQuality::Normal), Equals("普通"));
    CHECK_THAT(CropTable::QualityToText(CropQuality::Fine), Equals("优质"));
    CHECK_THAT(CropTable::QualityToText(CropQuality::Rare), Equals("稀有"));
    CHECK_THAT(CropTable::QualityToText(CropQuality::Spirit), Equals("灵品"));
    CHECK_THAT(CropTable::QualityToText(CropQuality::Holy), Equals("圣品"));
}

TEST_CASE("CropTable::QualityToPrefixText - includes quality prefix")
{
    auto text = CropTable::QualityToPrefixText(CropQuality::Fine);
    CHECK_THAT(text, Contains("优质"));

    text = CropTable::QualityToPrefixText(CropQuality::Rare);
    CHECK_THAT(text, Contains("稀有"));
}

// ============================================================================
// Quality Harvest Multiplier Tests
// ============================================================================

TEST_CASE("CropTable::QualityHarvestMultiplier - Normal is 1.0x")
{
    CHECK_FLOAT_EQ(CropTable::QualityHarvestMultiplier(CropQuality::Normal), 1.0f, 0.001f);
}

TEST_CASE("CropTable::QualityHarvestMultiplier - Fine is 1.3x")
{
    CHECK_FLOAT_EQ(CropTable::QualityHarvestMultiplier(CropQuality::Fine), 1.3f, 0.001f);
}

TEST_CASE("CropTable::QualityHarvestMultiplier - Rare is 1.8x")
{
    CHECK_FLOAT_EQ(CropTable::QualityHarvestMultiplier(CropQuality::Rare), 1.8f, 0.001f);
}

TEST_CASE("CropTable::QualityHarvestMultiplier - Spirit is 2.5x")
{
    CHECK_FLOAT_EQ(CropTable::QualityHarvestMultiplier(CropQuality::Spirit), 2.5f, 0.001f);
}

// ============================================================================
// Stage Growth Threshold Tests
// ============================================================================

TEST_CASE("CropTable::StageGrowthThreshold - 4 stages")
{
    auto t0 = CropTable::StageGrowthThreshold(0, 4);
    CHECK_FLOAT_EQ(t0, 0.0f, 0.001f);

    auto t1 = CropTable::StageGrowthThreshold(1, 4);
    CHECK_FLOAT_EQ(t1, 0.25f, 0.001f);

    auto t2 = CropTable::StageGrowthThreshold(2, 4);
    CHECK_FLOAT_EQ(t2, 0.5f, 0.001f);

    auto t3 = CropTable::StageGrowthThreshold(3, 4);
    CHECK_FLOAT_EQ(t3, 0.75f, 0.001f);
}

TEST_CASE("CropTable::StageGrowthThreshold - 5 stages")
{
    auto t0 = CropTable::StageGrowthThreshold(0, 5);
    CHECK_FLOAT_EQ(t0, 0.0f, 0.001f);

    auto t2 = CropTable::StageGrowthThreshold(2, 5);
    CHECK_FLOAT_EQ(t2, 0.4f, 0.001f);

    auto t4 = CropTable::StageGrowthThreshold(4, 5);
    CHECK_FLOAT_EQ(t4, 0.8f, 0.001f);
}

TEST_CASE("CropTable::StageGrowthThreshold - single stage")
{
    auto t0 = CropTable::StageGrowthThreshold(0, 1);
    CHECK_FLOAT_EQ(t0, 0.0f, 0.001f);
}

// ============================================================================
// Tag Parsing Tests
// ============================================================================

TEST_CASE("CropTable - tag parsing (internal)")
{
    CropDefinition def;
    def.tags = {"core", "premium"};

    CHECK_EQ(def.tags.size(), 2u);
    CHECK_THAT(def.tags[0], Equals("core"));
    CHECK_THAT(def.tags[1], Equals("premium"));
}

// ============================================================================
// GetGlobalCropTable Tests
// ============================================================================

TEST_CASE("GetGlobalCropTable - returns singleton")
{
    auto& table1 = GetTable();
    auto& table2 = GetTable();

    CHECK_TRUE(&table1 == &table2);
}
