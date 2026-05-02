# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **深度玩法设计文档首稿**：新增“动态叙事与行为记忆”“灵茶图鉴与茶灵收集”“庄园灵气生态”3 份正式系统设计稿，补齐目标、数据结构、工程落点、存档预留与验收标准
- **数据驱动重构骨架**：`DataRegistry` 升级为 typed CSV registry，`ResourceManager` 新增数据根目录解析与文本资源装载入口
- **数据表 schema 首轮落地**：新增 `tool_data / npc_data / pet_data / tea_data / skill_data / weapon_data / festival_rewards` 样例表
- **数据驱动文档**：新增现状评估报告、统一 schema 文档与数据表维护手册
- **系统扩展 Batch A-P4 首轮落地**：补齐 `GameWorldState` / `GameAppSave` 对日记、配方解锁、技能分支、DIY 摆放、净化回流、垂钓状态的承载与持久化
- **茶叶加工链 MVP**：新增萎凋、杀青、揉捻、干燥四段制茶配方，并补充对应物品显示名
- **食谱加工 MVP**：新增 `茶叶蛋 / 蔬菜汤 / 热牛奶` 的工坊配方与轻量配方解锁同步
- **庄园日记系统**：新增 `DiarySystem`、4 条基础日记、自动解锁逻辑，以及茶室/主屋方向的阅读提示
- **技能 10 级分支骨架**：新增技能分支持久化与 `assets/data/skills/branches.csv`，Lv.10 后自动授予首个分支
- **云海垂钓 MVP**：新增垂钓状态存档字段、基础鱼获与 `assets/data/fishing/spots.csv`
- **DIY 摆放骨架**：`decor_item|id|x|y|rot|room|custom` 从预留 tag 升级为实际读写的布局数据
- **DataRegistry 预留**：新增 `DataRegistry.hpp/cpp`，统一登记 `recipes / fishing / festival / diary / skills` 数据入口
- **战斗系统种子掉落**：新增 `SeedDropTable.csv` 数据文件和 `SeedDropTable.hpp/cpp` 实现，战斗胜利后根据敌人星数随机掉落种子
- **告白/婚礼系统完善**：`BuildRelationshipDialogueMenu_()` 对话注入告白选项，好感度 10 心可告白，订婚后可预约婚礼，婚后每日体力恢复增益
- **战斗 Victory 自动结算**：修复战斗胜利后奖励不发放的 bug，`BattleState::Victory` 状态持续 `kResultDisplayDuration` 后自动调用 `ProcessVictory_()`
- **美术资源映射系统 (SpriteMapping)**：新增 `SpriteMapping` 类，支持 CSV 配置表统一管理所有美术资源 ID → 路径映射，实现策划零门槛换皮
- **多主题皮肤切换系统 (SpriteThemeManager)**：支持春/夏/秋/冬多套美术资源主题，通过 `configs/sprite_themes.csv` 配置
- **ResourceManager 集成 SpriteMapping**：新增 `SetSpriteMapping()` / `LoadTextureBySpriteId()` / `PreloadTexturesBySpriteIds()` 方法

### Changed
- **日记系统数据源**：`DiarySystem` 现在优先从 `assets/data/diary/entries.csv` 加载定义，保留内置 fallback
- **节日 CSV 兼容性**：`FestivalSystem` 现支持带表头和旧式无表头 CSV，两种格式都可读取
- **净化回流经营链**：战斗净化结果会形成 3 天的庄园回流状态，并在次日日结中推动茶园生机与可追踪提示
- **NPC 社交可视化**：NPC 详情面板加入“云形态”阶段文本，地图面板会按现有日程显示高好感 NPC 的大致位置
- **四季节庆最小原型**：`festival_definitions.csv` 更新为春茶祭 / 灵兽祭 / 丰穰祭 / 界桥祭等第一轮可触发节庆配置
- 节日 CSV 中各传统节日在换日时自动发放礼包与 Buff（清明灵草双倍、花朝云海染色、中秋多日体力滋养、重阳菊花酒、丰收祭出售加成等），并发出 `FestivalMvpRewardEvent` 供成就等系统订阅
- 修正 `festival_definitions.csv` 中七夕与大潮祭同日冲突（七夕改为夏季第 20 天）
- 节日摊位：主地图新增 `Festival Booth` 交互点，按当日节日提供灯谜/许愿/包粽/斗茶/评选等轻量互动（每日一次）
- 节庆旅人成就：首次领取节日日切礼包解锁，奖励金币

### Technical
- **启动期数据校验**：`GameRuntime` 接入 `DataRegistry`，在初始化阶段统一登记并校验 `diary / festival / tool` 首批表
- **测试补充**：新增 `DataRegistry` 与 `DiarySystem` 数据加载测试
- **扩展回归测试**：新增覆盖日记解锁、技能分支、扩展状态存档、DataRegistry 的专门回归测试
- **架构整改**：`AudioManager::PreloadSFX()` 优先使用 `ResourceManager` 统一加载资源
- **文档更新**：`PROJECT_ROADMAP.md` 反映真实完成状态（进度 ~38% → ~58%）
- **迭代文档**：`ITERATION_PLAN_ARCH_REVIEW_2026Q2.md` 更新为"全部达成"状态
- **日切逻辑下线收口**：`DayCycleRuntime.*` 归档到 `07_归档与废弃/03_早期原型/DayCycleRuntime_legacy/`，运行时单入口统一为 `GameRuntime::OnDayChanged()` / `GameRuntime::SleepToNextMorning()`

## [0.1.0] - 2026-04-16

### Added
- Eight-directional player movement
- AABB obstacle collision
- TMX map loading and basic tile rendering
- Interactable object detection and highlighting
- Typed interactions: GatheringNode / Workstation / Storage
- Pickup item generation and auto-collection
- Inventory system prototype
- HUD and inventory text panels
- Stamina consumption and recovery
- Game clock and day progression
- Cloud sea state machine (Clear / Mist / DenseCloud / Tide)
- Debug cloud state toggle (F5)
- Config file loading (gameplay.cfg)
- Logger system
- Crop growth system with cloud density multiplier
- NPC schedule system with CSV/JSON data
- Spirit beast follow/wander behavior
- Skill system with level-up
- Festival system with seasonal events
- Workshop/tea machine processing system
- Cloud Guardian Contract system
- Dynamic Life system for NPCs
- Save/Load system
- Tutorial hints
- Heart particle effects
- Performance frame monitor

### Technical
- C++20 / SFML 3.0.2 / CMake build system
- Four-layer architecture: app / engine / domain / infrastructure
- Coding standards document (CODING_STANDARDS.md)
- Architecture guide (docs/ARCHITECTURE.md)
- Codebase guide with file placement filter (docs/CODEBASE_GUIDE.md)
- .clang-format and .editorconfig configured
