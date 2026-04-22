// ============================================================================
// 【SkillSystemTest.cpp】技能系统单元测试
// ============================================================================
// 测试模块：CloudSeamanor::domain::SkillSystem
// 测试用例：10 个
//
// 迁移记录：从手写框架迁移到 catch2Compat.hpp（Catch2 v3 兼容）
// 修正说明：
//   - Initialize() 无参数
//   - AddExp(skill, base_exp, cloud_density, tea_buff, beast_share) 5个参数
// ============================================================================

#include "catch2Compat.hpp"
#include "CloudSeamanor/SkillSystem.hpp"

namespace CloudSeamanor {
namespace testing {

// ============================================================================
// SK-01: Initialize 初始化所有技能为1级
// ============================================================================
TEST_CASE("SkillSystem: Initialize sets all skills to level 1") {
    domain::SkillSystem skills;
    skills.Initialize();

    REQUIRE_EQ(skills.GetLevel(domain::SkillType::SpiritFarm), 1);
    REQUIRE_EQ(skills.GetLevel(domain::SkillType::SpiritForage), 1);
    REQUIRE_EQ(skills.GetLevel(domain::SkillType::SpiritFish), 1);
    REQUIRE_EQ(skills.GetLevel(domain::SkillType::SpiritMine), 1);
    REQUIRE_EQ(skills.GetLevel(domain::SkillType::SpiritGuard), 1);
}

// ============================================================================
// SK-02: GetLevel 返回正确等级
// ============================================================================
TEST_CASE("SkillSystem: GetLevel returns correct level") {
    domain::SkillSystem skills;
    skills.Initialize();
    REQUIRE_EQ(skills.GetLevel(domain::SkillType::SpiritFarm), 1);
}

// ============================================================================
// SK-03: GetBonus Lv1 返回 0.0
// ============================================================================
TEST_CASE("SkillSystem: GetBonus at level 1 returns 0.0") {
    domain::SkillSystem skills;
    skills.Initialize();
    REQUIRE_FLOAT_EQ(skills.GetBonus(domain::SkillType::SpiritFarm), 0.0f, 0.001f);
}

// ============================================================================
// SK-04: GetMaxLevel 返回 10
// ============================================================================
TEST_CASE("SkillSystem: GetMaxLevel returns 10") {
    domain::SkillSystem skills;
    skills.Initialize();
    REQUIRE_EQ(skills.GetMaxLevel(domain::SkillType::SpiritFarm), 10);
}

// ============================================================================
// SK-05: GetExpThreshold(2) 返回正数
// ============================================================================
TEST_CASE("SkillSystem: GetExpThreshold returns positive value for level 2") {
    domain::SkillSystem skills;
    skills.Initialize();
    float threshold = skills.GetExpThreshold(domain::SkillType::SpiritFarm, 2);
    REQUIRE(threshold > 0.0f);
}

// ============================================================================
// SK-06: GetExpThreshold(1) 返回 0
// ============================================================================
TEST_CASE("SkillSystem: GetExpThreshold for level 1 returns 0") {
    domain::SkillSystem skills;
    skills.Initialize();
    float threshold = skills.GetExpThreshold(domain::SkillType::SpiritFarm, 1);
    REQUIRE_FLOAT_EQ(threshold, 0.0f, 0.001f);
}

// ============================================================================
// SK-07: GetSkillName 返回非空字符串
// ============================================================================
TEST_CASE("SkillSystem: GetSkillName returns non-empty string") {
    domain::SkillSystem skills;
    skills.Initialize();
    std::string name = skills.GetSkillName(domain::SkillType::SpiritFarm);
    REQUIRE_FALSE(name.empty());
}

// ============================================================================
// SK-08: AddExp 增加经验值
// ============================================================================
TEST_CASE("SkillSystem: AddExp increases experience") {
    domain::SkillSystem skills;
    skills.Initialize();
    float before = skills.GetExp(domain::SkillType::SpiritFarm);
    skills.AddExp(domain::SkillType::SpiritFarm, 50.0f, 1.0f, 1.0f, 1.0f);
    REQUIRE(skills.GetExp(domain::SkillType::SpiritFarm) > before);
}

// ============================================================================
// SK-09: GetExpRatio 返回 0~1
// ============================================================================
TEST_CASE("SkillSystem: GetExpRatio is within [0, 1]") {
    domain::SkillSystem skills;
    skills.Initialize();
    float ratio = skills.GetExpRatio(domain::SkillType::SpiritFarm);
    REQUIRE(ratio >= 0.0f);
    REQUIRE(ratio <= 1.0f);
}

// ============================================================================
// SK-10: 未初始化的技能返回默认值
// ============================================================================
TEST_CASE("SkillSystem: Uninitialized skill returns defaults") {
    domain::SkillSystem skills;
    // 未调用 Initialize() 时，GetLevel 应返回默认值 1
    REQUIRE_EQ(skills.GetLevel(domain::SkillType::SpiritFarm), 1);
    REQUIRE_FLOAT_EQ(skills.GetBonus(domain::SkillType::SpiritFarm), 0.0f, 0.001f);
}

}  // namespace testing
}  // namespace CloudSeamanor
