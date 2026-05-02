# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Phase 6**: 7天/28天留存循环系统
- **Phase 7**: 系统规则设计落地
  - 茶叶加工链 MVP 打通
  - 食谱加工系统 MVP 接入
  - 净化回流庄园可视化
  - 社交好感可见化（云形态表达）
  - NPC日程可视化
  - 四季节庆第一轮落地
- **Phase 8**: 深度玩法扩展
  - 行为记忆核心数据模型
  - 首批行为事件接线（制茶/探索/社交/生态/茶灵）
  - 动态对话钩子 MVP
  - 动态邮件 MVP
  - 茶灵图鉴状态与奖励骨架
  - 茶灵解锁判定 MVP
  - 茶灵现身演出与茶室展示位
  - 庄园生态基础状态与日切结算
  - 生态输入接线（种植/净化/装饰）
  - 生态反馈 MVP
  - 祥瑞事件 MVP
- **Phase 9**: NPC 动态发展系统
  - NPC 动态发展核心状态与存档骨架
  - 阶段条件判定与"双轨触发"规则
  - 首批关键 NPC 发展树配置
  - 阶段变化事件（邮件通知 + 简短演出）
  - 日程与地图存在感切换
  - 住所变化与建筑层显隐/替换
  - 阶段对话库与回顾式台词
  - 居所内可互动物品增量
  - NPC 之间的友谊/师徒关系
  - 流派/玩家行为对发展方向的影响
  - 云生完整原型线（从流浪者到店主）
- **Phase 10**: 工程治理
  - 数据表字段命名宪章落地
  - DataRegistry 引用完整性校验增强
  - 表格导出与校验脚本
  - 四层架构红线扫描
  - 存档版本迁移链与旧档回归
  - 渐进解锁路线固化
  - 节日/剧情/成熟日冲突规避
  - 文档-代码一致性检查清单
  - 文本与资源按需加载治理
  - DIY 与低配平台性能基线
  - NPC 动态发展节奏保护
- **Phase 11**: 玩法扩展与产品化增强
  - 灵气可视化与潮汐预报
  - 战斗"采灵"动作
  - 灵兽灵化工具/座驾原型
  - 灵茶占卜系统
  - 庄园茶会 MVP
  - 灵茶日记自动记事
  - 异步陪伴模式预留
  - 动态物品描述系统
  - 内容管道优先级规划
  - 系统开关组与沉浸式模式
  - 国际化基础设施预留
  - 里程碑重排与版本节奏校准
- **NPC 对话扩展**
  - 核心4位 NPC 对话扩至20条
  - 剩余9位 NPC 日常对话创建
  - 婚后内容深化
  - 心事件 h1 系统
  - 心事件 h3 系统
  - 特殊对话扩展（天气/节日/生日）
- **灵界深层系统**
  - F12 高难度挑战区域（3层级、毒云、首领）
- **像素 UI 组件增强**
  - 按钮状态绘制
  - 进度条渲染
  - 工具栏高亮
  - NPC头像框

### Fixed
- CMake 资源复制路径问题（使用绝对路径）
- 编译警告清理（命名空间限定符、前向声明）

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
