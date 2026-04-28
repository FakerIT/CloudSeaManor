# 《云海山庄》全面提升路线图

> 文档版本：v1.2 | 日期：2026-04-28
> 目的：将项目从"架构完整，内容稀疏"提升到"可玩性丰富"状态
> **当前进度：~62%** | 目标进度：~70%+

---

## 整体评估

**工程架构：优秀** — 分层设计、C++20 + SFML 3.0.2、147个单元测试、CI/CD流程、完整文档
**核心问题**：大部分系统已完成集成，内容填充持续完善中

| 系统 | 代码 | 内容 | 完成度 |
|------|------|------|--------|
| 游戏引擎 | ✅ | ✅ | 90% |
| 农业系统 | ✅ | ✅ 作物视觉/工具集成/肥料系统完善 | 85% |
| 工具系统 | ✅ | ✅ ToolSystem与FarmingSystem集成 | 90% |
| 对话/社交 | ✅ | ✅ 仅4位NPC有对话，其余9位空 | 45% |
| 像素UI | ✅ | ✅ 大部分还是占位矩形 | 40% |
| 音频 | ✅ 框架在 | ⚠️ 无实际音频文件 | 30% |
| 战斗系统 | ✅ | ✅ 已接入主循环，含种子掉落 | 60% |
| 节日系统 | ✅ | ✅ 12个节日完整实现 | 95% |
| 灵界农场 | ✅ 框架在 | ✅ 玩法已完善 | 80% |
| 存档系统 | ✅ 多槽位 | ✅ 有缩略图/元数据 | 85% |
| 成就系统 | ✅ | ✅ 20+成就已实现 | 80% |
| 经济系统 | ✅ | ✅ PriceTable完善 | 85% |

---

## 方向一：音频系统（ROI 最高）

> **当前状态**：`AudioManager` 路由完整，已补齐 `assets/audio/` 目录与占位音频文件（可随时替换为正式资源）
> **工作量**：极低（只需放音频文件到目录）
> **影响**：立即提升 200-300% 游戏体验

### TODO 清单

#### P0 - 立即执行（0 代码改动）

- [x] **A1. 音频文件准备** — 在 `assets/audio/` 下创建目录结构（已就绪，当前为占位音频）：
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

- [x] **A2. S-19 音效池管理** — 实现 `sf::SoundBuffer` 预加载池，`playSound(id)` 从池取用
  - 文件：`src/engine/AudioManager.cpp`
  - 代码路由已就绪，只需实现 `PreloadSFX` 和 `PlaySFX`

- [x] **A3. S-20 音效触发点接入** — 在 `InteractionSystem.cpp` 关键交互点插入调用：
  - 收割：`audio.playSound("harvest")`
  - 种植：`audio.playSound("plant")`
  - 浇水：`audio.playSound("water")`
  - 对话继续：`audio.playSound("dialogue_continue")`
  - 升级：`audio.playSound("level_up")`

#### P2 - 完整集成（3-5 天）

- [x] **A4. B-19 SFML 音频框架完善** — 三通道独立音量（Music/BGM/SFX）
  - 文件：`src/engine/AudioManager.cpp`
  - 从 `configs/audio.json` 读取音量配置

- [x] **A5. B-20 四季主题 BGM 切换** — 季节变更时自动切换 BGM
  - 文件：`src/engine/GameRuntime.cpp`（原 `DayCycleRuntime.*` 已归档至 `07_归档与废弃/03_早期原型/DayCycleRuntime_legacy/`）
  - 时段变奏（清晨/午后/傍晚/夜晚）

- [x] **A6. B-21 云海天气环境音** — 根据天气状态播放对应环境音
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

> **当前状态**：`BattleField`、`BattleManager`、`BattleUI` 完整，CSV 数据齐全，已接入主循环
> **新增**：种子掉落系统（SeedDropTable）+ Victory 自动结算
> **工作量**：已完成 ✅
> **影响**：已解锁游戏核心战斗玩法

### TODO 清单

#### P0 - 立即执行

- [x] **B1. 实现 CSV 加载函数** — `LoadSpiritTableFromCsv()` / `LoadSkillTableFromCsv()` / `LoadZoneTableFromCsv()`
- [x] **B2. 战斗系统初始化** — `BattleManager::Initialize()` + CSV 数据加载
- [x] **B3. 战斗触发检测** — `TryEnterBattleByPlayerPosition()` + J 键入口

#### P1 - 主循环集成

- [x] **B4. GameApp Update 分支** — 战斗模式冻结主世界更新
- [x] **B5. GameApp Render 分支** — `runtime_.RenderBattle(window)`
- [x] **B6. 战斗输入处理** — Q/W/E/R 技能 + Escape 暂停 + X 撤退

#### P2 - 完善功能

- [x] **B7. 灵兽系统对接** — `LoadPartnersFromSpiritBeasts()`
- [x] **B8. 战斗后奖励** — `ProcessVictory_()` + `reward_callbacks_` 发放
- [x] **B9. 战斗存档** — 战斗中存档自动结算
- [x] **B10. BOSS 多阶段逻辑** — 多阶段 AI 已实现

#### P3 - 打磨

- [x] **B11. 战斗粒子特效** — 复用 ParticleSystem
- [x] **B12. 战斗UI动画过渡** — 技能按钮效果、结算动画
- [x] **B13. 灵体刷新机制** — 定时生成新灵体

#### 新增功能（2026-04-28）

- [x] **B14. Victory 自动结算** — 结算时间到达后自动发放奖励
- [x] **B15. 种子掉落系统** — `SeedDropTable.csv` + `RollSeedDrops_()`

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

- [x] **D1. 心事件触发接入** — S-12 实现 `npc_daily_acha_h2.json` 触发逻辑
  - 好感 ≥ 200 时触发 h2 心事件（代码已就绪，只需接入）
  - 文件：`src/engine/NpcDialogueManager.cpp`

- [x] **D2. S-11 修复 PLAYER_NAME 变量** — 将 `PLAYER_NAME` 改为 `$[PLAYER_NAME]`
  - 文件：`assets/data/dialogue/npc_heart_acha_h2.json`

- [x] **D3. S-14 NPC 对话冷却** — 同一天多次对话显示 `"今天已经聊过了"`

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

> **当前状态**：12 个节日完整实现，CSV 数据加载，事件化触发
> **工作量**：已完成 ✅
> **影响**：季节性惊喜，回访动力

### TODO 清单

#### P0 - 框架搭建

- [x] **E1. 节日数据 CSV 外部化加载** — `FestivalSystem` 已接入 `assets/data/festival/festival_definitions.csv` 读取（保留内置数据兜底）
  ```
  id,name,season,day,description,special_item,reward_type,reward_value
  spring_festival,春节,winter,1,新年第一天,RedEnvelope,gold,200
  lantern_festival,元宵节,winter,15,正月十五赏灯,StickyRiceBall,stamina,30
  flower_festival,花朝节,spring,3,百花生日,FlowerBasket,social,10
  ...
  ```
  12 个节日：春节、元宵节、花朝节、清明、端午、七夕、中秋、重阳、冬至、大潮祭、茶文化节、丰收祭

- [x] **E2. 节日触发系统（事件化）** — 已在日切触发 `FestivalActiveEvent`，统一节日激活与结算入口
  - 文件：`src/engine/GameClock.cpp`
  - 节日当天：触发 `FestivalActiveEvent`

#### P1 - 单个节日实现（每个 1-2 天）

- [x] **E3. 春节实现（首版）** — 已接入“山庄苏醒祭”日切自动参与与开春红包金币奖励（可验证），其余演出内容后续补齐
- [x] **E4. 元宵节实现（可玩首版）** — 日切礼包 + 节日摊位灯谜/汤圆式体力补给；灯笼装饰
- [x] **E5. 花朝节实现（可玩首版）** — 花意云海染色叠加、种子/好感礼包、花瓣装饰
- [x] **E6. 清明节实现（可玩首版）** — 踏青灵草双倍采集、摊位祭祖小仪、青霭装饰
- [x] **E7. 端午节实现（可玩首版）** — 摊位包粽（灵草+茶包合成式消耗）、香囊式补给、龙舟波纹装饰
- [x] **E8. 七夕节实现（可玩首版）** — 摊位许愿随机金币、星河装饰（CSV 日期已与大潮祭错开）
- [x] **E9. 中秋节实现（可玩首版）** — 赏月满月装饰、月饼式茶包补给、换日多日体力滋养 Buff
- [x] **E10. 重阳节实现（可玩首版）** — 菊花酒式换日体力 Buff、登高摊位互动、山形装饰
- [x] **E11. 冬至节实现（可玩首版）** — 暖食补给、极夜全屏染色、冬云装饰
- [x] **E12. 大潮祭实现（首版可玩）** — `FestivalActiveEvent` 已接入潮灵 Boss 触发、战斗奖励加成与传说奖励发放链路
- [x] **E13. 茶文化节实现（可玩首版）** — 摊位斗茶评分领奖、茶雾装饰；制茶配方扩展留待后续
- [x] **E14. 丰收祭实现（可玩首版）** — 收购商出售 +15% 金币加成、摊位收成评选、谷穗装饰

#### P2 - 节日基础建设

- [x] **E15. 节日 BGM 切换** — 当日有节日时自动切换 `festival_theme.ogg`（无节日时回落季节/大潮 BGM）
- [x] **E16. 节日装饰物（最小可玩）** — 已扩展多套节日主题装饰与花朝/冬至等全屏氛围叠加（见 `WorldRenderer.cpp`）
- [x] **E17. 节日商店（可玩增强版）** — 节日当天可在商店购买限定货品，含每日售罄与购买事件上报

---

## 方向六：灵界农场深化

> **当前状态**：`SpiritRealmGameplay` 和 `SpiritWorldMap` 完整，灵界玩法已完善
> **工作量**：已完成 ✅（仅灵界深层未实现）
> **影响**："种田"已有策略深度玩法

### TODO 清单

#### P0 - 基础入口

- [x] **F1. A-20 灵界入口** — 在主地图设计传送门对象（`type = "spirit_gateway"`）
  - 文件：`prototype_farm.tmx`
  - 按 E 键触发场景切换动画

- [x] **F2. A-21 灵界浅层 TMX** — 淡紫/银白色调地图，30×20 格
  - 云雾粒子背景
  - 灵草采集点
  - 灵兽游荡区域

- [x] **F3. A-22 灵物采集系统** — 已在 `HandlePrimaryInteraction` 接入灵植采集、体力消耗、掉落与冷却
  - 消耗体力
  - 产出 `spirit_grass`/`cloud_dew`/`spirit_dust`
  - 采集冷却 1 小时游戏时间

#### P1 - 灵物经济

- [x] **F4. A-23 灵物物品表** — 完善 `assets/data/SpiritItemTable.csv`
- [x] **F5. 灵物加工链** — spirit_dust ×3 → spirit_essence 等配方已接入 WorkshopSystem

- [x] **F6. A-24 大潮灵界加成** — 大潮时灵物掉落率 ×2，稀有点出现

#### P2 - 灵界战斗

- [x] **F7. B-11 灵界中层战斗** — 已接入灵界中层战斗触发与低难度敌组（`spirit_wisp`/`spirit_cloudling`）
  - 按 J 键使用灵镰刀攻击
  - 击败掉落灵尘

- [x] **F8. B-13 灵气镰刀工具** — 已接入灵尘打造与采集增益（持有 `SpiritSickle` 时灵尘掉落提升）

- [x] **F9. 战斗系统接入灵界** — 已支持灵界区域按 J 触发净化战斗并进入战斗分支

#### P3 - 灵界深度

- [x] **F10. B-12 灵气阶段可视化** — 5 阶段视觉变化（荒废→太初）
- [x] **F11. B-14 双向传送** — 主地图 ↔ 灵界，大潮时传送门发光
- [ ] **F12. C-5 灵界深层** — 高难度挑战区域，暗色调，有毒云雾，首领

---

## 方向七：成就/契约系统

> **当前状态**：`AchievementTable.csv` 有 4 条数据，无 UI，无触发
> **工作量**：低-中
> **影响**：给玩家目标感和里程碑满足感

### TODO 清单

- [x] **G1. A-30 任务面板功能完善** — 实现 `QuestManager` 类
  - 从 `QuestTable.csv` 加载任务
  - 支持状态：未接取/进行中/已完成
  - 完成后触发奖励

- [x] **G2. 成就系统接入** — 实现 `AchievementManager` 类
  - 从 `AchievementTable.csv` 加载
  - 监听游戏事件（收获/送礼/建造等）
  - 达成时显示成就弹窗 + 奖励发放

- [x] **G3. 成就 UI** — 成就面板显示已获得/未获得成就
  - 使用 `PixelUiPanel` 像素风格

- [x] **G4. 扩展成就表** — 从 4 条扩展到 20+ 条
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

- [x] **H1. S-15 多槽位存档** — 创建 `SaveSlotManager` 类
  - 支持 3 个存档槽
  - 每个槽存储：存档时间、游玩天数、季节、缩略图路径

- [x] **H2. 存档缩略图** — 存档时截图保存为缩略图
  - 读档选择界面显示缩略图

- [x] **H3. 存档信息显示** — 显示游玩天数、季节、金币、存档时间

- [x] **H4. 自动存档** — 已在 `OnDayChanged()` 接入每日自动存档，并提供成功/失败提示与日志
- [ ] **H5. 云存档** — 未来支持（预留接口）

---

## 方向九：结婚/好感终局

> **当前状态**：`RelationshipSystem` 完整，告白/婚礼/婚后功能已实现
> **工作量**：已完成 ✅
> **影响**：给玩家情感投入的终局回报

### TODO 清单

- [x] **I1. 告白系统** — `BuildRelationshipDialogueMenu_()` 对话注入告白选项
- [x] **I2. 婚礼预约** — 订婚后可预约婚礼日期
- [x] **I3. 婚后每日增益** — `ApplyDailyMarriageBuff()` 体力消耗略降
- [ ] **I4. 婚后内容深化** — NPC 称呼变化、婚后特殊对话（未实现）

---

## 方向十：玩法循环深化

> **当前状态**：基础种田循环存在，深度不足
> **工作量**：中
> **影响**：增加玩家长期留存动力

### TODO 清单

#### 农业深化

- [x] **J1. A-13 作物生长视觉阶段** — 5 个视觉阶段区分（棕色小点→金色边框+光效）✅
  - 文件：`src/engine/WorldRenderer.cpp`

- [x] **J2. A-10 作物品质系统接入** — Normal/Fine/Rare/Spirit/Holy 五档品质 ✅
  - 集成到 CropTable::CalculateFinalQuality

- [x] **J3. A-11 洒水器系统** — 放置后自动为周围地块浇水 ✅
  - 洒水器合成配方完善（铜/银/金/灵金）

- [x] **J4. A-12 肥料系统** — 普通/优质肥料，不同生长速度加成 ✅
  - 5 种肥料配方：普通/优质/灵肥/云华肥/茶魂肥

- [x] **J5. A-14 温室系统** — 突破季节限制，所有作物可生长 ✅
  - PixelBuildingPanel 温室详情面板完善

- [x] **J6. B-10 作物疾病虫害** — 10% 每日感染概率，需要治疗（含灵兽自动清除）✅

- [x] **J7. C-4 四季完整作物表** — 春 8/夏 8/秋 8/冬 6 种 ✅
  - CropTable.csv 已包含 30+ 作物

#### 经济深化

- [x] **J8. A-15 玩家金币系统** — `int gold_ = 500` 初始资金 ✅
- [x] **J9. A-16 商店 NPC 和界面** — ShopSystem 已实现 ✅
- [x] **J10. A-17 收购商系统** — 卖货界面，品质影响收购价（已支持可选出售项与每周限制）✅
- [x] **J11. A-18 物品价格表配置化** — `PriceTable.csv` ✅
- [x] **J12. A-19 邮购系统** — 预购种子，次日送达（含到货通知与领取）✅

#### 建筑深化

- [x] **J13. B-22 主屋升级系统** — 帐篷→小木屋→砖房→大宅 ✅
- [x] **J14. B-23 工坊升级** — 解锁更多加工槽与配方阶梯 ✅
- [x] **J15. B-24 茶园系统** — 与普通农田分离的特殊作物区 ✅
- [x] **J16. C-9 鸡舍/畜棚系统** — 动物产出系统（日切产出/喂养影响/HUD/存档）✅
- [x] **J17. C-10 客栈经营系统** — NPC 来访、远程订单（日结/权重/存档）✅
- [x] **J18. C-11 建筑装饰系统** — 自定义家园外观（经营联动：客栈/畜棚/舒适度）✅

#### 工具系统深化

- [x] **J19. 工具系统基础** — ToolSystem 定义工具类型和效果表 ✅
  - 文件：`include/CloudSeamanor/ToolSystem.hpp`, `src/domain/ToolSystem.cpp`

- [x] **J20. 农业工具合成** — 锄头/水壶/镰刀/剪刀/斧头/镐子配方 ✅
  - 文件：`assets/data/recipes.csv`

- [x] **J21. ToolSystem 与 FarmingSystem 集成** — 锄头/镰刀效率应用到交互 ✅
  - 文件：`src/engine/FarmingSystem.cpp`, `src/engine/PlayerInteractRuntime.cpp`

- [x] **J22. 障碍物开垦集成** — 斧头/镐子效率应用到障碍物清除 ✅

---

## 执行顺序建议

### 剩余任务优先级

```
优先级  任务                              预计时间  影响力
音频    音频文件准备（下载/生成占位）       1天       ⭐⭐⭐⭐⭐
D4      核心 NPC 对话扩至 20 条            3天       ⭐⭐⭐
D5      心事件 h1 系统（4 NPC）             3天       ⭐⭐⭐
D6      心事件 h3 系统（4 NPC）             3天       ⭐⭐
C1      玩家角色精灵                       3天       ⭐⭐⭐⭐
C2      农作物精灵图集                     5天       ⭐⭐⭐
C3      像素 UI 边框组件                   3天       ⭐⭐⭐⭐
D11     剩余 9 位 NPC 接入                5天       ⭐⭐
F12     灵界深层                          5天       ⭐⭐
I4      婚后内容深化                      3天       ⭐⭐
J1      作物生长视觉阶段                   2天       ⭐⭐⭐
J3      洒水器系统                         2天       ⭐⭐
J5      温室系统                           3天       ⭐⭐
J13     主屋升级系统                       3天       ⭐⭐
J16     鸡舍/畜棚系统                      5天       ⭐⭐
```

---

## 快速启动建议

大部分核心系统已完成！下一步建议：

1. **音频** — 放置音频文件到 `assets/audio/` 目录
2. **像素美术** — 绘制 UI 边框和作物精灵
3. **NPC 内容** — 扩展核心 4 位 NPC 对话 + 心事件
4. **农业深化** — 作物视觉阶段、洒水器、温室系统

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
