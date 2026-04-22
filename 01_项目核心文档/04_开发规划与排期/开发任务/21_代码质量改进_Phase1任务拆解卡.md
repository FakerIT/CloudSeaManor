# 代码质量改进任务拆解卡（Phase 1 - 紧急重构）

## 任务概览

**任务来源**：[代码质量审核报告](../03_优化方案/01_代码质量与架构审核报告_v1.0.md)
**目标阶段**：MVP之后的第一轮工程化重构
**总工期**：4周（20个工作日）
**优先级**：🔥🔥🔥 **最高优先级**

---

## 一、任务目标

### 1.1 核心目标

1. **将GameApp.cpp从1068行拆分到<200行**
2. **建立完整的单元测试体系，核心模块覆盖率≥70%**
3. **消除硬编码数值，实现配置化驱动**
4. **提升代码可维护性和团队协作效率**

### 1.2 验收标准

- [ ] GameApp.cpp ≤ 200行（仅保留协调调用）
- [ ] 所有新增模块有对应的单元测试
- [ ] CI/CD自动运行测试，失败阻塞合并
- [ ] 零新增编译警告
- [ ] UI参数100%配置化
- [ ] 文档同步更新（CODEBASE_GUIDE.md）

---

## 二、Week 1：系统拆分 + 测试框架搭建

### Week 1 目标

完成GameApp.cpp的第一次大拆分，建立测试基础设施，为核心模块编写首批测试。

---

### Task 1.1：创建新的目录结构

**优先级**：🔥🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：无  
**产出物**：目录结构 + CMakeLists.txt更新

#### 具体步骤

```
1. 在 src/engine/ 下创建以下子目录：
   ├── systems/
   ├── rendering/
   ├── input/
   └── (保留现有文件)

2. 创建对应的.hpp/.cpp文件占位：
   ├── systems/
   │   ├── PlayerMovementSystem.hpp/cpp
   │   ├── CropGrowthSystem.hpp/cpp
   │   ├── NpcScheduleSystem.hpp/cpp
   │   └── SpiritBeastSystem.hpp/cpp
   ├── rendering/
   │   ├── HudRenderer.hpp/cpp
   │   └── WorldRenderer.hpp/cpp
   └── input/
       └── InputHandler.hpp/cpp

3. 更新 CMakeLists.txt：
   - 添加新的源文件组
   - 更新 source_group
   - 确保编译通过（空实现即可）
```

#### 验证标准

```bash
# 编译通过
cmake --build build --config Release

# 无新增警告
# 目录结构符合预期
ls -R src/engine/systems src/engine/rendering src/engine/input
```

#### 注意事项

- 先创建空文件确保编译链路通畅
- 不要一次性移动代码，采用渐进式迁移
- 每个新文件先添加最小注释模板

---

### Task 1.2：提取 PlayerMovementSystem

**优先级**：🔥🔥🔥 P0  
**预估工时**：1天  
**依赖**：Task 1.1  
**涉及文件**：
- 新建：`src/engine/systems/PlayerMovementSystem.hpp/.cpp`
- 修改：`src/engine/GameApp.cpp`（删除对应代码）

#### 提取范围（从GameApp.cpp提取）

```cpp
// 从 Update() 中提取（688-704行）：
// - 玩家输入读取（WASD）
// - 移动向量归一化
// - 调用 player.SetMovementState()
// - 调用 stamina.Consume() / stamina.Recover()
// - 调用 player.Move()

// 实现示例：
class PlayerMovementSystem {
public:
    void Update(
        GameAppState& state,
        float delta_seconds,
        const PlayerConfigParams& params
    );

private:
    sf::Vector2f ReadInput_();
    void ApplyMovement_(
        Player& player,
        StaminaSystem& stamina,
        const sf::Vector2f& movement,
        float delta_seconds,
        const PlayerConfigParams& params
    );
};
```

#### 接口设计

```cpp
#pragma once
#include "CloudSeamanor/GameAppState.hpp"

namespace CloudSeamanor::engine {

class PlayerMovementSystem {
public:
    // ========================================================================
    // 【Update】更新玩家移动状态
    // ========================================================================
    // @param state 全局状态引用
    // @param delta_seconds 帧间隔时间
    // @param params 玩家配置参数（速度、体力消耗等）
    //
    // 职责：
    //   1. 读取WASD输入
    //   2. 归一化方向向量
    //   3. 更新玩家朝向和移动状态
    //   4. 应用位移（含碰撞检测）
    //   5. 处理体力消耗/恢复
    void Update(
        GameAppState& state,
        float delta_seconds
    );
};

} // namespace CloudSeamanor::engine
```

#### 迁移清单

从 `GameApp::Update()` 删除以下代码块：

```cpp
// === 删除开始 ===
// ── 玩家移动 ────────────────────────────────────────────────────
sf::Vector2f movement(0.0f, 0.0f);
if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) movement.y -= 1.0f;
if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) movement.y += 1.0f;
if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) movement.x -= 1.0f;
if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) movement.x += 1.0f;

const bool is_moving = movement.x != 0.0f || movement.y != 0.0f;
if (is_moving) {
    const float len = std::sqrt(movement.x * movement.x + movement.y * movement.y);
    movement /= len;
    state_.player.SetMovementState(movement, true);
    state_.stamina.Consume(state_.player_params.stamina_move_per_second * delta_seconds);
    movement *= state_.player_params.player_speed * delta_seconds;
} else {
    state_.player.SetMovementState(movement, false);
    state_.stamina.Recover(state_.player_params.stamina_recover_per_second * delta_seconds);
}
// ... 以及后续的 Move() 调用
// === 删除结束 ===

// 替换为：
player_movement_.Update(state_, delta_seconds);
```

#### 验证标准

```bash
# 1. 编译通过
# 2. 游戏正常运行，WASD移动功能正常
# 3. 碰撞检测仍然有效
# 4. 体力消耗/恢复正常工作
# 5. GameApp.cpp减少约20行
```

---

### Task 1.3：提取 CropGrowthSystem

**优先级**：🔥🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**涉及文件**：
- 新建：`src/engine/systems/CropGrowthSystem.hpp/.cpp`
- 修改：`src/engine/GameApp.cpp`

#### 提取范围（788-807行）

```cpp
// 从 Update() 中提取：
// - 作物生长逻辑
// - 生长阶段判断
// - 成熟检测
// - 提示消息生成

class CropGrowthSystem {
public:
    void Update(
        std::vector<TeaPlot>& tea_plots,
        float delta_seconds,
        CloudState cloud_state,
        const std::function<void(const std::string&, float)>& push_hint
    );

private:
    static float GetGrowthMultiplier_(CloudState state);
    static int CalculateNextStage_(float growth);
};
```

#### 验证标准

- 作物正常生长
- 天气倍率生效
- 阶段切换提示正常显示
- GameApp.cpp减少约20行

---

### Task 1.4：提取 NpcScheduleSystem

**优先级**：🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**涉及文件**：
- 新建：`src/engine/systems/NpcScheduleSystem.hpp/.cpp`
- 修改：`src/engine/GameApp.cpp`

#### 提取范围（826-851行）

```cpp
// NPC日程更新逻辑：
// - 日程匹配
// - 位置插值
// - 颜色渐变（基于好感度）
```

#### 验证标准

- NPC按日程移动
- 位置平滑过渡
- 好感度影响透明度

---

### Task 1.5：提取 SpiritBeastSystem

**优先级**：🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**涉及文件**：
- 新建：`src/engine/systems/SpiritBeastSystem.hpp/.cpp`
- 修改：`src/engine/GameApp.cpp`

#### 提取范围（854-895行）

```cpp
// 灵兽AI行为：
// - 状态机（Follow/Wander/Idle/Interact）
// - 跟随距离判定
// - 巡逻点导航
// - 空闲计时
```

#### 验证标准

- 灵兽跟随行为正常
- 巡逻逻辑正确
- 交互状态切换流畅

---

### Task 1.6：集成 Catch2 测试框架

**优先级**：🔥🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：无  
**涉及文件**：
- 修改：`CMakeLists.txt`
- 新建：`tests/catch2_main.cpp`
- 删除/修改：`tests/TestFramework.hpp`（可选保留）

#### 实施步骤

```bash
# 1. 下载 Catch2 v3.x（单头文件版本）
# 方式A：直接下载单头文件
curl -O https://github.com/catchorg/Catch2/releases/download/v3.5.2/catch2.hpp
mv catch2.hpp tests/

# 方式B：使用FetchContent（CMake 3.14+）
# 在CMakeLists.txt中添加：
include(FetchContent)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(catch2)

# 2. 创建测试入口
# tests/catch2_main.cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

// 如果需要其他Catch2组件：
// #define CATCH_CONFIG_ENABLE_BENCHMARKING
// #include <catch2/benchmark/catch_benchmark.hpp>
```

#### CMakeLists.txt 更新

```cmake
# 在 CMakeLists.txt 末尾添加：
option(BUILD_TESTING "Build tests" ON)

if(BUILD_TESTING)
    enable_testing()

    # 测试可执行目标
    add_executable(CloudSeamanor_tests
        tests/catch2_main.cpp
        tests/domain/PlayerTest.cpp
        tests/domain/InventoryTest.cpp
        # ... 其他测试文件
    )

    target_link_libraries(CloudSeamanor_tests PRIVATE
        CloudSeamanor_core
        Catch2::Catch2WithMain
    )

    include(CTest)
    # 注册测试
    add_test(NAME UnitTests COMMAND CloudSeamanor_tests)
endif()
```

#### 验证标准

```bash
# 构建测试
cmake --build build --target CloudSeamanor_tests

# 运行测试（应该输出0 tests, 0 failures）
./build/CloudSeamanor_tests

# 输出示例：
# ============================================
# CloudSeamanor Test Suite (using Catch2)
# ============================================
# 0 test cases - 0 assertions - 0 failures
```

---

### Task 1.7：编写 Player 单元测试

**优先级**：🔥🔥🔥 P0  
**预估工时**：1天  
**依赖**：Task 1.6  
**涉及文件**：
- 新建：`tests/domain/PlayerTest.cpp`
- 测试对象：`CloudSeamanor::domain::Player`

#### 测试用例清单（15个）

| ID | 测试名称 | 验证点 | 优先级 |
|----|---------|--------|--------|
| P-01 | 默认构造 | 位置(620,340)、朝向Down | P0 |
| P-02 | SetPosition | 位置正确更新 | P0 |
| P-03 | GetPosition | 返回值准确 | P0 |
| P-04 | Bounds | 返回正确的AABB | P0 |
| P-05 | 无障碍移动 | 位置按delta变化 | P0 |
| P-06 | X轴碰撞 | 右侧受阻，X坐标被限制 | P0 |
| P-07 | Y轴碰撞 | 下方受阻，Y坐标被限制 | P0 |
| P-08 | 边界钳制 | 不超出world_bounds | P0 |
| P-09 | 朝向-右上 | UpRight | P0 |
| P-10 | 朝向-左下 | DownLeft | P0 |
| P-11 | 朝向-纯方向 | Up/Down/Left/Right | P1 |
| P-12 | 零向量朝向 | 返回默认Down | P1 |
| P-13 | FacingText | 返回中文描述 | P2 |
| P-14 | SavePosition | 格式"x,y" | P2 |
| P-15 | LoadPosition | 正确恢复位置 | P1 |

#### 代码示例

```cpp
// tests/domain/PlayerTest.cpp
#include <catch2/catch_test_macros.hpp>
#include "CloudSeamanor/Player.hpp"

using namespace CloudSeamanor::domain;

TEST_CASE("Player default construction") {
    GIVEN("A default-constructed Player") {
        Player player;

        THEN("Position should be (640, 360)") {
            auto pos = player.GetPosition();
            REQUIRE(pos.x == 620.0f);
            REQUIRE(pos.y == 340.0f);
        }

        AND_THEN("Facing direction should be Down") {
            REQUIRE(player.Facing() == FacingDirection::Down);
        }

        AND_THEN("Should not be moving") {
            REQUIRE(player.IsMoving() == false);
        }
    }
}

TEST_CASE("Player movement without obstacles") {
    GIVEN("A player at origin with no obstacles") {
        Player player;
        player.SetPosition({100, 100});
        
        sf::FloatRect bounds{{0, 0}, {800, 600}};
        std::vector<sf::FloatRect> obstacles;

        WHEN("Moving right by 50 units") {
            player.Move({50, 0}, bounds, obstacles);

            THEN("Position should update to (150, 100)") {
                REQUIRE(player.GetPosition().x == 150.0f);
                REQUIRE(player.GetPosition().y == 100.0f);
            }
        }
    }
}

TEST_CASE("Player collision detection - X axis") {
    GIVEN("A player near an obstacle on the right") {
        Player player;
        player.SetPosition({100, 100});

        std::vector<sf::FloatRect> obstacles{
            {{150, 80}, {50, 50}}  // Obstacle at x=150
        };
        sf::FloatRect bounds{{0, 0}, {800, 600}};

        WHEN("Trying to move into the obstacle") {
            player.Move({60, 0}, bounds, obstacles);

            THEN("Player should be stopped at obstacle edge") {
                REQUIRE(player.GetPosition().x == 140.0f);  // 150 - 10 (player width)
            }
        }
    }
}

TEST_CASE("Player boundary clamping") {
    GIVEN("A player near the right-bottom corner") {
        Player player;
        player.SetPosition({750, 550});  // Near edge of 800x600 world
        
        sf::FloatRect bounds{{0, 0}, {800, 600}};
        std::vector<sf::FloatRect> obstacles;

        WHEN("Moving beyond boundaries") {
            player.Move({100, 100}, bounds, obstacles);

            THEN("Player should be clamped within bounds") {
                REQUIRE(player.GetPosition().x <= 800 - 36);  // world_width - player_width
                REQUIRE(player.GetPosition().y <= 600 - 36);
            }
        }
    }
}

TEST_CASE("Player facing directions") {
    Player player;

    SECTION("Up-right diagonal") {
        player.SetMovementState({1, -1}, true);
        REQUIRE(player.Facing() == FacingDirection::UpRight);
    }

    SECTION("Pure left") {
        player.SetMovementState({-1, 0}, true);
        REQUIRE(player.Facing() == FacingDirection::Left);
    }

    SECTION("Zero vector keeps previous facing") {
        player.SetMovementState({1, 0}, true);  // Face right
        player.SetMovementState({0, 0}, false);  // Stop
        REQUIRE(player.Facing() == FacingDirection::Right);  // Unchanged
    }
}
```

#### 验证标准

```bash
# 运行Player测试
./build/CloudSeamanor_tests "[Player]"

# 预期输出：
# =======================================================================
# All tests passed (15 assertions in 5 test cases)
```

---

### Task 1.8：编写 Inventory 单元测试

**优先级**：🔥🔥🔥 P0  
**预估工时**：1天  
**依赖**：Task 1.6  
**涉及文件**：
- 新建：`tests/domain/InventoryTest.cpp`
- 测试对象：`CloudSeamanor::domain::Inventory`

#### 测试用例清单（20个）

| ID | 测试名称 | 验证点 | 优先级 |
|----|---------|--------|--------|
| I-01 | AddItem基础 | 数量正确累加 | P0 |
| I-02 | AddItem堆叠 | 同一物品槽位合并 | P0 |
| I-03 | AddItem新建槽位 | 不同物品独立槽位 | P0 |
| I-04 | RemoveItem成功 | 数量减少 | P0 |
| I-05 | RemoveItem清空槽位 | count=0时删除槽位 | P0 |
| I-06 | RemoveItem不足 | 返回false，不修改 | P0 |
| I-07 | RemoveItem不存在 | 返回false | P1 |
| I-08 | HasItems单物品 | 数量足够返回true | P0 |
| I-09 | HasItems多物品 | 任一不足返回false | P0 |
| I-10 | HasItems空背包 | 返回false | P1 |
| I-11 | TotalItemCount | 总数计算正确 | P0 |
| I-12 | ToMap/FromMap往返 | 序列化一致性 | P0 |
| I-13 | Clear清空 | TotalItemCount=0 | P1 |
| I-14 | SummaryText格式 | 包含物品名和数量 | P2 |
| I-15 | 边界-count<=0 | 忽略无效输入 | P0 |
| I-16 | 边界-empty item_id | 忽略无效输入 | P0 |
| I-17 | 边界-大量物品 | 性能不退化 | P2 |
| I-18 | RemoveItems原子性 | 部分失败全部回滚 | P0 |
| I-19 | AddItems批量 | 多个物品一次添加 | P1 |
| I-20 | CountOf不存在物品 | 返回0 | P1 |

#### 代码示例（精选关键测试）

```cpp
TEST_CASE("Inventory atomic batch removal") {
    GIVEN("An inventory with Wood:5 and Stone:3") {
        Inventory inv;
        inv.AddItem("Wood", 5);
        inv.AddItem("Stone", 3);

        WHEN("Attempting to remove more Wood than available atomically") {
            bool result = inv.RemoveItems({
                {"Wood", 10},  // Not enough!
                {"Stone", 2}   // This would succeed individually
            });

            THEN("Operation should fail completely") {
                REQUIRE(result == false);
            }

            AND_THEN("Inventory should remain unchanged") {
                REQUIRE(inv.CountOf("Wood") == 5);
                REQUIRE(inv.CountOf("Stone") == 3);
            }
        }
    }
}

TEST_CASE("Inventory serialization roundtrip") {
    GIVEN("An inventory with multiple items") {
        Inventory original;
        original.AddItem("TeaSeed", 4);
        original.AddItem("Wood", 5);
        original.AddItem("TurnipSeed", 2);

        WHEN("Serializing to map and deserializing") {
            auto map = original.ToMap();

            Inventory restored;
            restored.FromMap(map);

            THEN("Restored inventory should match original") {
                REQUIRE(restored.CountOf("TeaSeed") == 4);
                REQUIRE(restored.CountOf("Wood") == 5);
                REQUIRE(restored.CountOf("TurnipSeed") == 2);
                REQUIRE(restored.TotalItemCount() == original.TotalItemCount());
            }
        }
    }
}
```

#### 验证标准

```bash
# 运行Inventory测试
./build/CloudSeamanor_tests "[Inventory]"

# 预期：20 assertions, 0 failures
```

---

## 三、Week 2：继续拆分 + 核心测试补齐

### Week 2 目标

完成剩余系统的提取，补齐GameClock和Stamina测试，建立CI/CD流水线。

---

### Task 2.1：提取 WorkshopSystemRuntime

**优先级**：🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**提取范围**：GameApp.cpp 810-823行（工坊加工逻辑）

---

### Task 2.2：提取 PickupSystem

**优先级**：🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**提取范围**：GameApp.cpp 769-785行（掉落物拾取+动画）

---

### Task 2.3：提取 TutorialSystem

**优先级**：🔥 P1  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**提取范围**：GameApp.cpp 727-743行（教程引导逻辑）

---

### Task 2.4：提取 HudRenderer

**优先级**：🔥🔥 P0  
**预估工时**：1天  
**依赖**：Task 1.1  
**提取范围**：
- UpdateHudTextFn() 匿名函数（115-163行）
- RefreshWindowTitle()（989-1026行）

**注意**：这个函数参数很多（20+），需要定义上下文结构体。

---

### Task 2.5：提取 WorldRenderer

**优先级**：🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**提取范围**：Render() 函数（938-984行）

---

### Task 2.6：提取 UiPanelInitializer

**优先级**：🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.1  
**提取范围**：Run() 中的UI初始化部分（296-396行，约100行）

**额外收益**：为后续配置化改造做准备。

---

### Task 2.7：提取 InputHandler

**优先级**：🔥🔥 P0  
**预估工时**：1天  
**依赖**：Task 1.1  
**提取范围**：ProcessEvents() 函数（480-635行）

**挑战**：
- 大量lambda捕获state_
- E键和G键交互代码重复
- 需要重构PlayerInteractRuntimeContext构建

**优化建议**：
```cpp
// 将重复的Context构建提取为工厂方法
PlayerInteractRuntimeContext BuildInteractionContext(GameAppState& s) {
    return PlayerInteractRuntimeContext(
        s.inventory, s.stamina, /* ... 18个参数 ... */
    );
}
```

---

### Task 2.8：编写 GameClock 测试（25个case）

**优先级**：🔥🔥🔥 P0  
**预估工时**：1天  
**依赖**：Task 1.6  

**重点测试场景**：
- 时间推进与Tick
- 日跨越（23:59 → 00:00）
- 年跨越（Day 112 → Day 1）
- 季节计算（每28天一季）
- 时段划分（Morning/Afternoon/Evening/Night）
- SleepToNextMorning
- TimeText/DateText格式化
- 边界值（负时间、超大时间）

---

### Task 2.9：编写 Stamina 测试（15个case）

**优先级**：🔥🔥🔥 P0  
**预估工时**：0.5天  
**依赖**：Task 1.6  

**重点测试场景**：
- Consume/Recover基本操作
- 保护期倍率（0.7x消耗）
- 超出上限/下限的钳制
- SetMax对Current的影响
- CanAfford检查
- 不同保护天数的行为差异

---

### Task 2.10：配置 CI/CD 自动测试

**优先级**：🔥🔥 P1  
**预估工时**：0.5天  
**依赖**：Task 1.6, Task 1.7, Task 1.8, Task 2.8, Task 2.9  

**实施方案**（以GitHub Actions为例）：

```yaml
# .github/workflows/ci.yml
name: CI

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  build-and-test:
    runs-on: windows-latest

    steps:
      - uses: actions/checkout@v4

      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

      - name: Build
        run: cmake --build build --config Release

      - name: Run Tests
        run: ./build/CloudSeamanor_tests

      - name: Static Analysis (clang-tidy)
        run: |
          cmake --build build --target clang-tidy-check || true
```

**验收标准**：
- Push到main分支自动触发测试
- PR必须通过测试才能合并
- 测试结果可视化展示

---

## 四、Week 3：配置化 + 错误处理增强

### Week 3 目标

消除硬编码，增强健壮性。

---

### Task 3.1：创建 UI 配置文件

**优先级**：🔥🔥 P0  
**预估工时**：0.5天  
**产出物**：`configs/ui_layout.json`

**配置内容**：
```json
{
    "panels": {
        "main": { "position": [824, 18], "size": [430, 178], ... },
        "inventory": { "position": [824, 208], "size": [430, 172], ... },
        "dialogue": { "position": [824, 392], "size": [430, 126], ... },
        "hint": { "position": [40, 648], "size": [1214, 48], ... }
    },
    "text_styles": {
        "hud": { "font_size": 17, "color": "#EFF7FF" },
        "inventory_text": { "font_size": 17, "color": "#F5ECD6" },
        ...
    },
    "colors": {
        "stamina_bar_fill": "#59D68A",
        "roaster_progress_fill": "#F2BC68",
        ...
    }
}
```

---

### Task 3.2-3.4：UI配置化迁移

将Task 2.6中提取的UiPanelInitializer改为从JSON加载。

---

### Task 3.5-3.8：错误处理增强

定义Result类型，为关键函数添加错误返回值。

---

## 五、Week 4：Domain层净化 + 性能优化

### Week 4 目标

提升架构纯净度，优化性能热点。

---

### Task 4.1-4.3：Domain层SFML依赖抽离

将视觉状态从domain对象移到engine层Component。

**方案**：采用Component模式（参见审核报告问题4）。

---

### Task 4.4-4.7：性能优化

- vector预分配
- 粒子系统优化
- Tracy Profiler集成

---

## 六、风险与注意事项

### 6.1 技术风险

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| 拆分后引入回归Bug | 中 | 高 | 每步都运行完整游戏验证 |
| 测试覆盖不全 | 中 | 中 | 优先测核心路径，逐步扩展 |
| Catch2集成问题 | 低 | 中 | 使用单头文件版本最简单 |
| CI/CD配置复杂 | 低 | 低 | 先用手动触发，再自动化 |

### 6.2 最佳实践提醒

✅ **每次只改一个模块**：不要同时拆分多个系统  
✅ **小步提交**：每个Task完成后立即commit  
✅ **保持游戏可运行**：任何时刻都应该能编译运行  
✅ **先测试再重构**：为新模块写好测试再移动代码  
✅ **文档同步**：修改CODEBASE_GUIDE.md反映新的文件结构  

### 6.3 回滚策略

如果某个Task导致严重问题：

```bash
# 使用Git回滚到上一个稳定版本
git revert HEAD

# 或回到特定commit
git checkout <stable-commit-hash>

# 重新开始该Task，更小步地迁移
```

---

## 七、交付物清单

### 7.1 代码交付物

- [ ] 10+个新的系统模块文件（systems/rendering/input）
- [ ] 75+个单元测试用例（Player/Inventory/Clock/Stamina）
- [ ] Catch2测试框架集成
- [ ] CI/CD流水线配置
- [ ] ui_layout.json配置文件
- [ ] Result<T>错误处理类型
- [ ] 更新后的CMakeLists.txt

### 7.2 文档交付物

- [ ] 本任务卡（本文档）
- [ ] 更新后的CODEBASE_GUIDE.md
- [ ] API变更说明（如有）
- [ ] 测试覆盖率报告

### 7.3 质量指标

| 指标 | 当前值 | 目标值 | 验证方式 |
|------|--------|--------|----------|
| GameApp.cpp行数 | 1068 | ≤200 | `wc -l GameApp.cpp` |
| 测试用例数量 | ~10 | ≥126 | `ctest -N` |
| 核心模块覆盖率 | ~5% | ≥70% | `--coverage` flag |
| 硬编码UI参数 | 100% | 0% | 代码审查 |
| CI/CD状态 | ❌ 无 | ✅ 自动运行 | GitHub Actions |

---

## 八、后续规划

### Phase 2（Month 2）：功能完善

- 资源管理系统
- 存档升级（多槽位+校验）
- 音频系统集成
- 更多作物和物品

### Phase 3（Month 3）：内容扩展

- 对话树系统
- 任务/剧情系统
- 商店经济循环
- UI美术升级

---

## 附录：快速参考

### A. 文件命名规范

```
src/engine/systems/XxxSystem.hpp     # 运行时系统
src/engine/rendering/XxxRenderer.hpp  # 渲染模块
src/engine/input/XxxHandler.hpp       # 输入处理
tests/domain/XxxTest.cpp              # 领域层测试
tests/engine/XxxTest.cpp              # 引擎层测试
```

### B. Git提交规范

```
refactor(engine): extract PlayerMovementSystem from GameApp
test(domain): add Player collision detection tests
feat(infra): add JSON-based UI layout config
fix(stamina): correct protection period modifier calculation
docs(guide): update CODEBASE_GUIDE.md with new structure
```

### C. 有用的命令

```bash
# 统计代码行数
find src -name "*.cpp" -o -name "*.hpp" | xargs wc -l

# 运行特定测试
./CloudSeamanor_tests "[Player]"  # 只运行Player相关测试

# 查看测试覆盖率（需要--coverage编译选项）
llvm-cov report ./CloudSeamanor_tests

# 格式化所有代码
find src tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

---

**文档版本**：v1.0  
**创建日期**：2026-04-18  
**预计完成日期**：2026-05-16（4周后）  
**负责人**：待分配  
**审核人**：待指定
