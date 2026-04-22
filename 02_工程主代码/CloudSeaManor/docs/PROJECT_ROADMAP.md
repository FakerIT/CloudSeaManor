# 《云海山庄》全面提升路线图

> 文档版本：v1.0 | 日期：2026-04-21
> 目的：将项目从"架构完整，内容稀疏"提升到"可玩性丰富"状态
> 当前进度：~38% | 目标进度：~70%+

---

## 整体评估

**工程架构：优秀** — 分层设计、C++20 + SFML 3.0.2、147个单元测试、CI/CD流程、完整文档
**核心问题**：大量系统处于"代码存在，内容为空"状态

| 系统 | 代码 | 内容 | 完成度 |
|------|------|------|--------|
| 游戏引擎 | ✅ | ✅ | 90% |
| 农业系统 | ✅ | ⚠️ 缺作物视觉阶段、疾病系统 | 60% |
| 对话/社交 | ✅ | ⚠️ 仅4位NPC有对话，其余9位空 | 45% |
| 像素UI | ✅ | ⚠️ 大部分还是占位矩形 | 40% |
| 音频 | ⚠️ 框架在 | ❌ 无任何音频文件 | 0% |
| 战斗系统 | ✅ | ❌ 未接入主循环 | 30% |
| 节日系统 | ❌ | ❌ 无CSV、无数据 | 0% |
| 灵界农场 | ⚠️ 框架在 | ❌ 玩法空洞 | 20% |
| 存档系统 | ⚠️ 单槽 | ❌ 无多槽、无缩略图 | 30% |
| 成就系统 | ⚠️ CSV有4条 | ❌ 无UI、无触发 | 10% |

---

## 方向一：音频系统（ROI 最高）

> **当前状态**：`AudioManager` 路由完整，`assets/audio/` 为空
> **工作量**：极低（只需放音频文件到目录）
> **影响**：立即提升 200-300% 游戏体验

### TODO 清单

#### P0 - 立即执行（0 代码改动）

- [ ] **A1. 音频文件准备** — 在 `assets/audio/` 下创建目录结构：
  ```
  assets/audio/
  ├── bgm/
  │   ├── main_theme.ogg      （主菜单/农场）
  │   ├── spring_theme.ogg     （春季）
  │   ├── summer_theme.ogg    （夏季）
  │   ├── autumn_theme.ogg    （秋季）
  │   ├── winter_theme.ogg     （冬季）
  │   ├── battle_theme.ogg     （战斗）
  │   ├── tide_theme.ogg       （大潮）
  │   └── festival_theme.ogg   （节日）
  ├── sfx/
  │   ├── harvest.ogg          （收割）
  │   ├── plant.ogg            （种植）
  │   ├── water.ogg            （浇水）
  │   ├── dialogue_continue.ogg（对话继续）
  │   ├── level_up.ogg         （升级）
  │   ├── dialogue_choice.ogg  （选择）
  │   ├── shop_purchase.ogg    （购买）
  │   ├── gift.ogg             （送礼）
  │   ├── heart_event.ogg      （心事件触发）
  │   ├── rain.ogg             （下雨）
  │   ├── wind_mist.ogg        （薄雾风声）
  │   ├── wind_strong.ogg      （浓云风声）
  │   └── tide_magic.ogg       （大潮环境音）
  └── ambient/
      ├── farm_ambient.ogg     （农场环境音）
      └── spirit_realm.ogg     （灵界环境音）
  ```

#### P1 - 基础触发（1-2 天）

- [ ] **A2. S-19 音效池管理** — 实现 `sf::SoundBuffer` 预加载池，`playSound(id)` 从池取用
  - 文件：`src/engine/AudioManager.cpp`
  - 代码路由已就绪，只需实现 `PreloadSFX` 和 `PlaySFX`

- [ ] **A3. S-20 音效触发点接入** — 在 `InteractionSystem.cpp` 关键交互点插入调用：
  - 收割：`audio.playSound("harvest")`
  - 种植：`audio.playSound("plant")`
  - 浇水：`audio.playSound("water")`
  - 对话继续：`audio.playSound("dialogue_continue")`
  - 升级：`audio.playSound("level_up")`

#### P2 - 完整集成（3-5 天）

- [ ] **A4. B-19 SFML 音频框架完善** — 三通道独立音量（Music/BGM/SFX）
  - 文件：`src/engine/AudioManager.cpp`
  - 从 `configs/audio.json` 读取音量配置

- [ ] **A5. B-20 四季主题 BGM 切换** — 季节变更时自动切换 BGM
  - 文件：`src/engine/DayCycleRuntime.cpp`
  - 时段变奏（清晨/午后/傍晚/夜晚）

- [ ] **A6. B-21 云海天气环境音** — 根据天气状态播放对应环境音
  - 文件：`src/engine/CloudSystem.cpp`
  - 晴=无、薄雾=轻柔风声、浓云=呼啸、大潮=神秘空灵

#### P3 - 深度内容（5-10 天）

- [ ] **A7. C-6 场景 BGM 完整集成** — 每个场景独立 BGM（农舍/灵界浅层/灵界中层/灵界深层/商店/节日）
  - 场景切换时淡入淡出

- [ ] **A8. C-7 节日 BGM（12 首）** — 每个节日专属 BGM
  - 春节/清明/端午/七夕/中秋/重阳/冬至/大潮祭/茶文化节/丰收祭/万灯节/新年派对

- [ ] **A9. C-8 大潮专属 BGM + 音效** — 大潮期间独特史诗感 BGM + 专属音效

---

## 方向二：战斗系统接入（解锁核心玩法）

> **当前状态**：`BattleField`、`BattleManager`、`BattleUI` 完整，CSV 数据齐全，但从未被触发
> **工作量**：中（3-5 天 glue 代码）
> **影响**：解锁游戏核心战斗玩法，大幅提升可玩时长

### TODO 清单

#### P0 - 立即执行

- [ ] **B1. 实现 CSV 加载函数** — 在 `BattleField.cpp` 中实现三个加载函数：
  - `LoadSpiritTableFromCsv()` — 解析 `assets/data/battle/spirit_table.csv`
  - `LoadSkillTableFromCsv()` — 解析 `assets/data/battle/skill_table.csv`
  - `LoadZoneTableFromCsv()` — 解析 `assets/data/battle/zone_table.csv`
  - 参考 `战斗系统集成指南.md` Step 4.1-4.3 的模板代码

- [ ] **B2. 战斗系统初始化** — 在 `GameApp::Run()` 中：
  - 创建 `battle_manager_` 实例
  - 调用 `battle_manager_.Initialize(&cloud_system_)`
  - 加载 CSV 数据到 `battle_field_`

- [ ] **B3. 战斗触发检测** — 在 `GameApp::Update()` 中实现 `CheckBattleTrigger_()`：
  - 检测玩家与污染灵体的碰撞距离
  - 接近时调用 `battle_manager_.EnterBattle(zone, {spirit_ids})`
  - 设置 `in_battle_mode_ = true`

#### P1 - 主循环集成

- [ ] **B4. GameApp Update 分支** — 修改 `GameApp::Update()`：
  ```cpp
  if (in_battle_mode_) {
      battle_manager_.Update(delta_seconds, player_pos.x, player_pos.y);
      if (!battle_manager_.IsInBattle()) in_battle_mode_ = false;
      return;
  }
  ```

- [ ] **B5. GameApp Render 分支** — 修改 `GameApp::Render()`：
  ```cpp
  if (in_battle_mode_) {
      DrawBattleBackground(window);
      DrawPollutedSpirits(window);
      battle_manager_.GetUI().Draw(window);
  }
  ```

- [ ] **B6. 战斗输入处理** — 在 `GameApp::HandleInput()` 中：
  - Q/W/E/R 释放技能
  - Escape 暂停
  - X 撤退

#### P2 - 完善功能

- [ ] **B7. 灵兽系统对接** — 实现 `LoadPartnersFromSpiritBeasts()`
  - 从 `SpiritBeastSystem` 获取出战灵兽数据
  - 填充 `BattlePartner` 结构

- [ ] **B8. 战斗后奖励** — 实现 `ProcessVictory_()`
  - 更新灵兽羁绊好感
  - 发放 SpiritGuard 技能经验

- [ ] **B9. 战斗存档** — 在 `GameAppSave.cpp` 中：
  - 战斗中存档自动结算胜利
  - 保存战斗可用状态

- [ ] **B10. BOSS 多阶段逻辑** — 实现 `云海潮灵` 和 `太初浊源` 的多阶段 AI

#### P3 - 打磨

- [ ] **B11. 战斗粒子特效** — 复用现有 `ParticleSystem`，为技能添加视觉反馈
- [ ] **B12. 战斗UI动画过渡** — 技能按钮按下效果、净化条减少动画、胜利结算动画
- [ ] **B13. 灵体刷新机制** — 灵界区域定时生成新的污染灵体

---

## 方向三：像素美术资源

> **当前状态**：`assets/sprites/` 下只有 3 个 atlas JSON 文件，无实际图片
> **工作量**：高（需要大量像素美术资源）
> **影响**：从"能玩"到"好看"的关键一跃

### TODO 清单

#### P0 - 最优先（影响最大）

- [ ] **C1. 玩家角色精灵** — 16x16 或 32x32 像素精灵
  - `assets/sprites/characters/player_main.atlas.json` 有 atlas 定义但无图片
  - 需要：`idle_anim_4frame.png`、`walk_anim_6frame.png`、`tool_anim_4frame.png`

- [ ] **C2. 农作物精灵图集** — `items_crop.atlas.json` 有 atlas 但无图片
  - 每种作物需要 5 个生长阶段图片（种子/幼苗/成长/成熟/可收获+金色边框）
  - 30 种作物 × 5 阶段 = 150 张图片（可批量绘制风格统一的一组）

#### P1 - 高频 UI

- [ ] **C3. 像素 UI 边框组件** — `ui_main.atlas.json` 有 atlas 但无图片
  - 像素边框绘制：8x8 角块、1x8 横边、8x1 竖边
  - 按钮样式：默认/悬停/按下三种状态
  - 对话框样式：带像素人像框
  - 进度条样式：体力/生命/经验条
  - 工具栏样式：选中格高亮

- [ ] **C4. 物品图标集** — 工具、种子、作物、加工品、灵物的 16x16 像素图标
  - 可使用社区像素资源包

#### P2 - NPC 和灵兽

- [ ] **C5. NPC 精灵** — 13 位 NPC 各需要一套精灵
  - 每个 NPC：idle / walk_left / walk_right / walk_up / walk_down / interact
  - 优先完成核心 4 位 NPC（acha/lin/wanxing/xiaoman）

- [ ] **C6. 灵兽精灵** — 20 只灵兽各需要一套精灵
  - 每只：idle_anim / follow_anim / wander_anim / interact_anim
  - 可用 AI 生成 + 手工调整

#### P3 - 战斗和装饰

- [ ] **C7. 战斗敌人精灵** — 29 种污染灵体 + 2 个 BOSS
  - 每种需要 idle_anim / attack_anim / polluted_effect / purification_vanish_anim

- [ ] **C8. 装饰物和建筑** — 温室、鸡舍、井、围栏、传送门等建筑装饰
- [ ] **C9. 地图瓦片集** — 灵界地图需要新的瓦片集（淡紫/银白/云雾风格）
- [ ] **C10. 节日装饰** — 12 个节日各自的主题装饰物

---

## 方向四：NPC 内容深化

> **当前状态**：4 位 NPC（acha/lin/wanxing/xiaoman）有基础对话，其余 9 位空
> **工作量**：中-高（大量文案写作）
> **影响**：让 NPC 从"可对话"变成"有故事"

### TODO 清单

#### P0 - 立即激活

- [ ] **D1. 心事件触发接入** — S-12 实现 `npc_daily_acha_h2.json` 触发逻辑
  - 好感 ≥ 200 时触发 h2 心事件（代码已就绪，只需接入）
  - 文件：`src/engine/NpcDialogueManager.cpp`

- [ ] **D2. S-11 修复 PLAYER_NAME 变量** — 将 `PLAYER_NAME` 改为 `$[PLAYER_NAME]`
  - 文件：`assets/data/dialogue/npc_heart_acha_h2.json`

- [ ] **D3. S-14 NPC 对话冷却** — 同一天多次对话显示 `"今天已经聊过了"`

#### P1 - 扩展对话内容

- [ ] **D4. A-2 核心 NPC 日常对话扩至 20 条** — 4 位 NPC 各补充至 20 条
  - 覆盖：时段（早/中/晚）× 季节（春夏秋冬）× 好感度（0-50/50-100/100+）

- [ ] **D5. A-3 心事件 h1 系统（4 NPC）** — 好感 ≥ 50 时触发 h1 事件
  - 4 位 NPC 各 1 个 h1 事件，共 4 个

- [ ] **D6. A-4 心事件 h3 系统（4 NPC）** — 好感 ≥ 600 时触发 h3 事件
  - 4 位 NPC 各 1 个 h3 事件，共 4 个

- [ ] **D7. A-8 对话特殊标记扩展** — 支持 `$[WEATHER]` `$[SEASON]` `$[DAY]` `$[NPC_NAME]` `$[ITEM_NAME]`
  - 文件：`src/engine/DialogueEngine.cpp`

- [ ] **D8. A-5 NPC 特殊对话** — 下雨/生病/生日/节日特殊对话

#### P2 - 社交系统

- [ ] **D9. A-6 送礼反馈文案** — 根据礼物价值生成不同文案 + 爱心粒子
  - 喜爱/喜欢/讨厌三档反馈

- [ ] **D10. A-7 社交面板 UI** — 显示好感度/心进度/已触发事件列表
  - 文件：`src/engine/PixelInventoryGrid.cpp`（添加社交标签页）

#### P3 - 完整接入

- [ ] **D11. C-1 剩余 9 位 NPC 完整接入**
  - 日程 CSV 解析、NPC 移动插值、日常对话池
  - 9 位 NPC：song/yu/mo/qiao/he/ning/an/shu/yan

- [ ] **D12. C-2 完整心事件链** — 9 位 NPC × 8 事件 = 72 个心事件
  - 长期内容，可按优先级逐步完成

---

## 方向五：节日活动系统

> **当前状态**：0% — 无 CSV、无数据、无代码
> **工作量**：中（每个节日独立实现）
> **影响**：创造季节性惊喜，大幅提升回访动力

### TODO 清单

#### P0 - 框架搭建

- [ ] **E1. 节日数据 CSV** — 创建 `assets/data/festival/festival_definitions.csv`：
  ```
  id,name,season,day,description,special_item,reward_type,reward_value
  spring_festival,春节,winter,1,新年第一天,RedEnvelope,gold,200
  lantern_festival,元宵节,winter,15,正月十五赏灯,StickyRiceBall,stamina,30
  flower_festival,花朝节,spring,3,百花生日,FlowerBasket,social,10
  ...
  ```
  12 个节日：春节、元宵节、花朝节、清明、端午、七夕、中秋、重阳、冬至、大潮祭、茶文化节、丰收祭

- [ ] **E2. 节日触发系统** — 在 `GameClock` 中检测日期变化
  - 文件：`src/engine/GameClock.cpp`
  - 节日当天：触发 `FestivalActiveEvent`

#### P1 - 单个节日实现（每个 1-2 天）

- [ ] **E3. 春节实现** — 放烟花（粒子特效）、贴春联（装饰物）、收红包（随机金币）、NPC 特殊对话
- [ ] **E4. 元宵节实现** — 猜灯谜（对话互动小游戏）、吃汤圆（体力恢复道具）
- [ ] **E5. 花朝节实现** — 赏花（云海变色）、送花束（好感道具）、NPC 花语对话
- [ ] **E6. 清明节实现** — 扫墓祭祖（剧情触发）、踏青（双倍采集）
- [ ] **E7. 端午节实现** — 包粽子（工坊迷你游戏）、赛龙舟（速度小游戏）
- [ ] **E8. 七夕节实现** — 观星（观星台剧情）、许愿（随机奖励）、NPC 告白机会
- [ ] **E9. 中秋节实现** — 赏月（满月特效）、吃月饼（长期buff道具）、团圆饭
- [ ] **E10. 重阳节实现** — 登高（观星台特殊事件）、饮菊花酒（buff）
- [ ] **E11. 冬至节实现** — 吃饺子（体力恢复）、NPC 团聚对话、极夜天气
- [ ] **E12. 大潮祭实现** — 最高优先级：最强 BOSS 战 + 传说奖励 + 专属 BGM
- [ ] **E13. 茶文化节实现** — 制茶大赛（品质评分）、特殊茶叶配方
- [ ] **E14. 丰收祭实现** — 展示当日收成、评选最佳作物、额外卖价

#### P2 - 节日基础建设

- [ ] **E15. 节日 BGM 切换** — 节日期间自动播放 `festival_theme.ogg`
- [ ] **E16. 节日装饰物** — 节日期间地图出现对应装饰物（烟花/灯笼/花朵等）
- [ ] **E17. 节日商店** — 节日专属商店出售限定物品

---

## 方向六：灵界农场深化

> **当前状态**：`SpiritRealmGameplay.cpp` 和 `SpiritWorldMap.cpp` 不存在，无玩法
> **工作量**：中
> **影响**：将"种田"变为有策略深度的玩法

### TODO 清单

#### P0 - 基础入口

- [ ] **F1. A-20 灵界入口** — 在主地图设计传送门对象（`type = "spirit_gateway"`）
  - 文件：`prototype_farm.tmx`
  - 按 E 键触发场景切换动画

- [ ] **F2. A-21 灵界浅层 TMX** — 淡紫/银白色调地图，30×20 格
  - 云雾粒子背景
  - 灵草采集点
  - 灵兽游荡区域

- [ ] **F3. A-22 灵物采集系统** — `InteractWithSpiritPlant` 方法
  - 消耗体力
  - 产出 `spirit_grass`/`cloud_dew`/`spirit_dust`
  - 采集冷却 1 小时游戏时间

#### P1 - 灵物经济

- [ ] **F4. A-23 灵物物品表** — 完善 `assets/data/SpiritItemTable.csv`
- [ ] **F5. B-15 灵物加工链** — 灵物 → 加工 → 高级产品
  - `spirit_grass ×3 → spirit_essence`
  - `cloud_dew ×2 + spirit_dust → cloud_elixir`
  - `cloud_elixir + TeaPack → immortal_tea`（传说品质）

- [ ] **F6. A-24 大潮灵界加成** — 大潮时灵物掉落率 ×2，稀有点出现

#### P2 - 灵界战斗

- [ ] **F7. B-11 灵界中层战斗** — 引入低难度灵云兽
  - 按 J 键使用灵镰刀攻击
  - 击败掉落灵尘

- [ ] **F8. B-13 灵气镰刀工具** — 高效采集工具，用灵尘制作

- [ ] **F9. 战斗系统接入灵界** — 在灵界中触发战斗，击败污染灵体

#### P3 - 灵界深度

- [ ] **F10. B-12 灵气阶段可视化** — 5 阶段视觉变化（荒废→太初）
- [ ] **F11. B-14 双向传送** — 主地图 ↔ 灵界，大潮时传送门发光
- [ ] **F12. C-5 灵界深层** — 高难度挑战区域，暗色调，有毒云雾，首领

---

## 方向七：成就/契约系统

> **当前状态**：`AchievementTable.csv` 有 4 条数据，无 UI，无触发
> **工作量**：低-中
> **影响**：给玩家目标感和里程碑满足感

### TODO 清单

- [ ] **G1. A-30 任务面板功能完善** — 实现 `QuestManager` 类
  - 从 `QuestTable.csv` 加载任务
  - 支持状态：未接取/进行中/已完成
  - 完成后触发奖励

- [ ] **G2. 成就系统接入** — 实现 `AchievementManager` 类
  - 从 `AchievementTable.csv` 加载
  - 监听游戏事件（收获/送礼/建造等）
  - 达成时显示成就弹窗 + 奖励发放

- [ ] **G3. 成就 UI** — 成就面板显示已获得/未获得成就
  - 使用 `PixelUiPanel` 像素风格

- [ ] **G4. 扩展成就表** — 从 4 条扩展到 20+ 条
  - 农场类：首次收获、连续收获 7 天等
  - 社交类：首次送礼、达成 1 心等
  - 建筑类：建造第一个建筑等
  - 战斗类：首次净化、击败 BOSS 等
  - 探索类：进入灵界、采集稀有灵物等

---

## 方向八：存档系统增强

> **当前状态**：单槽位存档，无缩略图
> **工作量**：低-中
> **影响**：改善玩家体验，支持多游戏进度

### TODO 清单

- [ ] **H1. S-15 多槽位存档** — 创建 `SaveSlotManager` 类
  - 支持 3 个存档槽
  - 每个槽存储：存档时间、游玩天数、季节、缩略图路径

- [ ] **H2. 存档缩略图** — 存档时截图保存为缩略图
  - 读档选择界面显示缩略图

- [ ] **H3. 存档信息显示** — 显示游玩天数、季节、金币、存档时间

- [ ] **H4. 自动存档** — 每日结束时自动存档
- [ ] **H5. 云存档** — 未来支持（预留接口）

---

## 方向九：结婚/好感终局

> **当前状态**：好感度系统存在，无婚礼、无婚后内容
> **工作量**：中
> **影响**：给玩家情感投入的终局回报

### TODO 清单

- [ ] **I1. 告白系统** — 好感度 10 心时解锁告白选项
  - 对话中出现告白选项
  - 成功/失败分支

- [ ] **I2. 婚礼仪式** — 告白成功后触发婚礼场景
  - 全 NPC 出席
  - 特殊 BGM
  - 婚礼对话

- [ ] **I3. 婚后内容** — 婚后 NPC 称呼变化（名字后加"相公"/"娘子"）
  - 婚后每日特殊对话
  - 婚后任务（装修房子等）

---

## 方向十：玩法循环深化

> **当前状态**：基础种田循环存在，深度不足
> **工作量**：中
> **影响**：增加玩家长期留存动力

### TODO 清单

#### 农业深化

- [ ] **J1. A-13 作物生长视觉阶段** — 5 个视觉阶段区分（棕色小点→金色边框+光效）
  - 文件：`src/engine/WorldRenderer.cpp`

- [ ] **J2. A-10 作物品质系统接入** — Normal/Fine/Rare/Spirit/Holy 五档品质

- [ ] **J3. A-11 洒水器系统** — 放置后自动为周围地块浇水

- [ ] **J4. A-12 肥料系统** — 普通/优质肥料，不同生长速度加成

- [ ] **J5. A-14 温室系统** — 突破季节限制，所有作物可生长

- [ ] **J6. B-10 作物疾病虫害** — 10% 每日感染概率，需要治疗

- [ ] **J7. C-4 四季完整作物表** — 春 8/夏 8/秋 8/冬 6 种

#### 经济深化

- [ ] **J8. A-15 玩家金币系统** — `int gold_ = 500` 初始资金
- [ ] **J9. A-16 商店 NPC 和界面** — 皮埃尔商店
- [ ] **J10. A-17 收购商系统** — 卖货界面，品质影响收购价
- [ ] **J11. A-18 物品价格表配置化** — `PriceTable.csv`
- [ ] **J12. A-19 邮购系统** — 预购种子，次日送达

#### 建筑深化

- [ ] **J13. B-22 主屋升级系统** — 帐篷→小木屋→砖房→大宅
- [ ] **J14. B-23 工坊升级** — 解锁更多加工槽
- [ ] **J15. B-24 茶园系统** — 与普通农田分离的特殊作物区
- [ ] **J16. C-9 鸡舍/畜棚系统** — 动物产出系统
- [ ] **J17. C-10 客栈经营系统** — NPC 来访、远程订单
- [ ] **J18. C-11 建筑装饰系统** — 自定义家园外观

---

## 执行顺序建议

### 第一轮（1-2 周）：最大化影响力

```
优先级  任务                              预计时间  影响力
P0-A1   音频文件准备（下载/生成占位）       1天       ⭐⭐⭐⭐⭐
P0-A3   音效触发点接入                     0.5天     ⭐⭐⭐⭐⭐
P0-B1   CSV 加载函数                       1天       ⭐⭐⭐⭐
P0-B2   战斗系统初始化                     0.5天     ⭐⭐⭐⭐
P0-B3   战斗触发检测                       1天       ⭐⭐⭐⭐
P1-B4   GameApp Update 分支               0.5天     ⭐⭐⭐⭐
P1-B5   GameApp Render 分支               0.5天     ⭐⭐⭐⭐
P1-B6   战斗输入处理                       0.5天     ⭐⭐⭐
P1-D1   心事件触发接入                     1天       ⭐⭐⭐
P1-D2   PLAYER_NAME 变量修复             0.5天     ⭐⭐⭐
P1-D3   NPC 对话冷却                       0.5天     ⭐⭐⭐
P2-G1   任务面板功能完善                   2天       ⭐⭐⭐
P2-H1   多槽位存档                         2天       ⭐⭐⭐
P2-E1   节日数据 CSV                       1天       ⭐⭐
P2-E2   节日触发系统                       1天       ⭐⭐⭐
```

### 第二轮（2-4 周）：内容填充

```
优先级  任务                              预计时间  影响力
P1-A5   四季 BGM 切换                     2天       ⭐⭐⭐
P1-D4   核心 NPC 对话扩至 20 条            3天       ⭐⭐⭐
P1-D5   心事件 h1 系统                     3天       ⭐⭐⭐
P1-D6   心事件 h3 系统                     3天       ⭐⭐
P1-F1   灵界入口                           1天       ⭐⭐⭐
P1-F2   灵界浅层 TMX                      2天       ⭐⭐⭐
P1-F3   灵物采集系统                       1天       ⭐⭐⭐
P2-E3-E6 春节/元宵/花朝/清明实现          每节1-2天  ⭐⭐⭐
P2-J1   作物生长视觉阶段                   2天       ⭐⭐⭐
P2-J2   作物品质系统                       1天       ⭐⭐
P2-J8   玩家金币系统                       1天       ⭐⭐⭐
P2-J9   商店界面                           2天       ⭐⭐⭐
P2-B7   灵兽系统对接                       1天       ⭐⭐⭐
P2-B8   战斗后奖励                         1天       ⭐⭐⭐
```

### 第三轮（4-8 周）：深度内容

```
优先级  任务                              预计时间  影响力
P2-C1   玩家角色精灵                       3天       ⭐⭐⭐⭐
P2-C2   农作物精灵图集                     5天       ⭐⭐⭐
P2-C3   像素 UI 边框组件                   3天       ⭐⭐⭐⭐
P2-A6   云海天气环境音                     1天       ⭐⭐⭐
P3-D11  剩余 9 位 NPC 接入                5天       ⭐⭐
P3-E7-E14 七夕/中秋/重阳/冬至/大潮等实现  每节2天   ⭐⭐⭐
P3-F4   灵物加工链                         2天       ⭐⭐⭐
P3-F7   灵界中层战斗                       3天       ⭐⭐⭐
P3-I1   告白系统                           2天       ⭐⭐⭐
P3-I2   婚礼仪式                           3天       ⭐⭐⭐
P3-J3   洒水器系统                         2天       ⭐⭐
P3-J13  主屋升级系统                       3天       ⭐⭐
P3-J16  鸡舍/畜棚系统                      5天       ⭐⭐
```

---

## 快速启动建议

如果你想立即开始，按这个顺序：

1. **今天**：下载音频文件到 `assets/audio/`（零代码工作量）
2. **第 2 天**：实现 S-19 音效池 + S-20 音效触发点（1 天代码）
3. **第 3 天**：实现 CSV 加载函数 + 战斗系统初始化（1.5 天）
4. **第 4 天**：GameApp Update/Render 分支 + 战斗输入（1.5 天）
5. **第 5 天**：战斗触发检测 + 灵兽对接（1 天）

**5 天后，你就能在游戏中真正触发战斗了。**

---

## 附录：关键文件索引

| 文件 | 作用 | 状态 |
|------|------|------|
| `src/engine/GameApp.cpp` | 主循环，战斗集成点 | 已读 |
| `src/engine/AudioManager.cpp` | 音频管理 | 已读 |
| `src/engine/DialogueEngine.cpp` | 对话引擎 | 已读 |
| `src/engine/NpcDialogueManager.cpp` | NPC 对话管理 | 已读 |
| `src/engine/BattleManager.cpp` | 战斗管理器 | 已读 |
| `src/engine/BattleField.cpp` | 战场逻辑 | 已读 |
| `src/engine/BattleUI.cpp` | 战斗 UI | 已读 |
| `src/engine/SpiritBeastSystem.cpp` | 灵兽 AI | 已读 |
| `src/engine/GameAppSave.cpp` | 存档系统 | 已读 |
| `docs/design/battle/战斗系统集成指南.md` | 集成文档 | 已读 |
| `docs/100_TASKS_DETAILED.md` | 100 项任务清单 | 已读 |
| `assets/data/battle/*.csv` | 战斗数据 | 已读 |
| `assets/data/daily_dialogue/*.json` | 日常对话 | 已读 |
| `assets/data/dialogue/npc_heart_*.json` | 心事件 | 已读 |
| `assets/data/AchievementTable.csv` | 成就表 | 已读 |
| `assets/data/QuestTable.csv` | 任务表 | 已读 |
| `assets/data/NPC_Texts.json` | NPC 文本 | 已读 |
