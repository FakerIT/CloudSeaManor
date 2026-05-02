# 告白 / 婚礼系统设计（Phase 1 设计稿）

> 状态：设计完成，待进入实现  
> 范围：关系终局闭环（不含美术演出细化）

## 1. 目标与边界

- 在不破坏现有 `NpcDialogueManager`、`Schedule_Data.csv`、节日与存档链路的前提下，新增关系终局系统。
- 先实现最小可玩闭环：告白触发、预约婚礼、婚礼事件、婚后状态与日常增益。
- 本阶段不改动复杂分镜，演出可先用 UI + 文本 + 状态切换完成。

## 2. 状态机

`Single -> Close -> ConfessionAvailable -> Engaged -> WeddingScheduled -> Married`

- `Single`：默认状态
- `Close`：达到亲密前置门槛（但未到告白条件）
- `ConfessionAvailable`：可触发告白
- `Engaged`：告白成功，待婚礼
- `WeddingScheduled`：婚礼已预约，等待目标日期
- `Married`：婚后状态

### 状态迁移规则

- `Single -> Close`：好感达到 `CLOSE_FAVOR_THRESHOLD`
- `Close -> ConfessionAvailable`：满足告白门槛 + 前置心事件完成
- `ConfessionAvailable -> Engaged`：告白成功
- `Engaged -> WeddingScheduled`：玩家完成婚礼预约并支付仪式消耗
- `WeddingScheduled -> Married`：到达婚礼日并完成事件
- `ConfessionAvailable -> Close`：告白失败触发冷却

## 3. 触发条件

## 3.1 告白触发

- 好感度：`favor >= 1000`（可配置）
- 心事件前置：至少完成 `h1/h2/h3`
- 时间窗口：`18:00 - 22:00`
- 天气限制：允许晴/薄雾；浓云与大潮不可触发
- 道具：需持有 `confession_token`

## 3.2 婚礼触发

- 状态为 `Engaged`
- 至少提前 2 天预约
- 日期不能与高优先级节日冲突（冲突时自动顺延）
- 需支付婚礼资源包（金币 + 礼物道具）

## 4. 失败与回退

- 告白失败后进入冷却：`CONFESSION_COOLDOWN_DAYS = 3`
- 冷却内不显示告白选项
- 失败惩罚：好感小幅下降（例如 `-30`）
- 连续失败保护：第二次失败后最少保留 `Close` 状态，不回退 `Single`

## 5. 婚礼流程

1. 玩家在关系面板选择预约日期
2. 系统校验节日冲突与 NPC 日程可用性
3. 婚礼日进入婚礼事件场景（可先使用文字演出）
4. 发放奖励（称号、家具、关系加成）
5. 状态切换为 `Married`
6. 应用婚后效果（每日小增益 + 专属对话）

## 6. 系统耦合点

- `NpcDialogueManager`
  - 增加关系状态 gating，决定是否显示告白/婚后专属台词
- `Schedule_Data.csv`
  - 婚礼日生成临时日程覆盖，事件后恢复常规日程
- 节日系统
  - 婚礼与节日冲突处理（顺延策略）
- 存档系统
  - 新增关系状态、冷却天数、婚礼预约信息持久化

## 7. 数据结构建议

- `assets/data/social/relationship_milestones.json`
  - 存放阈值、冷却、道具门槛、天气/时间窗口
- `assets/data/social/wedding_events.json`
  - 存放婚礼事件定义、奖励、冲突规则

## 8. MVP 验收

- 可对一个目标 NPC 完整跑通：告白成功 -> 预约 -> 婚礼 -> 婚后状态
- 告白失败冷却与二次尝试生效
- 婚礼与节日冲突可自动处理
- 读档后关系状态与预约信息不丢失
