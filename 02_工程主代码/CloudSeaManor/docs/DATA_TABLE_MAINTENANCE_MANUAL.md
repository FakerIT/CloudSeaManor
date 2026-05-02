# 数据表维护手册

## 适用对象
策划、关卡设计、系统设计与程序协作人员。

## 编辑原则
1. 改数值优先改表，不先改 C++。
2. 新增实体必须先分配全局唯一 `Id`。
3. 平铺字段写 CSV；一对多列表写 JSON 数组字符串。
4. 中文文本直接写入表格；多行文本在 JSON 字符串中使用 `\n`。
5. 关联字段必须引用真实存在的 ID，禁止留“待定”“todo”。

## 推荐流程
1. 在对应目录创建或编辑表文件，例如 `assets/data/npc/npc_data.csv`。
2. 按既有表头补行，不随意改列名顺序。
3. 若新增列，先同步更新 `docs/DATA_TABLE_SCHEMAS.md`。
4. 启动游戏或运行测试，查看 `DataRegistryCheck` 日志。
5. 若出现缺字段、重复 ID、坏引用，先修表再修代码。

## 目录约定
- `assets/data/npc/`：NPC 主表、生日、阶段 profile、对话 profile
- `assets/data/pet/`：灵宠/灵兽主表
- `assets/data/tea/`：茶品与加工相关表
- `assets/data/skills/`：技能与技能分支
- `assets/data/weapons/`：武器与强化参数
- `assets/data/festival/`：节日定义、节日奖励、节日装饰
- `assets/data/diary/`：日记与回顾类条目
- `assets/data/tool/`：工具分级和数值

## 常见错误
- 使用重复 `Id`
- 在 CSV 单元格里直接写未转义逗号
- JSON 数组少了双引号，例如 `[tea_a, tea_b]`
- 引用不存在的技能、物品、节日或 NPC
- 将运行规则和剧情文本混在同一列里

## 新增实体示例
### 新增 NPC
1. 在 `npc_data.csv` 增加一行
2. 填写 `DialogueProfileId`、`ScheduleProfileId`、`GiftProfileId`
3. 补对应对话/日程/礼物配置
4. 启动后检查 NPC 是否被成功索引

### 新增节日
1. 在 `festival_definitions.csv` 增加基础定义
2. 在 `festival_rewards.csv` 增加奖励与提示文本
3. 若需要视觉效果，再补节日装饰配置
4. 验证预告天数、HUD 提示和奖励发放

### 新增工具阶段
1. 在 `tool_data.csv` 增加对应 `ToolType + Tier`
2. 补齐数值列，不留空
3. 若新增了全新工具类型，再同步程序枚举和 UI 图标

## 验收建议
- 改完表后至少做一次启动校验
- 关键表改动后执行测试目标
- 大批量改表时优先拆分为一类实体一个提交
