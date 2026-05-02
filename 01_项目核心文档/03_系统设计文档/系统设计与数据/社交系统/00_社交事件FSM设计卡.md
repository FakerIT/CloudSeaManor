# 社交事件FSM设计卡

## 状态机核心
- Idle：空闲待机
- ScheduledMove：按日程移动
- AnchorReady：到达事件锚点并等待触发
- EventRunning：事件进行中
- EventPaused：事件中断挂起
- EventCompleted：事件完成并写入存档

## 触发器结构
- 时间：季节/日期/时段
- 天气：晴朗/薄雾/浓云海/大潮
- 地点：地图ID + 区域ID
- 前置条件：好感心级、建筑等级、任务标记

## 中断处理规则
- 体力耗尽、强制睡眠、切场景失败 -> 进入EventPaused
- 次日满足条件时优先尝试Resume
- 超过N天未恢复则回退到最近稳定节点

## 存档字段建议
- event_id
- event_state（pending/running/paused/completed）
- current_node_id
- last_trigger_game_day
