# 数据驱动现状评估报告

## 范围
本报告用于固化《云海山庄》从硬编码内容迁移到表格化数据的 Phase 1 扫描结果，覆盖 `domain`、关键 `engine` 运行时文件、现有 `assets/data` 表与启动装配入口。

## 现状概览
- 项目已经存在多份运行时表：`CropTable.csv`、`TeaTable.csv`、`HungerTable.csv`、`recipes.csv`、`festival/festival_definitions.csv`、`battle/*.csv` 等。
- 启动入口 `src/engine/GameRuntime.cpp` 已负责路径解析、资源存在性验证与分散加载，但没有统一的 typed table registry。
- `src/infrastructure/DataRegistry.cpp` 原先只做目录登记与 hash 描述，未承担表加载、字段转换、唯一性校验、引用校验职责。

## 硬编码热点

### 内容型热点
- `src/domain/MainPlotSystem.cpp`
  - 章节 fallback 数据 `ch1` 到 `ch10`
  - 标题、副标题、剧情描述、章节目标、plot id 列表
- `src/domain/FestivalSystem.cpp`
  - 默认节日 roster、名称、说明、活动、奖励、预告天数
- `src/domain/CloudGuardianContract.cpp`
  - 契约卷册、条目需求、奖励说明
- `src/domain/DiarySystem.cpp`
  - 日记 ID、标题、摘要

### NPC 与社交热点
- `src/engine/GameAppNpc.cpp`
  - NPC roster、默认尺寸、颜色、初始坐标、默认 schedule fallback、地点 anchor
- `src/domain/DynamicLifeSystem.cpp`
  - NPC 人生阶段阈值、条件、职位文本、地点标识
- `src/engine/NpcDialogueManager.cpp`
  - 生日规则、节日特殊对话
- `src/domain/RelationshipSystem.cpp`
  - 告白/婚礼条件、婚后 buff、阶段文本

### 玩法数值热点
- `src/domain/ToolSystem.cpp`
  - 工具二维效果表与显示文本
- `src/domain/FestivalGameplayMvp.cpp`
  - 节日自动奖励、道具、提示文本
- `src/engine/PlayerInteractRuntime.cpp`
  - 节庆摊位奖池、小游戏奖励、鱼与许可证映射
- `include/CloudSeamanor/GameConstants.hpp`
  - 技能、节庆、成长等系统常量

## 已存在且应纳入统一注册表的数据
- `assets/data/diary/entries.csv`
- `assets/data/festival/festival_definitions.csv`
- `assets/data/skills/branches.csv`
- `assets/data/spirit_beast/*.csv`
- `assets/data/fishing/spots.csv`
- `assets/data/battle/weapon_table.csv`

## 迁移优先级
1. `diary_entries`、`festival_definitions`、`festival_rewards`
2. `npc_data`、NPC 生日/礼物/日程 profile
3. `tool_data`
4. `skill_data`、`weapon_data`、`pet_data`、`tea_data`
5. 契约、关系状态与剩余参数表

## 本轮落地
- `DataRegistry` 升级为 typed CSV registry
- `ResourceManager` 增加数据根目录解析能力
- `GameRuntime` 启动期接入 registry 校验
- `DiarySystem` 改为优先从 `assets/data/diary/entries.csv` 加载
- 补齐 `tool_data`、`npc_data`、`pet_data`、`tea_data`、`skill_data`、`weapon_data`、`festival_rewards`
