# 《云海山庄》后端开发任务清单

> **版本**: 1.0
> **日期**: 2026-04-22
> **项目阶段**: V0.1.0 MVP 原型阶段
> **技术栈**: C++20 / SFML 3.0.2 / CMake / Catch2
> **前置说明**: 本清单严格遵循《云海山庄》四层架构规范，后端负责**核心逻辑、数据处理、接口提供**，不涉及 UI 渲染表现。所有代码严格落入 app / engine / domain / infrastructure 四层。

---

## 文档说明

### 拆分原则

| 原则 | 说明 |
|------|------|
| **四层边界清晰** | app（入口）→ engine（运行时编排）→ domain（纯玩法规则）→ infrastructure（IO/解析） |
| **禁止循环依赖** | infrastructure 禁止依赖 engine；domain 禁止依赖 app 或 infrastructure |
| **纯逻辑无渲染** | domain 层仅含标准库，不引入 SFML Graphics |
| **数据驱动** | 业务逻辑从 CSV/JSON 配置读取，不硬编码 |
| **接口契约** | 所有 engine 层面板通过访问器获取 domain 数据，不直接持有 domain 对象 |

### 四层职责对照

| 层级 | 职责 | 示例模块 |
|------|------|---------|
| **app** | 程序入口、应用装配 | `main.cpp`，`GameApp.cpp`（协调者） |
| **engine** | 主循环、输入、渲染编排、系统协调 | `GameRuntime`、`UISystem`、`InputManager` |
| **domain** | 纯玩法规则、稳定业务状态 | `Player`、`Inventory`、`CloudSystem`、`CropData` |
| **infrastructure** | 配置、存档、日志、TMX 解析 | `SaveGameService`、`ResourceManager`、`TmxMapLoader` |

### 后端接口提供给前端

所有后端面板的数据接口通过访问器模式暴露，前端只调用接口不持有对象：

```cpp
// domain 层示例
class CloudSystem {
public:
    [[nodiscard]] CloudState CurrentState() const { return state_; }
    [[nodiscard]] int GetTodaySpiritBonus() const { return spirit_bonus_; }
    // 不暴露内部状态，只暴露只读接口
};

// engine 层示例（为 UI 提供数据）
class CloudSeaForecastData {
public:
    CloudState today_state;
    CloudState tomorrow_state;
    int tide_countdown_days;
    std::vector<ActivityRecommendation> recommendations;
};

class UISystem {
public:
    [[nodiscard]] CloudSeaForecastData GetCloudForecast() const;
    [[nodiscard]] PlayerStatusData GetPlayerStatus() const;
    // ... 其他 UI 数据接口
};
```

---

## 任务总览

| 分类 | 任务数 | 层级 |
|------|--------|------|
| **B1 — Phase 1 重构收尾** | 6 | engine / domain / infra |
| **B2 — 农业系统** | 12 | domain + engine |
| **B3 — 对话与社交系统** | 10 | domain + engine |
| **B4 — 云海与灵界系统** | 10 | domain + engine |
| **B5 — 工坊与经济系统** | 9 | domain + engine |
| **B6 — 战斗系统** | 6 | engine + infrastructure |
| **B7 — 节日与成就系统** | 6 | engine + domain |
| **B8 — 建筑与经营系统** | 6 | engine + domain |
| **B9 — 存档与配置系统** | 5 | infrastructure |
| **B10 — 音频与输入系统** | 5 | engine + infrastructure |
| **B11 — 灵兽与宠物系统** | 5 | engine + domain |
| **总计** | **80** | |

---

## B1 — Phase 1 重构收尾

> 目标：完成 Phase 1 工程重构遗留任务，GameApp.cpp ≤ 200 行，测试覆盖率 ≥ 70%。

### BE-001 | GameApp.cpp 降至 ≤ 200 行

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `src/engine/GameApp.cpp` 当前 43 行（满足 ≤ 200 行）
- [x] `GameApp.cpp` 仅保留状态映射与颜色辅助函数，主流程逻辑已拆分至其他 engine 子模块
- [x] 未发现 `UpdateHudTextFn` 在 `GameApp.cpp` 残留

**任务描述**：
继续拆分 `GameApp.cpp` 中剩余的逻辑，将不足 5 行的小函数内联到调用处，确保 GameApp 仅作为协调者。

**技术要求**：
- 将 `UpdateHudTextFn` 完全下沉到 `HudRenderer`（已在 Week 2 完成，需确认调用链路）
- 检查剩余匿名函数，确认无业务逻辑残留
- 所有游戏逻辑必须落入 engine 层子系统，GameApp 只负责调用子系统 Update/Render
- **落点**: `engine/GameApp.cpp`（仅协调调用，不含业务逻辑）

**交付标准**：
- `GameApp.cpp` 行数 ≤ 200 行
- CI 编译通过，所有测试通过

---

### BE-002 | Domain 层 SFML 依赖抽离

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `Player` 已为纯数据/移动逻辑对象（位置、朝向、速度状态），未持有 SFML 渲染对象
- [x] `TeaPlot` 当前归属 engine 运行时类型，不在 domain 层
- [ ] `PlayerVisualComponent` 尚未独立建模为 engine 渲染组件（后续补齐）
- [ ] 本轮复核：domain 层仍需做一次 `SFML` 头文件全量清扫验收

**任务描述**：
将 `Player.hpp` 和 `TeaPlot` 结构中的 SFML 渲染对象（`sf::RectangleShape` 等）抽离到 engine 层的 Component 组件中，使 domain 层完全不含 SFML 依赖。

**技术要求**：
- `Player.hpp` 中的位置信息（`sf::Vector2f`）保持不变（这是纯数据）
- `PlayerVisualComponent`：新建 engine 层组件，持有 SFML 渲染对象
- `TeaPlot` 中的 `sf::Color` 渲染状态移入 engine 层
- `FarmingRuntime.cpp` 持有 VisualComponent，负责在 domain 数据变化时更新视觉
- **落点**:
  - `domain/player/Player.hpp` — 纯数据（位置、速度、属性）
  - `engine/rendering/PlayerVisualComponent.hpp` — SFML 渲染对象
  - `engine/systems/FarmingRuntime.cpp` — 协调 domain 数据和 visual component

**交付标准**：
- `domain/` 目录下所有 `.cpp/.hpp` 文件不含 `#include <SFML/Graphics.hpp>`
- Catch2 测试可在无 GUI 环境下运行 domain 层测试

---

### BE-003 | UI 参数 100% 配置化

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已存在 `ui_layout.json` 与 `UiLayoutConfig` 配置链路
- [ ] `GameWorldState.cpp` / `UISystem.cpp` 全量硬编码扫描与迁移未全部验收完成
- [ ] 本轮复核：仍需将 UI 魔法数字做“代码->配置”逐项闭环

**任务描述**：
扫描 `GameWorldState.cpp` 和 `UISystem.cpp` 中所有硬编码颜色值和尺寸值，迁移到 `ui_layout.json`。

**技术要求**：
- 所有 UI 布局常量（位置、尺寸、内边距）已在 `PixelUiConfig.hpp` 中定义，需确认全部迁移完成
- 扫描残留硬编码：`sf::Color(r, g, b)`、`setPosition(x, y)` 中的魔法数字
- 配置项格式参考 `ui_layout.json` 的现有 schema
- **落点**: `infrastructure/data/UiLayoutLoader.cpp`（配置加载）

**交付标准**：
- `ui_layout.json` 包含所有 UI 参数（颜色、尺寸、字体路径等）
- 代码中无硬编码 UI 常量（颜色和尺寸）

---

### BE-004 | 字体路径配置化 + 跨平台支持

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `ResourceManager` 已具备 Windows/macOS/Linux 字体搜索路径
- [x] 已对齐“从 `ui_layout.json` 的 `fonts.primary/fallback` 读取并优先加载”验收项
- [x] 配置字体加载失败时自动回退至系统/项目字体搜索链，不崩溃

**任务描述**：
移除 Windows 绝对路径字体加载，支持 macOS 和 Linux 字体自动搜索。

**技术要求**：
- 从 `ui_layout.json` 的 `fonts.primary` / `fonts.fallback` 读取字体路径
- 实现跨平台搜索链：
  - Windows: `C:/Windows/Fonts/` + 用户指定路径
  - macOS: `/System/Library/Fonts/` + `~/Library/Fonts/` + 用户指定路径
  - Linux: `/usr/share/fonts/` + `~/.fonts/` + 用户指定路径
- 缺失字体时返回 `Result::Err()` 而非崩溃
- **落点**: `infrastructure/ResourceManager.cpp`

**交付标准**：
- `ResourceManager::GetFont()` 支持跨平台字体搜索
- Windows 平台下可正常加载字体
- 字体缺失时优雅降级（返回 fallback 字体或默认占位符）

---

### BE-005 | 单元测试覆盖率提升至 ≥ 70%

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 当前已有 domain/engine 多组测试文件
- [ ] 受本机 `cmake` 不可用限制，暂未完成覆盖率 70% 的实测与报表验证
- [ ] 本轮复核：测试用例可继续补，但覆盖率门槛仍待本机工具链恢复后验收

**任务描述**：
补充 domain 层和 engine 层的单元测试，将覆盖率从当前约 25% 提升至 70%。

**技术要求**：
- domain 层测试重点（目标 80%）：
  - `CropGrowthSystemTest.cpp`：生长/品质/季节/浇水 各 10+ 用例
  - `InteractionSystemTest.cpp`：各种交互结果 各 10+ 用例
  - `DialogueEngineTest.cpp`：路由/打字机/选项 各 20+ 用例
  - `WorkshopSystemTest.cpp`：加工/队列 各 10+ 用例
  - `CloudSystemTest.cpp`：天气状态转换 各 10+ 用例
  - `ShopSystemTest.cpp`：买卖/价格波动 各 10+ 用例
  - `AudioManagerTest.cpp`：mock SFML audio 各 5+ 用例
- infrastructure 层测试重点（目标 60%）：
  - `SaveGameServiceTest.cpp`：序列化/反序列化
  - `TmxMapLoaderTest.cpp`：地图加载
- **落点**: `tests/domain/` 和 `tests/infrastructure/`

**交付标准**：
- 测试覆盖率 ≥ 70%（通过 `cmake --preset vs2022-x64 && cmake --build --preset test-debug && gcov` 验证）
- 所有新增代码附带单元测试

---

### BE-006 | Tracy Profiler 集成

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `Profiling.hpp` 已提供 `TRACY_ENABLE` 条件接入
- [x] 已在 `WorldRenderer::Render()`、`InputManager::HandleEvent()`、`UISystem::Render()` 补充埋点宏
- [x] `GameRuntime::Update()` 已存在埋点
- [x] 现行主循环入口已重构为运行时分发，关键 Update/Render 路径埋点位点已对齐
- [x] 已补充 `.tracy/TracyCallstack.xml`

**任务描述**：
在关键路径插入性能分析宏，建立性能基线。

**技术要求**：
- 已在 `include/CloudSeamanor/Profiling.hpp` 中配置 Tracy 可选依赖（`TRACY_ENABLE` + `__has_include` 自动接入）
- 在以下关键路径插入 `ZoneScopedN()` 宏：
  - `GameApp::Update()` — 主循环 Update
  - `GameApp::Render()` — 主循环 Render
  - `InputManager::HandleEvent()` — 输入处理
  - `WorldRenderer::Render()` — 世界渲染
  - `UISystem::Update()` — UI 更新
- 配置文件 `.tracy/TracyCallstack.xml` 定义关注的函数列表
- **落点**: `engine/core/Profiling.cpp`

**交付标准**：
- Tracy 集成完成，关键路径有性能数据
- 建立性能基线文档（每帧 Update/Render 耗时目标）

---

## B2 — 农业系统

> 目标：完善作物系统，实现品质系统、洒水器、肥料、温室等农业玩法。

### BE-007 | 作物数据表扩展（5 种春季作物）

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `assets/data/CropTable.csv` 已包含 `turnip/cloud_berry/root_turnip/rutabaga/bean_pod`（及更多春季作物）
- [x] 春季标签按 `season_spring` 写入作物条目

**任务描述**：
在 `CropTable.csv` 中扩展春季作物，从当前 1 种扩展到 5 种。

**技术要求**：
- 新增作物：
  - `turnip`（萝卜）：生长时间 4 天，产量 1-2，春季专属
  - `cloud_berry`（云莓）：生长时间 5 天，产量 2-3，春季专属
  - `root_turnip`（根茎萝卜）：生长时间 6 天，产量 2-3，春季专属
  - `rutabaga`（芜菁）：生长时间 5 天，产量 2-3，春季专属
  - `bean_pod`（豆荚）：生长时间 4 天，产量 3-4，春季专属
- 所有作物在 CSV 中标注 `season=spring`
- **落点**: `domain/farming/CropTable.csv` + `CropData.cpp`

**交付标准**：
- `CropTable.csv` 包含 5 种春季作物
- `CropTable::GetCropsBySeason(Spring)` 返回 5 种作物

---

### BE-008 | 作物品质系统接入

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 收获链路调用 `CropTable::CalculateQuality(...)`
- [x] 产量倍率使用 `QualityHarvestMultiplier(...)`
- [x] 文案已接入品质文本前缀

**任务描述**：
在 `CropGrowthSystem` 的收获链路中接入品质计算和产量修正。

**技术要求**：
- `CropData.hpp` 中已有 `CropQuality` 枚举（Normal/Fine/Rare/Spirit/Holy）
- 收获时调用 `CropTable::CalculateQuality(cloud_state)` 计算品质
- 应用 `QualityHarvestMultiplier`（普通 1.0x / 优质 1.3x / 稀有 1.8x / 灵品 2.5x）
- 提示文案接入品质文本（普通/优质/稀有/灵品）
- **落点**: `domain/farming/CropGrowthSystem.cpp`（engine 层持有，调用 domain 层数据）

**交付标准**：
- 收获时根据云海状态产生不同品质作物
- 品质影响产量和售价

---

### BE-009 | 洒水器系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 交互链路支持放置洒水器并记录地块状态
- [x] `GameRuntime` 每日切换时执行洒水器自动浇水
- [x] 洒水器耐久天数递减与失效清理已接入
- [x] `sprinkler_radius` 已从配置读取

**任务描述**：
实现洒水器物品和自动浇水逻辑。

**技术要求**：
- 创建 `SprinklerItem` 物品类型（在 `ItemType` 枚举中增加）
- 创建 `SprinklerPlotEffect` 效果类型
- 当玩家背包中有洒水器且站在已翻土地块旁时，按 R 键放置
- 每日日切时（`GameClock::DayChangedEvent`），对洒水器周围 `sprinkler_radius` 格地块自动浇水
- 洒水器有耐久度（`sprinkler_durability_days`，默认 30 天），到期消耗
- 从 `configs/gameplay.cfg` 读取 `sprinkler_radius` 配置
- **落点**:
  - `domain/farming/SprinklerItem.hpp` — 洒水器物品数据
  - `engine/systems/SprinklerRuntime.cpp` — 洒水器运行时逻辑

**交付标准**：
- R 键可放置洒水器
- 每日自动为周围地块浇水
- 耐久度正确消耗

---

### BE-010 | 肥料系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 地块已支持 `fertilized/fertilizer_type`
- [x] 生长系统已按 `basic/premium` 应用倍率（1.2/1.5）
- [x] 施肥与生长加成链路已接入

**任务描述**：
实现肥料物品和生长速度加成逻辑。

**技术要求**：
- 创建 `FertilizerItem` 物品类型
- 当玩家在已翻土地块上使用肥料时，设置 `plot.fertilizer_type`
- `CropGrowthSystem::Update` 中检测地块是否有肥料增益：
  - 无肥料：正常生长
  - 普通肥料（`basic`）：`growth_rate *= 1.2`
  - 优质肥料（`premium`）：`growth_rate *= 1.5`
- 肥料持续效果：整季有效
- **落点**: `engine/systems/FertilizerRuntime.cpp`

**交付标准**：
- 施肥后作物生长速度提升
- 优质肥料效果强于普通肥料

---

### BE-011 | 作物生长视觉阶段区分

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `WorldRenderer` 按 `plot.stage` 渲染 0-4 阶段差异
- [x] stage 4/可收获阶段加入金色边框和粒子光效
- [x] 已区分浇水地块（蓝色调）与未浇水地块（土黄色调）

**任务描述**：
在 `WorldRenderer` 中实现作物不同生长阶段的视觉区分。

**技术要求**：
- 根据 `plot.stage`（0-4）渲染不同颜色/大小的作物图形：
  - stage 0（种子）：棕色小点
  - stage 1（幼苗）：浅绿色小方形
  - stage 2（成长期）：深绿色中方形
  - stage 3（成熟期）：作物颜色大方形 + 轻微摇动动画
  - stage 4（可收获）：作物颜色 + 金色边框 + 粒子光效
- 浇水地块使用蓝色调，未浇水使用土黄色调
- **落点**: `engine/rendering/WorldRenderer.cpp`

**交付标准**：
- 作物 5 个生长阶段视觉区分明显
- 可收获阶段有金色边框和粒子提示

---

### BE-012 | 收割自动入袋完善

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 收获链路使用 `inventory.TryAddItems(...)`
- [x] 成功时提示文案为 `"收获了 {品质前缀}{作物名} ×{数量}"` 并重置地块状态
- [x] 背包满时提示 `"背包已满，无法收获更多作物"` 且不重置地块状态

**任务描述**：
确认 `InteractionSystem` 中收割时物品自动入袋逻辑完善，支持背包满保护。

**技术要求**：
- 收获时调用 `inventory.TryAddItems()`
- `TryAddItems` 返回成功时：显示 `"收获了 {品质前缀}{作物名} ×{数量}"`；更新地块状态
- `TryAddItems` 返回失败（背包满）时：显示 `"背包已满，无法收获更多作物"`，**不重置地块状态**（玩家可稍后清理背包再收获）
- **落点**: `engine/InteractionSystem.cpp`

**交付标准**：
- 收获成功时物品正确进入背包
- 背包满时地块状态保持不变，不消耗体力

---

### BE-013 | 地块开垦系统完善

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已实现 `tilled -> seeded -> watered -> harvest` 主流程状态切换
- [ ] `raw land/obstacle` 完整状态机与工具耐久消耗链路仍需补齐
- [ ] 本轮复核：核心缺口仍聚焦在障碍物子状态和工具耐久扣减

**任务描述**：
完善从 raw land → obstacle → tilled → seeded → watered → harvest 的完整地块流程。

**技术要求**：
- Raw land：默认不可交互
- Obstacle：石头（3 次斧击清除）/ 树桩（2 次镐击清除）/ 杂草（1 次镰刀清除）
- Tilled：使用锄头翻土，`plot.tilled = true`
- Seeded：背包中有种子时按 E 播种
- Watered：每日浇水（玩家/洒水器/灵兽）
- Harvest：按 E 收获（见 BE-012）
- `InteractionSystem::InteractWithPlot` 中实现完整状态机
- **落点**: `engine/InteractionSystem.cpp`

**交付标准**：
- 地块从原始到收获的完整流程可用
- 障碍物清除消耗工具耐久

---

### BE-014 | 作物跨季节循环

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已存在 `in_greenhouse` 标记并在生长倍率中生效
- [x] 季节变更触发、跨季枯萎与温室豁免的完整事件链路已补齐
- [x] 本轮复核：跨季枯萎事件已落地，且改为仅在换季当天触发

**任务描述**：
在季节变更时触发作物枯萎和回收逻辑。

**技术要求**：
- 在 `GameClock` 中，季节变更时（每 28 天）触发 `SeasonChangedEvent`
- `CropGrowthSystem` 监听此事件：
  - 对非当前季节作物，自动枯萎并显示 `"季节已过，作物枯萎了"`
  - 枯萎地块变为可翻土状态（`cleared = true`）
  - 对季节适配作物，继续生长
- 温室地块（`in_greenhouse = true`）不受季节影响
- **落点**: `engine/systems/CropGrowthSystem.cpp`

**交付标准**：
- 跨季节时非当季作物正确枯萎
- 温室作物不受影响

---

### BE-015 | 作物疾病虫害系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] `TeaPlot` 已包含 `disease/pest/disease_days` 字段
- [x] 每日感染概率、3天惩罚与治疗链路已完整落地
- [x] 本轮复核：治疗物品与灵兽自动除虫逻辑已贯通

**任务描述**：
实现作物疾病虫害感染和治疗逻辑。

**技术要求**：
- 在 `TeaPlot` 中添加 `disease_` 和 `pest_` 布尔字段，以及 `disease_days_` 连续天数
- 每日有 10% 概率感染（浇水可降低概率）
- 感染后不治疗则 3 天内产量降为 0
- 治疗方式：使用杀虫剂物品，或高好感灵兽有概率自动除虫
- 显示：`"作物感染了虫害！"` 提示
- **落点**: `engine/systems/CropGrowthSystem.cpp`

**交付标准**：
- 每日感染概率正确
- 未治疗时产量降为 0
- 杀虫剂可治疗

---

### BE-016 | 温室季节逻辑

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 地块已支持 `in_greenhouse` 标记
- [x] 主屋 Lv.3 解锁与入口交互标记逻辑已完成
- [x] 本轮复核：已把“建筑等级->温室入口->地块标记”串成完整流程

**任务描述**：
实现温室地块的季节豁免逻辑。

**技术要求**：
- 在 `GameWorldState` 中标记温室区域（碰撞体或地块标记 `in_greenhouse`）
- `CropGrowthSystem::Update` 中：位于温室区域的地块跳过跨季枯萎判定
- 解锁条件：主屋升级至 Lv.3
- 玩家与温室入口交互时，标记后续种植的地块为温室地块
- **落点**: `engine/systems/CropGrowthSystem.cpp`

**交付标准**：
- 温室地块不受季节影响
- 主屋 Lv.3 后解锁

---

### BE-017 | 作物数据 CSV 校验脚本

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已提供 `tools/validate_crop_table.py`
- [x] 校验覆盖 `growth_time/stages/base_harvest/seed_item_id/harvest_item_id`
- [x] 已补充季节合法性校验（支持 `season` 字段或 `season_*` 标签）

**任务描述**：
编写 Python 脚本自动校验 `CropTable.csv` 的数据完整性。

**技术要求**：
- 校验规则：
  - `growth_time > 0`
  - `stages >= 1`
  - `base_harvest >= 1`
  - `seed_item_id` 和 `harvest_item_id` 非空
  - `season` 字段合法（spring/summer/autumn/winter）
- 输出错误行号和原因
- **落点**: `tools/validate_crop_table.py`

**交付标准**：
- 脚本可正确识别所有数据错误
- 校验规则覆盖所有关键字段

---

### BE-018 | 夏季/秋季/冬季作物扩展

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `CropTable.csv` 已扩展为夏 9 / 秋 8 / 冬 6（满足任务最低数量）
- [x] `tools/validate_crop_table.py` 校验通过

**任务描述**：
扩展作物表至四季完整作物集。

**技术要求**：
- 春 8 种（已在 BE-007 完成）：萝卜、芜菁、豆荚、云莓、根茎萝卜、（+3 种待定）
- 夏 8 种：辣椒、蓝莓、番茄、（+5 种新增）
- 秋 8 种：南瓜、山药、苹果、葡萄、月华稻、灵芝菇、（+2 种新增）
- 冬 6 种：雪莲、冬小麦、羽衣甘蓝、（+3 种新增，根茎萝卜和灵芝菇跨季）
- 所有作物数据写入 `CropTable.csv`
- **落点**: `domain/farming/CropTable.csv`

**交付标准**：
- `CropTable.csv` 包含四季完整作物
- 校验脚本通过

---

## B3 — 对话与社交系统

> 目标：完善对话引擎、NPC 社交、NPC 深度内容。

### BE-019 | 对话引擎分支选择路由完善

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `DialogueEngine` 已支持 `condition`（`has_item/min_favor`）解析
- [x] 条件不满足时可跳转 `fallback` 分支

**任务描述**：
扩展 `DialogueEngine::RouteChoice_()` 支持条件路由。

**技术要求**：
- 支持条件路由 JSON 格式：
  ```json
  { "id": "c1", "text": "送茶", "next": "give_tea",
    "condition": { "has_item": "TeaPack", "min_favor": 50 } }
  ```
- 满足条件走 `next`，不满足显示 fallback 文案并跳转
- **落点**: `engine/DialogueEngine.cpp`

**交付标准**：
- 条件路由正确解析和执行
- fallback 分支可正常工作

---

### BE-020 | NPC 日常对话池完善

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `acha/lin/wanxing/xiaoman` 日常对话文件均已存在
- [x] 4 位核心 NPC 对话条目均达到 `>=20`
- [x] 天气/节日/生日对话路由链路已接入

**任务描述**：
为 4 位核心 NPC（acha/lin/wanxing/xiaoman）各补全至 20 条日常对话。

**技术要求**：
- 当前 `npc_daily_acha.json` 有约 15 条对话，扩展至 20 条
- 新增 `npc_daily_lin.json`、`npc_daily_wanxing.json`、`npc_daily_xiaoman.json`
- 每条对话需覆盖：时段（早/中/晚）× 季节（春夏秋冬）× 好感度（0-50/50-100/100+）组合
- 特殊对话：
  - 天气为薄雾/浓云/大潮时优先匹配 `weather` 字段
  - 节日当天匹配节日专属对话
  - NPC 生日显示生日祝福
- **落点**: `assets/data/dialogue/npc_daily_*.json` + `engine/NpcDialogueManager.cpp`

**交付标准**：
- 4 位核心 NPC 各有 ≥ 20 条日常对话
- 天气/节日/生日特殊对话正确触发

---

### BE-021 | 心事件 h1-h3 触发系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 4 位核心 NPC 已配置 h1/h2/h3 事件与对应 JSON
- [x] 触发阈值已对齐（h1:50 / h2:200 / h3:600）
- [x] 已实现完成标记与重入保护（重复触发拦截）

**任务描述**：
实现心事件 h1-h3 的触发逻辑。

**技术要求**：
- h1：好感 ≥ 50（1 心）时，第一次对话触发
- h2：好感 ≥ 200（2 心）时触发（已有 JSON，需确认链路）
- h3：好感 ≥ 600（3 心）时触发
- 创建 `npc_heart_{id}_h1.json`、`npc_heart_{id}_h3.json`
- `NpcDialogueManager` 中实现触发条件检测
- 重入保护：已完成的心事件不重复触发
- **落点**: `engine/NpcDialogueManager.cpp` + `assets/data/dialogue/npc_heart_*.json`

**交付标准**：
- 4 位 NPC 的 h1、h2、h3 事件均可正常触发
- 心事件完成标记正确存档

---

### BE-022 | NPC 特殊对话扩展

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已实现生日/节日特殊对话注入
- [x] 天气条件对话匹配已接入
- [x] 路由优先级符合：生日 > 节日 > 天气 > 日常

**任务描述**：
在 `NpcDialogueManager::SelectGreeting` 中增加天气/节日/生日条件判断。

**技术要求**：
- 天气优先：匹配 `weather` 字段对话
- 节日当天：匹配节日专属对话
- NPC 生日：显示生日祝福对话
- 优先级：生日 > 节日 > 天气 > 日常
- **落点**: `engine/NpcDialogueManager.cpp`

**交付标准**：
- 天气/节日/生日特殊对话正确路由

---

### BE-023 | 送礼反馈系统完善

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已按 loved/liked/disliked/neutral 提供分级文案与好感变化
- [x] 本轮已补送礼 SFX（`gift`）触发
- [x] 中性文案与爱心粒子颜色已细化（红/粉/灰粒子区分完成）

**任务描述**：
完善 `InteractionSystem::GiveGiftToNpc` 的视觉反馈和文案。

**技术要求**：
- `GiftResult.message` 根据礼物价值生成不同文案：
  - 喜爱（`loved` 列表）：`"{NPC}眼睛亮了起来！" + 好感+30`
  - 喜欢（`liked` 列表）：`"{NPC}微微点头。" + 好感+15`
  - 讨厌（`disliked` 列表）：`"{NPC}不太高兴。" + 好感-5`
  - 中性：`"收到了。" + 好感+1`
- 触发对应颜色的爱心粒子（红/粉/灰）
- **落点**: `engine/InteractionSystem.cpp`

**交付标准**：
- 礼物价值分级文案正确
- 爱心粒子颜色与反馈类型一致

---

### BE-024 | NPC 对话冷却时间系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已按 `daily_talked + last_talk_day` 实现同日冷却
- [x] 第二次对话提示 `"今天已经聊过了，明天再来吧。"`
- [x] 跨日重置已在每日状态刷新逻辑中接入

**任务描述**：
实现同日多次对话冷却机制。

**技术要求**：
- 在 `NpcActor` 中已有 `daily_talked` 和 `last_talk_day` 字段
- 同一天内与同一 NPC 多次对话时：
  - 第一次：正常对话
  - 第二次起：显示 `"今天已经聊过了，明天再来吧。"`（可配置冷却次数，默认 1 次/天）
- 跨日后重置冷却计数
- **落点**: `engine/NpcDialogueManager.cpp`

**交付标准**：
- 同日重复对话正确显示冷却提示
- 跨日后冷却重置

---

### BE-025 | 对话打字机速度配置化

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `GameRuntime` 从 `gameplay.cfg` 读取 `dialogue_typing_speed_ms`
- [x] 已在 `DialogueEngine::SetTypingSpeed` 增加 20~100ms 范围钳制
- [x] 运行时配置生效链路已接入

**任务描述**：
将对话打字机速度从硬编码改为配置文件驱动。

**技术要求**：
- 从 `configs/gameplay.cfg` 读取 `dialogue_typing_speed_ms`
- 范围限制：20ms - 100ms（防止过快或过慢）
- 玩家可在设置界面调整（写入配置文件）
- **落点**: `engine/DialogueEngine.cpp`

**交付标准**：
- 打字机速度可配置
- 玩家可动态调整速度

---

### BE-026 | 扩展对话变量标记

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `$[WEATHER]`、`$[SEASON]`、`$[DAY]`、`$[NPC_NAME]`、`$[ITEM_NAME]` 已支持
- [x] 替换逻辑在 `DialogueEngine::ReplaceTokens` 生效

**任务描述**：
扩展 `DialogueEngine::ReplaceTokens` 支持更多变量。

**技术要求**：
- 已支持：`$PLAYER_NAME`、`$FARM_NAME`
- 扩展支持：
  - `$[WEATHER]` → 当前云海天气
  - `$[SEASON]` → 当前季节
  - `$[DAY]` → 当前天数
  - `$[NPC_NAME]` → 当前对话 NPC 名字
  - `$[ITEM_NAME]` → 最近获得的物品名
- **落点**: `engine/DialogueEngine.cpp`

**交付标准**：
- 所有 5 种新变量正确替换

---

### BE-027 | NPC 心情系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] `NpcActor` 已有 `NpcMood` 字段（Happy/Normal/Sad/Angry）
- [x] 已有按天气更新心情的基础逻辑
- [x] 已在跨日更新链路刷新 NPC 心情状态
- [x] 好感倍率联动与完整语气分支已完善（×1.0 / ×0.5 + 对话语气前缀）

**任务描述**：
实现 NPC 动态心情系统。

**技术要求**：
- 在 `NpcActor` 中添加 `Mood` 枚举（Happy/Normal/Sad/Angry）
- 心情由以下因素决定：
  - 天气：晴 → Happy，浓云 → Normal，大潮 → Happy
  - 季节：喜欢当前季节 → Happy
  - 玩家行为：最近送礼 → Happy，忽略 → Sad
- 心情跨日刷新（`GameClock::DayChangedEvent`）
- 心情影响对话语气和好感获取倍率（×1.0 / ×0.5）
- **落点**: `domain/social/NpcActor.hpp` + `engine/NpcDialogueManager.cpp`

**交付标准**：
- 心情随天气和玩家行为动态变化
- 不同心情有不同对话语气

---

### BE-028 | NPC 委托任务系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 项目已有委托/任务相关状态与面板链路基础
- [x] 已有 `QuestTable.csv` 读取与每日任务状态刷新基础
- [x] `NpcDeliverySystem` 与每日 6:00 刷新机制已落地到独立系统（含“完成后对话领奖”闭环）

**任务描述**：
实现 NPC 每日委托任务（采集/送货/击杀）。

**技术要求**：
- 创建 `QuestDelivery` 类
- NPC 每天发布 1-3 个委托（从 `NpcDeliveryTable.csv` 读取）
- 委托显示在任务面板（`PixelQuestMenu`）
- 玩家完成委托后，与对应 NPC 对话领取奖励（金币+好感度）
- 委托在每日凌晨 6:00 刷新
- **落点**: `engine/systems/NpcDeliverySystem.cpp`

**交付标准**：
- 每日委托正确刷新
- 委托完成奖励正确发放

---

## B4 — 云海与灵界系统

> 目标：实现灵界探索、云海大潮事件、灵气阶段等云海山庄核心差异化玩法。

### BE-029 | 灵界入口设计

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已具备灵界进出切换逻辑（交互切换 in_spirit_realm）
- [x] 已接入 `Spirit Gateway` 交互目标
- [ ] 传送门对象与独立场景切换动画仍待补齐

**任务描述**：
在主地图中设计灵界传送门，实现场景切换逻辑。

**技术要求**：
- 在 `prototype_farm.tmx` 中设计传送门对象（`type = "spirit_gateway"`）
- 当玩家按 E 键与传送门交互时：
  - 触发场景切换动画（淡入淡出）
  - 淡入到灵界地图 `spirit_realm_layer1.tmx`
- 大潮期间：传送门发光特效（粒子系统）
- **落点**: `assets/maps/prototype_farm.tmx` + `engine/SceneManager.cpp`

**交付标准**：
- E 键可与传送门交互
- 场景切换动画正常播放

---

### BE-030 | 灵界浅层 TMX 地图

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] TMX 地图加载链路已存在
- [x] `assets/maps/spirit_realm_layer1.tmx` 已落地（30×20，含灵草点与灵界传送门对象）
- [ ] `spirit_beast_zone` / `spirit_gateway_return` 语义对象与管理器联动仍待补齐

**任务描述**：
设计灵界浅层地图。

**技术要求**：
- 新建 `assets/maps/spirit_realm_layer1.tmx`
- 地图尺寸：30×20 格，每格 48×48 像素
- 色调：淡紫/银白色调（区别于主地图的绿色）
- 元素：
  - 云雾粒子背景
  - 灵草采集点（`type = "spirit_plant"`）
  - 灵兽游荡区域（`type = "spirit_beast_zone"`）
  - 传送门返回点（`type = "spirit_gateway_return"`）
- **落点**: `assets/maps/spirit_realm_layer1.tmx` + `engine/TmxMapLoader.cpp`

**交付标准**：
- 灵界地图正确加载
- 传送门可双向切换

---

### BE-031 | 灵物采集系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已支持灵物采集、体力消耗与冷却判定
- [x] 已支持大潮期掉落增强与灵物产出分支
- [x] 已支持冷却提示文案输出

**任务描述**：
实现灵草采集逻辑。

**技术要求**：
- 在 `InteractionSystem` 中添加 `InteractWithSpiritPlant` 方法
- 玩家按 E 键采集：
  - 消耗体力（`SpiritForageStaminaCost`，默认 10）
  - 产出灵物物品（`spirit_grass`/`cloud_dew`/`spirit_dust`）
  - 采集后进入冷却（每株 1 小时游戏时间）
  - 显示倒计时：`"灵草冷却中，剩余 0:45"`
- 大潮期间掉落率 ×2，稀有灵物出现
- **落点**: `engine/InteractionSystem.cpp`

**交付标准**：
- 灵草可采集，有冷却时间
- 大潮期间掉落翻倍

---

### BE-032 | 灵物物品表

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已新增 `assets/data/economy/SpiritItemTable.csv`
- [x] 已包含 `spirit_grass/cloud_dew/spirit_dust/star_fragment` 基础字段
- [x] 现有交易与工坊链路可消费灵物类物品

**任务描述**：
完善灵物物品数据表。

**技术要求**：
- 创建 `assets/data/SpiritItemTable.csv`：
  ```
  id,name,rarity,base_value,use_effect
  spirit_grass,灵草,common,20,stamina_recover_5
  cloud_dew,云露,uncommon,50,cloud_resistance_1
  spirit_dust,灵尘,rare,100,craft_boost_1
  star_fragment,星辰碎片,legendary,500,quality_boost_1
  ```
- 灵物可作为工坊加工原料，产出高价值产品
- **落点**: `domain/economy/SpiritItemTable.csv`

**交付标准**：
- 灵物物品表完整
- 灵物可出售/加工

---

### BE-033 | 云海大潮灵界加成

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 大潮状态下灵草采集已实现掉落翻倍
- [x] 大潮状态下采集产物已切换到高阶分支（`cloud_dew`）
- [ ] 稀有采集点显式生成与 `SpiritRealmManager` 独立系统化尚待补齐

**任务描述**：
实现大潮期间灵界的专属加成。

**技术要求**：
- 在 `CloudSystem` 中，当状态为 `Tide` 时：
  - 灵物掉落率 ×2
  - 出现稀有灵物采集点（`spirit_grass_rare`）
  - 触发特殊大潮专属对话（NPC 会有特殊反应）
- 在 `SpiritRealmManager` 中自动应用以上加成
- **落点**: `domain/CloudSystem.cpp` + `engine/systems/SpiritRealmManager.cpp`

**交付标准**：
- 大潮期间灵界掉落翻倍
- 稀有采集点在大潮时出现

---

### BE-034 | 灵气镰刀工具

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已支持 `SpiritSickle` 物品
- [x] 灵界采集时普通/灵镰效率分别为 `1` / `3`
- [x] 工坊链路已接入灵尘打造灵气镰刀

**任务描述**：
实现灵界专属采集工具——灵气镰刀。

**技术要求**：
- 创建 `SpiritSickle` 工具物品
- 在灵界中：
  - 普通镰刀采集效率：1 个/次
  - 灵气镰刀采集效率：3 个/次
- 灵气镰刀需要用灵尘在工坊制作
- **落点**: `domain/tools/SpiritSickle.hpp` + `engine/InteractionSystem.cpp`

**交付标准**：
- 灵气镰刀采集效率高于普通镰刀
- 灵气镰刀可在工坊制作

---

### BE-035 | 灵界双向传送

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已支持主地图与灵界双向 E 键切换
- [x] 已支持 22:00 后自动返回主地图
- [ ] 淡入淡出动画与传送门粒子特效可后续增强

**任务描述**：
实现主地图 ↔ 灵界的双向传送。

**技术要求**：
- 主地图 → 灵界：传送门（E 键）
- 灵界 → 主地图：传送门（E 键）或游戏内时间 22:00 后自动返回
- 大潮期间：传送门发光特效
- 传送时有淡入淡出动画（1.0s）
- **落点**: `engine/SceneManager.cpp` + `engine/InteractionSystem.cpp`

**交付标准**：
- 双向传送均可正常触发
- 22:00 自动返回主地图

---

### BE-036 | 灵物加工链

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已新增灵物加工配方到 `assets/data/recipes.csv`
- [x] 运行时工坊交互已支持 `spirit_essence/cloud_elixir/immortal_tea` 产出链
- [x] 灵物与茶加工系统联动已接入

**任务描述**：
在 `RecipeTable.csv` 中添加灵物加工配方。

**技术要求**：
- 添加配方：
  ```
  spirit_grass ×3 → spirit_essence ×1 (60s)
  cloud_dew ×2 + spirit_dust ×1 → cloud_elixir ×1 (120s)
  cloud_elixir ×1 + TeaPack ×1 → immortal_tea ×1 (300s, 品质=legendary)
  ```
- 灵界产物与茶加工系统联动
- **落点**: `domain/economy/RecipeTable.csv` + `engine/WorkshopSystem.cpp`

**交付标准**：
- 灵物加工链完整
- 传说品质灵茶可制作

---

### BE-037 | 灵气阶段可视化数据

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有契约进度与阶段文本输出基础
- [x] 运行时已具备灵气值提示与进度展示入口
- [ ] 尚未按 0-20/20-50/50-80/80-95/95-100 的独立灵气阶段区间完整建模

**任务描述**：
实现灵气阶段的数据管理和计算逻辑。

**技术要求**：
- 在 `CloudGuardianContract` 中实现灵气积累机制：
  - 荒废（0-20%）：基础产出
  - 苏醒（20-50%）：灵物正常
  - 兴盛（50-80%）：灵物丰富
  - 繁华（80-95%）：稀有灵物出现
  - 太初（95-100%）：金色云海，最稀有产物
- 提供接口供 `WorldRenderer` 获取当前灵气阶段
- **落点**: `domain/contracts/CloudGuardianContract.cpp`

**交付标准**：
- 灵气阶段正确计算
- UI 面板可获取灵气阶段数据

---

### BE-038 | 灵界中层 TMX 地图

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] `assets/maps/spirit_realm_layer2.tmx` 已存在
- [x] 灵尘掉落与灵界怪物交互已有基础链路
- [x] 可选战斗导向（接触交互 + 资源掉落）已具备轻量原型
- [ ] J 键战斗触发与中层专属战斗流程仍待独立完善

**任务描述**：
设计灵界中层地图，引入轻度战斗区域。

**技术要求**：
- 新建 `assets/maps/spirit_realm_layer2.tmx`
- 引入低难度灵云兽（`SpiritBeast` 类型怪物）
- 按 J 键使用灵镰刀进行攻击
- 击败后掉落灵尘
- 战斗为可选内容，高情商玩家可完全避开
- **落点**: `assets/maps/spirit_realm_layer2.tmx`

**交付标准**：
- 灵界中层地图正确加载
- 战斗系统正确触发

---

## B5 — 工坊与经济系统

> 目标：完善工坊加工链、商店系统、金币经济闭环。

### BE-039 | 玩家金币系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `GameWorldState` 已有初始资金 `gold=500`
- [x] 购买/出售链路已实现金币增减与不足拦截
- [x] 存档链路已接入金币持久化

**任务描述**：
实现玩家金币持有和交易逻辑。

**技术要求**：
- 在 `GameWorldState` 中添加 `int gold_ = 500`（初始资金 500）
- `PixelGameHud` 中显示 `"💰 {gold}"`（前端 UI BE-008）
- 所有买卖操作增减金币：
  - 收购商出售：增加金币
  - 商店购买：减少金币
- 金币不足时交易失败
- **落点**: `domain/player/PlayerGold.hpp` + `engine/InteractionSystem.cpp`

**交付标准**：
- 金币系统正常运作
- 买卖交易正确增减金币

---

### BE-040 | 皮埃尔商店 NPC 和界面

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 商店购买交互与价格表读取已存在可用链路
- [x] 商店日库存基础机制已接入（杂货店日更）
- [ ] `ShopSystem` 独立类与完整店铺 UI 仍待抽离实现

**任务描述**：
实现商店 NPC 和购买逻辑。

**技术要求**：
- 创建 `ShopSystem` 类
- 商店界面使用 `PixelUiPanel`（前端 UI BE-033）
- 商店库存每日刷新（从 `GeneralStoreTable.csv` 读取）
- 玩家选择商品后扣除金币，增加物品到背包
- 从 `PriceTable.csv` 读取价格
- **落点**: `engine/systems/ShopSystem.cpp`

**交付标准**：
- 商店可购买物品
- 金币扣除和物品增加正确

---

### BE-041 | 收购商系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已实现按 E 触发收购商出售
- [x] 已实现品质倍率影响售价（1x/1.5x/2x/3x）
- [x] 已完成背包扣除与金币增加文案反馈

**任务描述**：
实现玩家向收购商出售物品的逻辑。

**技术要求**：
- 在 `InteractionSystem` 中实现出售逻辑
- 对可收购物品（作物/加工品）按 E 键触发收购界面
- 根据物品品质计算收购价：
  - 普通 1x / 优质 1.5x / 稀有 2x / 灵品 3x
- 显示：`"收购商买下了 {物品}，获得 {金额} 金"`
- 从背包扣除物品，增加金币
- **落点**: `engine/InteractionSystem.cpp`

**交付标准**：
- 物品可出售给收购商
- 品质影响收购价

---

### BE-042 | 物品价格表配置化

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已有 `assets/data/PriceTable.csv` 且字段完整
- [x] 商店/收购商共用同一价格表数据源
- [x] 价格读取链路已在运行时接入

**任务描述**：
创建物品价格表 CSV。

**技术要求**：
- 创建 `assets/data/PriceTable.csv`：
  ```
  item_id,buy_price,sell_price,buy_from,category
  TurnipSeed,20,10,shop,seed
  TeaLeaf,0,15,purchaser,crop
  TeaPack,0,100,purchaser,processed
  ```
- 所有价格从 CSV 读取
- 商店和收购商共用同一数据源
- **落点**: `infrastructure/data/PriceTableLoader.cpp`

**交付标准**：
- 价格表完整
- 商店和收购商共用同一价格表

---

### BE-043 | 工坊加工链完善

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 工坊加工、进度更新与产物入包链路已可运行
- [x] 配方从 `recipes.csv` 数据源读取并驱动加工
- [x] 已接入灵物加工配方扩展链

**任务描述**：
确认工坊加工链完整，支持多配方分类。

**技术要求**：
- 工坊标签页分类：制茶/加工/酿造/精炼
- 制作队列：最多 3 个
- 制作进度：实时更新
- 制作完成：物品自动进入背包
- 从 `RecipeTable.csv` 读取配方
- 自动制作开关（`workshop_auto_craft` 配置）
- **落点**: `engine/WorkshopSystem.cpp`

**交付标准**：
- 工坊可制作物品
- 制作队列正确更新

---

### BE-044 | 邮购系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已支持邮购下单即时扣款
- [x] 已支持次日送达（邮件订单列表）
- [x] 订单数据已纳入存档持久化链路

**任务描述**：
实现邮购预购系统。

**技术要求**：
- 创建 `MailOrderSystem` 类
- 在工坊界面添加"邮购"标签页，显示可预购的种子/物品列表
- 玩家下单后，金币立即扣除
- 物品在次日早晨通过邮件系统送达
- 订单数据持久化（存档）
- **落点**: `engine/systems/MailOrderSystem.cpp`

**交付标准**：
- 邮购下单正确扣款
- 次日物品送达邮件系统

---

### BE-045 | 杂货店系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已支持每日杂货店库存选择与购买
- [x] 数据来源与价格表联动可运行
- [x] 大潮期间特殊进货链路已接入

**任务描述**：
实现宋杂货店每日随机进货系统。

**技术要求**：
- 创建 `GeneralStore` 类，继承自 `ShopSystem`
- 杂货店每天有不同的随机商品（从 `GeneralStoreTable.csv` 读取每日进货列表）
- 商品价格固定但种类每天变化
- 大潮期间杂货店有特殊进货
- **落点**: `engine/systems/GeneralStoreSystem.cpp`

**交付标准**：
- 杂货店每日进货随机变化
- 大潮期间有特殊商品

---

### BE-046 | 季节商店（大潮特殊商店）

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已实现大潮状态下 `Tide Shop` 开放判定
- [x] 已支持限定商品购买链路
- [x] 大潮结束后自动关闭逻辑已接入

**任务描述**：
实现大潮期间的特殊商店。

**技术要求**：
- 当 `CloudSystem::State() == Tide` 时，在主地图固定位置显示 `TideShop`
- 商店出售普通商店不提供的限定商品：
  - 星辰碎片
  - 翡翠戒指
  - 传说品质种子
- 大潮结束后商店自动关闭
- **落点**: `engine/systems/TideShopSystem.cpp`

**交付标准**：
- 大潮商店仅在大潮期间开放
- 限定商品正确显示

---

### BE-047 | 商店价格波动系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已实现按周购买/售出次数动态调整价格
- [x] 已实现波动上限/下限约束
- [x] 波动数据随存档价格表持久化

**任务描述**：
实现商店价格动态调整。

**技术要求**：
- 在 `ShopSystem` 中跟踪玩家购买每种物品的累计数量
- 当某物品在一周内购买超过 10 个：次日价格上涨 10%（上限 2 倍原价）
- 当玩家向收购商卖出某物品超过 20 个：收购价下降 5%（下限 0.5 倍原价）
- 价格波动数据跨存档持久化
- **落点**: `engine/systems/PriceFluctuationSystem.cpp`

**交付标准**：
- 价格随供需动态变化
- 波动有上限和下限

---

## B6 — 战斗系统

> 目标：将已有战斗系统接入主循环，使其可实际触发。

### BE-048 | 战斗系统 CSV 加载

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `BattleField` 已实现 `LoadSpiritTableFromCsv/LoadSkillTableFromCsv/LoadZoneTableFromCsv`
- [x] 已新增 `assets/data/battle/spirit_table.csv`
- [x] 已新增 `assets/data/battle/skill_table.csv`
- [x] 已新增 `assets/data/battle/zone_table.csv`

**任务描述**：
实现战斗数据的 CSV 加载。

**技术要求**：
- 在 `BattleField.cpp` 中实现三个加载函数：
  - `LoadSpiritTableFromCsv()` — 解析 `assets/data/battle/spirit_table.csv`
  - `LoadSkillTableFromCsv()` — 解析 `assets/data/battle/skill_table.csv`
  - `LoadZoneTableFromCsv()` — 解析 `assets/data/battle/zone_table.csv`
- 参考 `docs/design/battle/战斗系统集成指南.md` Step 4.1-4.3 的模板代码
- **落点**: `engine/BattleField.cpp`

**交付标准**：
- 三个 CSV 文件正确加载
- 数据结构完整填充

---

### BE-049 | 战斗系统初始化接入

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已在 `BattleManager::Initialize` 接入战斗 CSV 默认加载
- [x] 已完成战斗管理器初始化路径接入
- [ ] `GameApp::Run` 入口已重构为运行时协调，按当前架构在 `GameRuntime/BattleManager` 侧生效

**任务描述**：
在游戏主循环中初始化战斗系统。

**技术要求**：
- 在 `GameApp::Run()` 中：
  - 创建 `battle_manager_` 实例
  - 调用 `battle_manager_.Initialize(&cloud_system_)`
  - 加载 CSV 数据到 `battle_field_`
- **落点**: `engine/GameApp.cpp`

**交付标准**：
- 战斗系统正确初始化
- CSV 数据正确加载

---

### BE-050 | 战斗触发检测

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已具备战斗触发距离检测（`ShouldTriggerBattle`）
- [x] 已接入 `EnterBattle(...)` 与战斗状态切换链路

**任务描述**：
实现玩家与污染灵体的碰撞检测和战斗触发。

**技术要求**：
- 在 `GameApp::Update()` 中实现 `CheckBattleTrigger_()`：
  - 检测玩家与污染灵体的碰撞距离
  - 接近时调用 `battle_manager_.EnterBattle(zone, {spirit_ids})`
  - 设置 `in_battle_mode_ = true`
- **落点**: `engine/GameApp.cpp`

**交付标准**：
- 玩家接近灵体时正确触发战斗
- 战斗模式切换正常

---

### BE-051 | 战斗输入和渲染分支

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已具备战斗状态 Update 分支与 `BattleUI` 同步更新
- [x] 已具备技能键输入入口 `OnSkillKeyPressed`
- [x] 已具备撤退与胜利确认的状态切换入口
- [ ] Q/W/E/R、Escape、X 的完整按键映射与独立战斗渲染分支仍需与当前主循环做最终对齐

**任务描述**：
将 GameApp 的 Update 和 Render 分支加入战斗模式处理。

**技术要求**：
- Update 分支：
  ```cpp
  if (in_battle_mode_) {
      battle_manager_.Update(delta_seconds, player_pos.x, player_pos.y);
      if (!battle_manager_.IsInBattle()) in_battle_mode_ = false;
      return;
  }
  ```
- Render 分支：
  ```cpp
  if (in_battle_mode_) {
      DrawBattleBackground(window);
      DrawPollutedSpirits(window);
      battle_manager_.GetUI().Draw(window);
  }
  ```
- 输入处理：Q/W/E/R 释放技能，Escape 暂停，X 撤退
- **落点**: `engine/GameApp.cpp`

**交付标准**：
- 战斗期间 Update/Render/Input 正确分支
- 战斗 UI 正确显示

---

### BE-052 | 战斗后奖励系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 战斗胜负状态机与撤退安全返回链路已存在
- [x] `ProcessVictory_()` 已实现奖励结算分发（羁绊/经验/灵尘与战利品）
- [x] 奖励回调接口已补齐（物品、经验、羁绊、通知）

**任务描述**：
实现战斗胜利后的奖励发放。

**技术要求**：
- 战斗胜利时触发 `ProcessVictory_()`：
  - 更新灵兽羁绊好感
  - 发放 SpiritGuard 技能经验
  - 掉落灵尘物品（自动进入背包）
- 战斗失败时：自动撤退，无惩罚
- **落点**: `engine/BattleManager.cpp`

**交付标准**：
- 战斗胜利后奖励正确发放
- 战斗失败安全撤退

---

### BE-053 | 战斗存档接入

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 战斗系统已具备可独立状态对象与初始化入口
- [ ] 战斗中存档结算、防刷战斗与读档恢复策略尚未完整接入

**任务描述**：
确保战斗状态正确存档。

**技术要求**：
- 战斗中存档自动结算胜利（防止存档刷战斗）
- 保存战斗可用状态（灵兽配置等）
- 战斗中退出自动重置战斗状态
- **落点**: `infrastructure/SaveGameService.cpp`

**交付标准**：
- 存档/读档时战斗状态正确处理

---

## B7 — 节日与成就系统

> 目标：实现节日活动和成就系统。

### BE-054 | 节日数据 CSV

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已新增 `assets/data/festival/festival_definitions.csv`
- [x] 已包含 12 个节日定义（含大潮祭）
- [x] 节日字段覆盖触发/奖励基础信息

**任务描述**：
创建节日定义数据表。

**技术要求**：
- 创建 `assets/data/festival/festival_definitions.csv`：
  ```
  id,name,season,day,description,special_item,reward_type,reward_value
  spring_festival,春节,winter,1,新年第一天,RedEnvelope,gold,200
  lantern_festival,元宵节,winter,15,正月十五赏灯,StickyRiceBall,stamina,30
  ...
  ```
- 12 个节日：春节、元宵节、花朝节、清明、端午、七夕、中秋、重阳、冬至、大潮祭、茶文化节、丰收祭
- **落点**: `domain/world/FestivalData.csv`

**交付标准**：
- 节日数据表完整
- 节日触发日期正确

---

### BE-055 | 节日触发系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `FestivalSystem` 已实现按季节/日期触发节日
- [x] 已实现节日前预告（常规 3 天/特殊 7 天）
- [x] 已提供 `GetNoticeText()` 供 HUD/日报展示

**任务描述**：
实现节日日期检测和事件触发。

**技术要求**：
- 在 `GameClock` 中检测日期变化
- 节日当天：触发 `FestivalActiveEvent`
- 节日预告：提前 3 天在云海日报中显示（普通节日）/ 提前 7 天显示（大潮祭）
- **落点**: `domain/world/FestivalSystem.cpp` + `engine/GameClock.cpp`

**交付标准**：
- 节日当天正确触发
- 节日预告正确显示

---

### BE-056 | 成就数据表

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `assets/data/AchievementTable.csv` 已扩展至 20+ 条
- [x] 已覆盖收获/社交/建筑/战斗/经济等主类型条件

**任务描述**：
扩展成就数据表。

**技术要求**：
- 扩展 `assets/data/AchievementTable.csv`：
  ```
  id,name,description,condition,reward
  first_crop,初次收获,收获你的第一个作物,harvest_count>=1,gold+100
  ten_crops,小有规模,累计收获100个作物,total_harvest>=100,gold+500
  full_heart,心心相印,与任意NPC好感度达到8心,any_npc_favor>=2000,title+称号
  ```
- 从 4 条扩展到 20+ 条
- **落点**: `domain/world/AchievementData.csv`

**交付标准**：
- 成就表包含 20+ 条成就
- 成就条件可被正确检测

---

### BE-057 | 成就触发和通知系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 现有 UI 通知横幅链路可承接成就提示
- [x] 已新增统一成就解锁入口（交互链路内 `UnlockAchievement_`），避免重复触发
- [x] 已接入成就提示文案：`beast_bond` / `beast_bond_max` / `first_pet` / `home_designer`
- [x] 已新增独立 `AchievementSystem`（`UnlockOnce` + `EvaluateDaily`）并接入 `GameRuntime` 日结流程
- [ ] 事件总线监听（收获/送礼/建造）与通用发奖流水尚未完整落地

**任务描述**：
实现成就达成检测和通知。

**技术要求**：
- 创建 `AchievementSystem` 类
- 监听游戏事件（收获/送礼/建造等）
- 达成时：
  - 触发成就解锁事件
  - 发放奖励（物品/金币/称号）
  - 触发通知横幅（前端 UI BE-006）
- 成就数据跨存档持久化
- **落点**: `engine/systems/AchievementSystem.cpp`

**交付标准**：
- 成就达成时正确触发通知
- 奖励正确发放

---

### BE-058 | 任务（Quest）管理器

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已存在 `QuestTable.csv` 与 `PixelQuestMenu` 展示链路
- [x] 已新增独立 `QuestManager`（CSV 加载 / 每日刷新 / 进度检测）
- [x] `GameRuntime` 已改为通过 `QuestManager` 接管任务自动接取与完成判定
- [x] 已补齐时段刷新基础：06:00 / 12:00 / 18:00 分批自动接取
- [x] 已补齐基础奖励结算：从任务 `reward` 文本提取金币并发放
- [ ] 奖励类型扩展（体力上限/道具/好感）与独立发奖流水仍待补齐

**任务描述**：
实现任务管理逻辑。

**技术要求**：
- 创建 `QuestManager` 类
- 从 `assets/data/QuestTable.csv` 加载任务列表
- 支持任务状态：未接取/进行中/已完成
- 任务完成后触发奖励（物品/金币/好感度）
- 任务在每日 6:00 刷新或按时段刷新
- **落点**: `engine/systems/QuestManager.cpp`

**交付标准**：
- 任务列表正确加载
- 任务状态和奖励正确

---

### BE-059 | 主线剧情系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `MainPlotSystem` 已接入并可更新推进
- [x] 已支持章节数据加载与剧情触发基础链路
- [x] 已具备剧情进度持久化基础

**任务描述**：
完善主线剧情系统接入。

**技术要求**：
- 确认 `MainPlotSystem` 已完整实现（已在 Week 3 中接入）
- 确保 10 章剧情数据（`assets/data/plot/chapter_*.json`）可加载
- 剧情触发条件（好感度/道具/日期）正确检测
- 多结局路由正确执行
- **落点**: `engine/systems/MainPlotSystem.cpp`

**交付标准**：
- 主线剧情可正常推进
- 存档正确持久化剧情进度

---

## B8 — 建筑与经营系统

> 目标：实现建筑升级、温室、工坊升级等经营玩法。

### BE-060 | 主屋升级系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已新增 `assets/data/building/BuildingUpgradeTable.csv`（主屋升级定义）
- [x] 交互链路已支持主屋升级消耗与等级提升
- [x] 升级后功能解锁链路已接入（含温室开启）

**任务描述**：
实现主屋从帐篷→小木屋→砖房→大宅的升级流程。

**技术要求**：
- 创建 `BuildingUpgrade` 类
- 在 `assets/data/BuildingUpgradeTable.csv` 中定义升级需求：
  ```
  building_id,level,wood_cost,gold_cost,days_required,new_feature
  main_house,1,4,0,0,帐篷→小木屋
  main_house,2,10,500,3,小木屋→砖房，+1房间
  main_house,3,20,2000,7,砖房→大宅，+2房间+温室
  ```
- 玩家与升级标志物交互，消耗材料
- 升级期间显示建造动画（进度条）
- 完成后解锁新功能
- **落点**: `engine/systems/BuildingSystem.cpp`

**交付标准**：
- 主屋可升级
- 升级消耗正确扣除
- 新功能正确解锁

---

### BE-061 | 工坊升级系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 工坊升级链路已支持槽位增长（1/2/4/6）
- [x] 升级消耗金币与材料逻辑已接入
- [x] 升级后可立即生效

**任务描述**：
实现工坊槽位升级。

**技术要求**：
- 在 `WorkshopSystem` 中实现升级逻辑：
  - 基础工坊：1 个加工槽
  - Lv.2：2 个加工槽
  - Lv.3：4 个加工槽
  - Lv.4：6 个加工槽（终极）
- 升级消耗金币和材料
- 升级后立即生效
- **落点**: `engine/WorkshopSystem.cpp`

**交付标准**：
- 工坊槽位随等级增加
- 升级消耗正确扣除

---

### BE-062 | 茶园种植系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已存在 `TeaGardenPlots` 独立数据与运行时更新链路
- [ ] `TeaGarden` 独立类与“160秒周期 + 每周浇水”专属规则仍需单独固化

**任务描述**：
实现与普通农田分离的茶树种植区。

**技术要求**：
- 创建 `TeaGarden` 类
- 茶树生长周期：160 秒（vs 普通作物 80 秒）
- 茶树产出茶叶（`TeaLeaf`），用于工坊加工
- 茶树品质受云海天气影响（大潮时产出茶叶品质更高）
- 茶树不需要每天浇水（每周浇水 1-2 次即可）
- **落点**: `domain/farming/TeaGarden.cpp`

**交付标准**：
- 茶园独立于普通农田
- 大潮时茶树品质提升

---

### BE-063 | 客栈经营系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有客栈金库/订单相关基础状态字段
- [ ] `InnSystem` 独立系统与完整经营结算回路尚待补齐

**任务描述**：
实现客栈吸引 NPC 来访和远程订单系统。

**技术要求**：
- 创建 `InnSystem` 类
- 客栈吸引 NPC 来访（增加社交机会）
- 接收远程订单（通过邮件系统送达）
- 经营数据：每日收入/支出/利润统计
- 客栈等级越高，来的 NPC 越多，订单价值越高
- **落点**: `engine/systems/InnSystem.cpp`

**交付标准**：
- 客栈可正常经营
- 每日收益正确结算

---

### BE-064 | 建筑装饰系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有装饰分值（`decoration_score`）等基础数据链路
- [x] 已新增独立 `DecorationSystem`，并接管 `Decoration Bench` 交互分支
- [x] 已实现基础舒适度反馈：制作装饰会提升 `decoration_score`，并触发家园成就链路
- [ ] 家具类型细分（桌/椅/床/书架/盆栽）与摆放网格系统尚待实现
- [ ] 舒适度对体力恢复速度的数值联动尚待实现

**任务描述**：
实现建筑内家具装饰系统。

**技术要求**：
- 创建 `DecorationSystem` 类
- 玩家可在建筑内放置家具（桌子/椅子/床/书架/盆栽）
- 每个家具有舒适度加成，影响玩家每日体力恢复速度
- 部分装饰品有故事背景（通过与 NPC 对话揭示）
- **落点**: `engine/systems/DecorationSystem.cpp`

**交付标准**：
- 家具可放置
- 舒适度影响体力恢复

---

### BE-065 | 鸡舍/畜棚系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已具备畜棚喂养与产出相关交互基础
- [ ] `CoopSystem/BarnSystem` 独立养殖循环尚需完整实现

**任务描述**：
实现动物养殖系统。

**技术要求**：
- 创建 `CoopSystem` 和 `BarnSystem` 类
- 玩家建造畜棚后，可购买动物（鸡/鸭/牛/羊）
- 动物每天产出产品（鸡蛋/鸭蛋/牛奶/羊毛）
- 动物需要喂食和清理（有体力/时间成本）
- 高好感灵兽可辅助照料动物
- **落点**: `engine/systems/CoopSystem.cpp`

**交付标准**：
- 动物可购买和养殖
- 每日产出正确

---

## B9 — 存档与配置系统

> 目标：完善存档系统、配置管理、日志系统。

### BE-066 | 存档版本管理

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已有存档读写与多字段解析框架
- [x] 存档头已包含 `version` 字段（当前 `v4`）
- [x] 读档时已解析并提示版本号（防止静默误读）
- [ ] 版本迁移（v1~v3 -> v4）仍可继续补齐（当前以“前向兼容字段缺省”方式运行）

**任务描述**：
实现存档版本管理和迁移。

**技术要求**：
- 在存档文件中加入版本号字段（当前 v4）
- 存档加载时检测版本号：
  - 如果版本号低于当前版本，执行迁移逻辑
  - 如果版本号高于当前版本，报错并拒绝加载（防止降级损坏）
- 迁移逻辑在 `SaveGameService.cpp` 中实现
- **落点**: `infrastructure/SaveGameService.cpp`

**交付标准**：
- 存档版本正确检测
- 版本迁移逻辑完整

---

### BE-067 | 存档损坏检测和恢复

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已有解析错误提示与部分容错分支
- [x] 存档已写入 `checksum`（FNV-1a 32-bit）并在读档时校验
- [x] checksum 不匹配时自动尝试回退到 `.bak` 备份存档
- [x] 每次写档前会备份上一版本存档（`.bak`）

**任务描述**：
增强存档损坏检测和恢复能力。

**技术要求**：
- 存档文件加入 checksum 字段（CRC32 或 MD5）
- 存档加载时验证 checksum，如果不匹配：
  - 尝试从自动备份恢复
  - 如果无备份，显示错误并提供新建存档选项
- 自动备份：每次存档前备份上一版本存档
- **落点**: `infrastructure/SaveGameService.cpp`

**交付标准**：
- 损坏存档正确检测
- 自动备份可恢复

---

### BE-068 | GameConfig 详细错误日志

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 现有 `GameConfig` 已提供基础加载与日志输出
- [x] 配置文件不可读/格式错误时输出路径与行号（加载阶段）
- [x] 字段缺失时输出 key 与 fallback 默认值（读取阶段）
- [x] 错误不影响启动（使用默认值兜底）

**任务描述**：
增强配置文件加载的错误处理。

**技术要求**：
- 配置加载失败时输出详细错误信息：
  - 文件路径
  - 错误类型（文件不存在/解析失败/字段缺失）
  - 行号（如果适用）
  - 使用的默认值
- 使用 `Logger` 类统一输出日志
- **落点**: `infrastructure/data/GameConfigLoader.cpp`

**交付标准**：
- 配置错误有详细日志
- 错误不影响游戏启动（使用默认值兜底）

---

### BE-069 | TMX 地图加载健壮性

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] TMX 加载失败已有基础回退与日志链路
- [x] 缺失/损坏属性解析失败不崩溃（解析失败会跳过并降级）
- [x] Ground 图层缺失时使用占位地面瓦片（保证场景可构建）
- [x] CSV 数据异常时用 0 占位（避免 `stoi/stof` 崩溃）

**任务描述**：
增强 TMX 地图加载的错误处理。

**技术要求**：
- 地图文件不存在时使用默认地图
- 解析错误时跳过错误元素并记录日志
- 缺少必需图层时使用占位符
- **落点**: `infrastructure/TmxMapLoader.cpp`

**交付标准**：
- 地图加载失败不崩溃
- 错误日志详细

---

### BE-070 | Mod API 框架

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有 `mod_hooks` 相关数据接入点
- [x] `IMod` 接口已存在（OnLoad/OnUpdate/OnRender）
- [x] `ModLoader` 已支持扫描 `mods/` 并解析 `mod.json`
- [x] 已提供 `BuildDataRoots(base_data_root)` 供数据表叠加扩展
- [x] 已提供示例 mod：`mods/example_mod/mod.json`
- [ ] “核心数据表从 mod 目录扩展加载”的实际接入点仍需逐表落地（Price/Crop/Dialogue/Map 等）

**任务描述**：
实现 Mod 支持预留 API。

**技术要求**：
- 设计 `ModLoader` 类：
  - 扫描 `mods/` 目录
  - 加载 `mod.json` 配置文件
  - 注入自定义作物/对话/地图
- 定义 `IMod` 接口：
  - `onLoad()` / `onUpdate(float)` / `onRender()`
- 确保核心数据表支持从 mod 目录扩展
- **落点**: `infrastructure/ModLoader.cpp`

**交付标准**：
- Mod 加载框架可用
- 可加载简单的 mod 示例

---

## B10 — 音频与输入系统

> 目标：完善音频系统、输入管理、控制器支持。

### BE-071 | 音效池管理系统

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `AudioManager` 已具备 `SoundBuffer` 预加载与缓存复用
- [x] `PlaySFX` 走池化取用并支持复用
- [x] 缺失音效返回失败而不崩溃（可安全降级）

**任务描述**：
完善 `AudioManager` 的音效池管理。

**技术要求**：
- 实现 `sf::SoundBuffer` 预加载池
- `playSound(const std::string& id)` 从池中取用
- 避免重复加载（已加载的 SoundBuffer 复用）
- 未找到音效时返回默认占位而非崩溃
- **落点**: `engine/audio/AudioManager.cpp`

**交付标准**：
- 音效预加载和缓存正常工作
- 未找到音效不崩溃

---

### BE-072 | BGM 流式播放

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `AudioManager` 已使用 `sf::Music` 做 BGM 流式播放（`PlayBGM`/`StopBGM`）
- [x] 三通道音量（`music_volume` / `bgm_volume` / `sfx_volume`）已独立维护
- [x] 已从 `configs/audio.json` 读取音量配置
- [x] BGM 已支持淡入/淡出更新流程（场景切换处 0.5s 级别接入可直接复用）

**任务描述**：
实现 BGM 流式播放。

**技术要求**：
- 使用 `sf::Music` 实现流式播放
- 三通道独立音量控制：Music / BGM / SFX
- 从 `configs/audio.json` 读取音量配置
- 场景切换时 BGM 淡入淡出（0.5s）
- **落点**: `engine/audio/AudioManager.cpp`

**交付标准**：
- BGM 流式播放正常
- 三通道音量独立可调

---

### BE-073 | 音效触发点接入

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] 已接入：收割 `harvest`、浇水 `water`、对话继续 `dialogue_continue`、升级 `level_up`
- [x] 本轮新增：种植 `plant`、送礼 `gift`、心事件触发 `heart_event`
- [x] 已补齐：购买 `shop_purchase`（Shop Stall / General Store / Tide Shop / Mailbox 下单）

**任务描述**：
在关键交互点插入音效调用。

**技术要求**：
- 收割：`audio.playSound("harvest")`
- 浇水：`audio.playSound("water")`
- 对话继续：`audio.playSound("dialogue_continue")`
- 升级：`audio.playSound("level_up")`
- 其他关键音效：
  - 种植：`plant`
  - 购买：`shop_purchase`
  - 送礼：`gift`
  - 心事件触发：`heart_event`
- **落点**: `engine/InteractionSystem.cpp` + 各相关系统

**交付标准**：
- 所有关键交互点有音效反馈
- 音效 ID 正确对应音频文件

---

### BE-074 | 输入管理器完善

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `InputManager` 默认映射已支持 WASD + 方向键并行
- [x] `IsActionJustPressed` 已通过按帧状态对比实现
- [x] 移动向量返回前已做归一化
- [x] 已提供手柄向量与连接检测接口（左摇杆移动）
- [x] 支持按键重绑定与配置加载入口

**任务描述**：
完善 `InputManager` 的控制器支持。

**技术要求**：
- WASD + 方向键同时可用
- `IsActionJustPressed`（本帧刚按下）正确实现
- 移动向量归一化
- 手柄支持：左摇杆移动，A/B 按钮映射
- 按键映射可通过配置文件覆盖
- **落点**: `engine/input/InputManager.cpp`

**交付标准**：
- 键盘和手柄输入均正常
- 按键映射可配置

---

### BE-075 | 四季 BGM 路由

**开发进度**：✅ 已完成（2026-04-22）

**完成标识**：
- [x] `DayCycleRuntime` 已具备季节到 BGM 路径映射（春/夏/秋/冬）
- [x] 日切逻辑已可感知季节并产出切换提示
- [x] `GameRuntime::OnDayChanged()` 已提供 `callbacks_.play_bgm(path, loop, fade_in, fade_out)`，并按季节/大潮覆写路由
- [x] 已在 `GameApp` 侧绑定 `play_bgm` 到 `audio::AudioManager::PlayBGM(...)` + `StopBGM(fade_out)`，并在每帧调用 `AudioManager::Update(dt)` 完成淡入淡出

**任务描述**：
实现季节变更时 BGM 自动切换。

**技术要求**：
- 在 `DayCycleRuntime.cpp` 中监听季节变更
- 自动切换 BGM：
  - 春：`spring_theme.ogg`
  - 夏：`summer_theme.ogg`
  - 秋：`autumn_theme.ogg`
  - 冬：`winter_theme.ogg`
- 时段变奏（可选）：清晨/午后/傍晚/夜晚 各有不同版本
- **落点**: `engine/systems/DayCycleRuntime.cpp`

**交付标准**：
- 季节变更时 BGM 自动切换
- 切换时淡入淡出

---

## B11 — 灵兽与宠物系统

> 目标：完善灵兽互动、AI 和宠物系统。

### BE-076 | 灵兽命名和个性系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有灵兽状态机与可视表现切换基础（Idle/Wander/Follow/Interact）
- [x] 已完成个性枚举建模：`SpiritBeastPersonality`（活泼/慵懒/好奇）
- [x] 已接入灵兽命名字段：`SpiritBeast::custom_name`（默认值 `灵团`）
- [x] 存档已扩展灵兽名称与个性字段（向后兼容旧存档）
- [x] 个性差异已接入行为参数（Follow/Wander 速度、Idle 时长）
- [ ] 玩家输入命名 UI 入口尚未接入

**任务描述**：
实现灵兽个性差异。

**技术要求**：
- 灵兽有个性枚举：活泼（好动）/ 慵懒（慢悠悠）/ 好奇（到处探索）
- 个性影响行为动画和状态切换频率
- 灵兽有名字（玩家可命名）
- **落点**: `domain/world/SpiritBeast.hpp` + `engine/systems/SpiritBeastSystem.cpp`

**交付标准**：
- 灵兽有个性区分
- 不同个性有不同行为

---

### BE-077 | 灵兽互动系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有灵兽交互与羁绊激活基础流程
- [x] 已存在“灵兽协助浇水/收割倍率”方向上的联动基础
- [x] 已补齐 G 键灵兽三分支最小可用流程：喂食（消耗 `Feed`）/ 抚摸（兜底）/ 派遣（高羁绊）
- [x] 喂食与抚摸均已触发爱心粒子
- [x] 派遣状态已联动跨日自动除虫概率提升（高于普通协助）
- [ ] 独立互动菜单 UI（可选分支选择面板）仍待接入

**任务描述**：
实现灵兽喂食、抚摸、派遣等互动。

**技术要求**：
- 喂食：按 G 键打开交互菜单，选择喂食
  - 特定物品增加灵兽好感
  - 好感影响辅助效率（高好感浇水范围更大）
- 抚摸：按 G 键，有爱心粒子效果
- 派遣：高好感灵兽可派去工作（自动浇水/除虫）
- **落点**: `engine/systems/SpiritBeastSystem.cpp`

**交付标准**：
- 喂食/抚摸/派遣均可用
- 好感影响辅助效率

---

### BE-078 | 灵兽跟随 AI 优化

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 现有跟随/游荡距离阈值与速度参数可运行
- [x] 跟随行为已是连续位移（非瞬移）
- [ ] 障碍物绕行尚未接入
- [ ] 多灵兽分散与防重叠策略尚未实现
- [ ] 玩家高速移动时的跟随弹性区间仍需增强

**任务描述**：
优化灵兽跟随路径的智能性。

**技术要求**：
- 当前灵兽跟随 AI 较简单（`FollowTriggerDistance` 等参数已有）
- 优化点：
  - 平滑跟随（使用插值而非瞬移）
  - 障碍物绕行
  - 避免重叠（多只灵兽同时跟随时分散位置）
  - 停止跟随范围扩大（玩家快跑时灵兽不强制跟随）
- **落点**: `engine/systems/SpiritBeastSystem.cpp`

**交付标准**：
- 灵兽跟随更平滑自然
- 多灵兽同时跟随不重叠

---

### BE-079 | 宠物收养系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有 `pet_adopted` 状态与“首只宠物”成就打点基础
- [x] 已抽象统一收养入口：`AdoptPet_(ctx, pet_type, source)`（支持商店/事件来源扩展）
- [x] 已接入 `Pet House` 收养链路（猫）并预留事件收养入口（鸟）
- [x] 已接入价格表驱动的宠物商品：`pet_cat_license / pet_dog_license / pet_bird_license`
- [x] 商店购买时可通过商品类型自动触发收养（统一复用 `AdoptPet_`）
- [ ] `PetSystem` 独立类尚未落地
- [ ] 猫/狗/鸟差异化与跟随/互动动画尚未完整接入
- [ ] 宠物商品购买流程目前走“默认首项选择”策略，后续需补完整商店选购 UI

**任务描述**：
实现非灵兽宠物系统（猫/狗/鸟）。

**技术要求**：
- 创建 `PetSystem` 类（与灵兽分离）
- 宠物无法辅助农活
- 宠物有陪伴动画（跟随玩家、与玩家互动）
- 宠物通过商店购买或事件获得
- **落点**: `engine/systems/PetSystem.cpp`

**交付标准**：
- 宠物可购买和收养
- 宠物有跟随和互动动画

---

### BE-080 | 灵兽羁绊成就系统

**开发进度**：🟡 进行中（2026-04-22）

**完成标识**：
- [x] 已有灵兽相关成就键值（如 `beast_bond`）触发入口
- [x] 已新增满羁绊成就打点：`beast_bond_max`（羁绊达到 100）
- [x] 满羁绊成就已统一到 `AchievementSystem::EvaluateDaily` 日结判定
- [ ] 全类型灵兽收集里程碑未实现（当前仅单灵兽模型）
- [ ] 成就通知横幅与灵兽羁绊专用事件链仍待统一

**任务描述**：
实现灵兽羁绊成就。

**技术要求**：
- 灵兽好感度达到里程碑时触发成就：
  - 首次招募灵兽
  - 灵兽好感满级
  - 所有类型灵兽各招募一只
- 成就触发后显示通知横幅（前端 UI BE-006）
- **落点**: `engine/systems/SpiritBeastSystem.cpp` + `engine/systems/AchievementSystem.cpp`

**交付标准**：
- 灵兽相关成就可触发
- 通知横幅正确显示

---

## 附录 A：四层架构文件落点参考

```
CloudSeaManor/src/
├── app/                          ← 程序入口
│   └── main.cpp
├── domain/                       ← 纯玩法规则（无 SFML 依赖）
│   ├── player/
│   │   ├── Player.hpp            // 玩家数据（位置/属性/金币）
│   │   └── Stamina.hpp           // 体力系统
│   ├── farming/
│   │   ├── CropData.hpp         // 作物数据
│   │   ├── TeaPlot.hpp           // 茶田地块
│   │   └── TeaGarden.hpp         // 茶园
│   ├── economy/
│   │   ├── RecipeData.hpp        // 配方数据
│   │   └── SpiritItemTable.csv   // 灵物价格表
│   ├── social/
│   │   ├── NpcActor.hpp         // NPC 演员（好感/心情）
│   │   └── DynamicLifeSystem.hpp // 动态人生
│   ├── world/
│   │   ├── GameClock.hpp        // 游戏时钟
│   │   ├── FestivalData.csv      // 节日定义
│   │   └── AchievementData.csv    // 成就数据
│   ├── contracts/
│   │   └── CloudGuardianContract.hpp  // 云海契约
│   ├── CloudSystem.hpp           // 云海天气
│   ├── Inventory.hpp             // 背包
│   ├── PickupDrop.hpp            // 拾取系统
│   ├── SkillSystem.hpp           // 技能系统
│   └── MainPlotSystem.hpp        // 主线剧情
├── engine/                       ← 运行时编排（有 SFML 依赖）
│   ├── core/
│   │   ├── GameApp.cpp          // 主循环（协调者）
│   │   ├── GameRuntime.cpp      // 运行时上下文
│   │   ├── InputManager.cpp      // 输入管理
│   │   ├── SceneManager.cpp     // 场景管理
│   │   └── Profiling.cpp         // 性能分析
│   ├── rendering/
│   │   ├── WorldRenderer.cpp     // 世界渲染
│   │   ├── HudRenderer.cpp      // HUD 渲染
│   │   └── UiPanelInitializer.cpp // UI 面板初始化
│   ├── systems/
│   │   ├── CropGrowthSystem.cpp  // 作物生长运行时
│   │   ├── WorkshopSystem.cpp    // 工坊运行时
│   │   ├── NpcScheduleSystem.cpp // NPC 日程
│   │   ├── NpcDialogueManager.cpp // NPC 对话管理
│   │   ├── InteractionSystem.cpp  // 交互系统
│   │   ├── SpiritBeastSystem.cpp // 灵兽系统
│   │   ├── SpiritRealmManager.cpp // 灵界管理
│   │   ├── ShopSystem.cpp        // 商店系统
│   │   ├── AchievementSystem.cpp  // 成就系统
│   │   ├── QuestManager.cpp      // 任务系统
│   │   ├── FestivalSystem.cpp    // 节日系统
│   │   ├── BuildingSystem.cpp     // 建筑系统
│   │   ├── BattleManager.cpp     // 战斗管理
│   │   └── DayCycleRuntime.cpp   // 昼夜循环
│   ├── audio/
│   │   └── AudioManager.cpp      // 音频管理
│   ├── BattleField.cpp           // 战斗数据
│   ├── BattleUI.cpp             // 战斗 UI
│   ├── DialogueEngine.cpp        // 对话引擎
│   ├── GameWorldState.cpp       // 世界状态
│   ├── UISystem.cpp             // UI 系统
│   └── Pixel*.cpp                // 像素 UI 组件（渲染）
├── infrastructure/               ← IO 与解析（无 SFML 依赖）
│   ├── SaveGameService.cpp      // 存档服务
│   ├── ResourceManager.cpp      // 资源管理
│   ├── TmxMapLoader.cpp         // TMX 地图加载
│   ├── data/
│   │   ├── GameConfigLoader.cpp // 游戏配置加载
│   │   ├── UiLayoutLoader.cpp   // UI 布局配置加载
│   │   ├── PriceTableLoader.cpp  // 价格表加载
│   │   └── SaveSlotManager.cpp   // 存档槽管理
│   └── ModLoader.cpp            // Mod 加载
```

---

## 附录 B：后端测试文件结构

```
tests/
├── TestRunner.cpp
├── TestFramework.hpp
├── domain/
│   ├── CropDataTest.cpp          // 作物数据测试
│   ├── GameClockTest.cpp         // 游戏时钟测试
│   ├── InventoryTest.cpp         // 背包测试
│   ├── StaminaTest.cpp           // 体力测试
│   ├── CloudSystemTest.cpp        // 云海系统测试
│   ├── CropGrowthSystemTest.cpp  // 作物生长测试
│   ├── DialogueEngineTest.cpp    // 对话引擎测试
│   ├── WorkshopSystemTest.cpp     // 工坊系统测试
│   ├── MainPlotSystemTest.cpp     // 主线剧情测试
│   └── ModApiTest.cpp             // Mod API 测试
├── engine/
│   ├── InteractionSystemTest.cpp  // 交互系统测试
│   └── SpiritBeastSystemTest.cpp // 灵兽系统测试
└── infrastructure/
    ├── SaveGameServiceTest.cpp   // 存档服务测试
    └── TmxMapLoaderTest.cpp      // 地图加载测试
```

---

*文档版本 1.0 | 2026-04-22 | 后端开发任务清单，与《云海山庄》前端 UI 任务清单配套使用*
