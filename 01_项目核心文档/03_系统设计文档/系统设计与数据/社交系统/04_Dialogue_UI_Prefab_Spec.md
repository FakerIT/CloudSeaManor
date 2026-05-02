# Dialogue UI Prefab Spec

## 必要组件
- 立绘显示区（左/右双槽）
- 对话文本框（支持打字机效果）
- 角色名标签
- 选项按钮组（1-4项）
- 快进/自动/历史记录按钮

## 交互流程
1. 进入事件节点时加载立绘与表情
2. 文本按打字机速度播放
3. 选择支路写入`dialogue_choice_flag`
4. 结束节点回调`SocialManager.on_dialogue_complete(event_id)`

## UI事件回调
- `on_select_option(option_id)`
- `on_skip_line()`
- `on_open_history()`
- `on_dialogue_end()`

## 性能与可用性
- 立绘资源异步加载，防止卡顿
- 文本支持多语言键值
- 键鼠与手柄操作路径一致
