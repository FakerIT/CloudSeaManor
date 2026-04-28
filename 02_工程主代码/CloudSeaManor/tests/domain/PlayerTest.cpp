// ============================================================================
// 【PlayerTest.cpp】Player 单元测试
// ============================================================================
// Test cases for CloudSeamanor::domain::Player
//
// Coverage:
// - Construction and default values
// - Position getter/setter
// - Movement and collision detection
// - Facing direction
// - Bounds calculation
// - Save/Load position
// ============================================================================

#include "../catch2Compat.hpp"
#include "CloudSeamanor/Player.hpp"

using CloudSeamanor::domain::FacingDirection;
using CloudSeamanor::domain::FacingFromVector;
using CloudSeamanor::domain::RectF;
using CloudSeamanor::domain::Vec2f;
using CloudSeamanor::domain::Player;

namespace {

RectF DefaultBounds() {
    return RectF{{0.0f, 0.0f}, {800.0f, 600.0f}};
}

std::vector<RectF> EmptyObstacles() {
    return {};
}

}  // namespace

// ============================================================================
// Construction Tests
// ============================================================================

TEST_CASE("Player::Construction - default position")
{
    Player player;

    auto pos = player.GetPosition();
    CHECK_EQ(pos.x, 620.0f);
    CHECK_EQ(pos.y, 340.0f);
}

TEST_CASE("Player::Construction - default facing")
{
    Player player;

    CHECK_EQ(player.Facing(), FacingDirection::Down);
}

TEST_CASE("Player::Construction - not moving initially")
{
    Player player;

    CHECK_FALSE(player.IsMoving());
}

// ============================================================================
// Position Tests
// ============================================================================

TEST_CASE("Player::SetPosition - updates position correctly")
{
    Player player;
    player.SetPosition({100.0f, 200.0f});

    auto pos = player.GetPosition();
    CHECK_EQ(pos.x, 100.0f);
    CHECK_EQ(pos.y, 200.0f);
}

TEST_CASE("Player::GetPosition - returns accurate position")
{
    Player player;
    player.SetPosition({450.0f, 320.0f});

    auto pos = player.GetPosition();
    CHECK_EQ(pos.x, 450.0f);
    CHECK_EQ(pos.y, 320.0f);
}

TEST_CASE("Player::Bounds - returns correct AABB")
{
    Player player;
    player.SetPosition({100.0f, 100.0f});

    auto bounds = player.Bounds();
    CHECK_EQ(bounds.position.x, 100.0f);
    CHECK_EQ(bounds.position.y, 100.0f);
    CHECK_EQ(bounds.size.x, 36.0f);
    CHECK_EQ(bounds.size.y, 36.0f);
}

// ============================================================================
// Movement Tests
// ============================================================================

TEST_CASE("Player::Move - without obstacles updates position")
{
    Player player;
    player.SetPosition({100.0f, 100.0f});

    player.Move({50.0f, 0.0f}, DefaultBounds(), EmptyObstacles());

    auto pos = player.GetPosition();
    CHECK_EQ(pos.x, 150.0f);
    CHECK_EQ(pos.y, 100.0f);
}

TEST_CASE("Player::Move - vertical movement")
{
    Player player;
    player.SetPosition({200.0f, 200.0f});

    player.Move({0.0f, 30.0f}, DefaultBounds(), EmptyObstacles());

    auto pos = player.GetPosition();
    CHECK_EQ(pos.x, 200.0f);
    CHECK_EQ(pos.y, 230.0f);
}

TEST_CASE("Player::Move - diagonal movement")
{
    Player player;
    player.SetPosition({200.0f, 200.0f});

    player.Move({20.0f, 20.0f}, DefaultBounds(), EmptyObstacles());

    auto pos = player.GetPosition();
    CHECK_EQ(pos.x, 220.0f);
    CHECK_EQ(pos.y, 220.0f);
}

TEST_CASE("Player::Move - X axis collision with obstacle")
{
    Player player;
    player.SetPosition({100.0f, 100.0f});

    std::vector<RectF> obstacles{{{150.0f, 80.0f}, {50.0f, 50.0f}}};

    player.Move({60.0f, 0.0f}, DefaultBounds(), obstacles);

    CHECK_EQ(player.GetPosition().x, 140.0f);
}

TEST_CASE("Player::Move - Y axis collision with obstacle")
{
    Player player;
    player.SetPosition({100.0f, 100.0f});

    std::vector<RectF> obstacles{{{80.0f, 150.0f}, {50.0f, 50.0f}}};

    player.Move({0.0f, 60.0f}, DefaultBounds(), obstacles);

    CHECK_EQ(player.GetPosition().y, 140.0f);
}

TEST_CASE("Player::Move - boundary clamping right edge")
{
    Player player;
    player.SetPosition({750.0f, 300.0f});

    player.Move({100.0f, 0.0f}, DefaultBounds(), EmptyObstacles());

    CHECK_LE(player.GetPosition().x, 800.0f - 36.0f);
}

TEST_CASE("Player::Move - boundary clamping bottom edge")
{
    Player player;
    player.SetPosition({400.0f, 550.0f});

    player.Move({0.0f, 100.0f}, DefaultBounds(), EmptyObstacles());

    CHECK_LE(player.GetPosition().y, 600.0f - 36.0f);
}

// ============================================================================
// Facing Direction Tests
// ============================================================================

TEST_CASE("Player::SetMovementState - facing up-right")
{
    Player player;
    player.SetMovementState({1.0f, -1.0f}, true);

    CHECK_EQ(player.Facing(), FacingDirection::UpRight);
}

TEST_CASE("Player::SetMovementState - facing down-left")
{
    Player player;
    player.SetMovementState({-1.0f, 1.0f}, true);

    CHECK_EQ(player.Facing(), FacingDirection::DownLeft);
}

TEST_CASE("Player::SetMovementState - facing pure left")
{
    Player player;
    player.SetMovementState({-1.0f, 0.0f}, true);

    CHECK_EQ(player.Facing(), FacingDirection::Left);
}

TEST_CASE("Player::SetMovementState - facing pure up")
{
    Player player;
    player.SetMovementState({0.0f, -1.0f}, true);

    CHECK_EQ(player.Facing(), FacingDirection::Up);
}

TEST_CASE("Player::SetMovementState - zero vector keeps previous facing")
{
    Player player;
    player.SetMovementState({1.0f, 0.0f}, true);
    CHECK_EQ(player.Facing(), FacingDirection::Right);

    player.SetMovementState({0.0f, 0.0f}, false);

    CHECK_EQ(player.Facing(), FacingDirection::Right);
}

TEST_CASE("Player::FacingText - returns Chinese description")
{
    Player player;

    player.SetMovementState({0.0f, 1.0f}, true);
    CHECK_THAT(player.FacingText(), Equals("下"));

    player.SetMovementState({-1.0f, 0.0f}, true);
    CHECK_THAT(player.FacingText(), Equals("左"));

    player.SetMovementState({1.0f, 1.0f}, true);
    CHECK_THAT(player.FacingText(), Equals("右下"));
}

// ============================================================================
// Save/Load Tests
// ============================================================================

TEST_CASE("Player::SavePosition - format is x,y")
{
    Player player;
    player.SetPosition({123.0f, 456.0f});

    auto saved = player.SavePosition();
    CHECK_THAT(saved, Equals("123.00,456.00"));
}

TEST_CASE("Player::LoadPosition - restores position correctly")
{
    Player player;
    player.LoadPosition(250.0f, 380.0f);

    auto pos = player.GetPosition();
    CHECK_EQ(pos.x, 250.0f);
    CHECK_EQ(pos.y, 380.0f);
}

TEST_CASE("Player::LoadPosition - then SavePosition roundtrip")
{
    Player player;
    player.SetPosition({333.0f, 777.0f});

    auto saved = player.SavePosition();
    player.SetPosition({0.0f, 0.0f});
    player.LoadPosition(333.0f, 777.0f);

    CHECK_EQ(player.SavePosition(), saved);
}

// ============================================================================
// FacingFromVector Utility Tests
// ============================================================================

TEST_CASE("FacingFromVector - pure directions")
{
    CHECK_EQ(FacingFromVector({0.0f, 1.0f}), FacingDirection::Down);
    CHECK_EQ(FacingFromVector({0.0f, -1.0f}), FacingDirection::Up);
    CHECK_EQ(FacingFromVector({1.0f, 0.0f}), FacingDirection::Right);
    CHECK_EQ(FacingFromVector({-1.0f, 0.0f}), FacingDirection::Left);
}

TEST_CASE("FacingFromVector - diagonal directions")
{
    CHECK_EQ(FacingFromVector({1.0f, -1.0f}), FacingDirection::UpRight);
    CHECK_EQ(FacingFromVector({-1.0f, -1.0f}), FacingDirection::UpLeft);
    CHECK_EQ(FacingFromVector({1.0f, 1.0f}), FacingDirection::DownRight);
    CHECK_EQ(FacingFromVector({-1.0f, 1.0f}), FacingDirection::DownLeft);
}

TEST_CASE("FacingFromVector - zero vector returns Down")
{
    CHECK_EQ(FacingFromVector({0.0f, 0.0f}), FacingDirection::Down);
}

// ============================================================================
// IsMoving State Tests
// ============================================================================

TEST_CASE("Player::IsMoving - true when moving")
{
    Player player;
    player.SetMovementState({1.0f, 0.0f}, true);

    CHECK_TRUE(player.IsMoving());
}

TEST_CASE("Player::IsMoving - false when stopped")
{
    Player player;
    player.SetMovementState({0.0f, 0.0f}, false);

    CHECK_FALSE(player.IsMoving());
}
