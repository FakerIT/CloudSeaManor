# 像素UI精灵资源规划

> 版本：1.0 | 日期：2026-05-01 | 状态：待美术制作
>
> 本文档定义云海山庄所有需要的像素UI精灵资源。

---

## 目录结构

```
assets/sprites/
├── ui/
│   ├── ui_main.atlas.json      # UI主元素
│   ├── ui_borders.atlas.json   # UI边框
│   ├── ui_icons.atlas.json     # 图标集
│   ├── ui_buttons.atlas.json   # 按钮样式
│   └── ui_progress.atlas.json   # 进度条样式
├── characters/
│   └── player_main.atlas.json  # 玩家角色
├── npc/                         # NPC精灵（13位）
├── beasts/                      # 灵兽精灵（20种）
├── items/                       # 物品精灵
│   ├── items_crop.atlas.json   # 农作物
│   └── items_food.atlas.json   # 食物
├── enemies/                     # 敌人精灵（29种+2 BOSS）
└── tiles/                       # 瓦片集
```

---

## P0 - 立即需要

### 1. 玩家角色精灵 (player_main.atlas.json)

**当前状态**: Atlas已定义，图片缺失

| 精灵ID | 用途 | 尺寸 | 帧数 |
|--------|------|------|------|
| `player_idle_down` | 待机下 | 32x48 | 4 |
| `player_idle_up` | 待机上 | 32x48 | 4 |
| `player_idle_left` | 待机左 | 32x48 | 4 |
| `player_idle_right` | 待机右 | 32x48 | 4 |
| `player_walk_down` | 行走下 | 32x48 | 6 |
| `player_walk_up` | 行走上 | 32x48 | 6 |
| `player_walk_left` | 行走左 | 32x48 | 6 |
| `player_walk_right` | 行走右 | 32x48 | 6 |
| `player_tool_hoe` | 锄头动画 | 32x48 | 4 |
| `player_tool_water` | 浇水动画 | 32x48 | 4 |
| `player_tool_axe` | 斧头动画 | 32x48 | 4 |
| `player_tool_pickaxe` | 镐子动画 | 32x48 | 4 |
| `player_tool_sickle` | 镰刀动画 | 32x48 | 4 |

---

### 2. UI边框组件 (ui_borders.atlas.json)

**当前状态**: Atlas已定义，使用Kenney Tiny Town瓦片

**说明**: 当前引用第三方瓦片图 `tilemap_packed.png`，建议后续替换为自定义云海风格边框。

**推荐风格**:
- 云海山庄主题：淡蓝/淡紫/金色边框
- 圆润像素风格，避免尖角
- 带有云雾纹理装饰

**建议新增边框类型**:
| 精灵ID | 尺寸 | 描述 |
|--------|------|------|
| `border_cloud_tl` | 16x16 | 云纹左上角 |
| `border_cloud_tr` | 16x16 | 云纹右上角 |
| `border_cloud_bl` | 16x16 | 云纹左下角 |
| `border_cloud_br` | 16x16 | 云纹右下角 |
| `border_cloud_h` | 16x16 | 云纹横边 |
| `border_cloud_v` | 16x16 | 云纹竖边 |
| `border_gold_tl` | 16x16 | 金色左上角 |
| `border_gold_tr` | 16x16 | 金色右上角 |
| `border_gold_bl` | 16x16 | 金色左下角 |
| `border_gold_br` | 16x16 | 金色右下角 |
| `border_gold_h` | 16x16 | 金色横边 |
| `border_gold_v` | 16x16 | 金色竖边 |

---

### 3. UI图标集 (ui_icons.atlas.json)

**当前状态**: Atlas已定义

| 精灵ID | 尺寸 | 描述 |
|--------|------|------|
| `icon_stamina` | 16x16 | 体力图标 |
| `icon_coin` | 16x16 | 金币图标 |
| `icon_spirit` | 16x16 | 灵气图标 |
| `icon_cloud_clear` | 16x16 | 晴朗云海 |
| `icon_cloud_mist` | 16x16 | 薄雾 |
| `icon_cloud_dense` | 16x16 | 浓云海 |
| `icon_cloud_tide` | 16x16 | 大潮 |
| `icon_quest` | 16x16 | 任务图标 |
| `icon_heart_10` | 16x16 | 10心好感 |
| `icon_heart_8` | 16x16 | 8心好感 |
| `icon_heart_6` | 16x16 | 6心好感 |
| `icon_heart_4` | 16x16 | 4心好感 |
| `icon_heart_2` | 16x16 | 2心好感 |
| `icon_heart_0` | 16x16 | 0心好感 |
| `icon_season_spring` | 16x16 | 春 |
| `icon_season_summer` | 16x16 | 夏 |
| `icon_season_autumn` | 16x16 | 秋 |
| `icon_season_winter` | 16x16 | 冬 |
| `icon_time_morning` | 16x16 | 清晨 |
| `icon_time_noon` | 16x16 | 中午 |
| `icon_time_afternoon` | 16x16 | 下午 |
| `icon_time_evening` | 16x16 | 傍晚 |
| `icon_time_night` | 16x16 | 夜晚 |
| `icon_weather_sunny` | 16x16 | 晴天 |
| `icon_weather_rainy` | 16x16 | 雨天 |
| `icon_weather_snowy` | 16x16 | 雪天 |
| `icon_quality_normal` | 16x16 | 普通品质 |
| `icon_quality_fine` | 16x16 | 优质 |
| `icon_quality_rare` | 16x16 | 稀有 |
| `icon_quality_spirit` | 16x16 | 灵品 |
| `icon_quality_holy` | 16x16 | 圣品 |

---

### 4. UI按钮样式 (ui_buttons.atlas.json)

**当前状态**: 需新建

| 精灵ID | 尺寸 | 描述 |
|--------|------|------|
| `btn_normal_idle` | 64x32 | 常规按钮-默认 |
| `btn_normal_hover` | 64x32 | 常规按钮-悬停 |
| `btn_normal_pressed` | 64x32 | 常规按钮-按下 |
| `btn_normal_disabled` | 64x32 | 常规按钮-禁用 |
| `btn_small_idle` | 48x24 | 小按钮-默认 |
| `btn_small_hover` | 48x24 | 小按钮-悬停 |
| `btn_small_pressed` | 48x24 | 小按钮-按下 |
| `btn_gold_idle` | 64x32 | 金色按钮-默认 |
| `btn_gold_hover` | 64x32 | 金色按钮-悬停 |
| `btn_gold_pressed` | 64x32 | 金色按钮-按下 |
| `btn_icon_idle` | 32x32 | 图标按钮-默认 |
| `btn_icon_hover` | 32x32 | 图标按钮-悬停 |
| `btn_icon_pressed` | 32x32 | 图标按钮-按下 |

---

### 5. 进度条样式 (ui_progress.atlas.json)

**当前状态**: 需新建

| 精灵ID | 尺寸 | 描述 |
|--------|------|------|
| `bar_bg` | 128x16 | 进度条背景 |
| `bar_fill_green` | 8x12 | 绿色填充 |
| `bar_fill_blue` | 8x12 | 蓝色填充 |
| `bar_fill_red` | 8x12 | 红色填充 |
| `bar_fill_gold` | 8x12 | 金色填充 |
| `bar_fill_cloud` | 8x12 | 云海填充 |
| `bar_border` | 128x16 | 进度条边框 |
| `heart_bar_bg` | 120x16 | 好感条背景 |
| `heart_bar_fill` | 120x16 | 好感条填充 |
| `exp_bar_bg` | 128x12 | 经验条背景 |
| `exp_bar_fill` | 128x12 | 经验条填充 |

---

### 6. 物品图标 (items_crop.atlas.json)

**当前状态**: Atlas已定义

**农作物清单** (30种，每种5生长阶段):

| 类别 | 作物 | 尺寸 |
|------|------|------|
| 春 | 萝卜、白菜、草莓、土豆、小麦、莴苣、蓝莓、葱 | 16x16 |
| 夏 | 番茄、辣椒、玉米、南瓜、茄子、甜瓜、西瓜、菠萝 | 16x16 |
| 秋 | 苹果、葡萄、石榴、南瓜、萝卜、胡萝卜、土豆、红薯 | 16x16 |
| 冬 | 白菜、土豆、芜菁、雪莲、人参、姜、山药、蒜 | 16x16 |
| 灵茶 | 初雾绿茶、晨露白茶、云莓红茶、暮霞乌龙、松烟黑茶等 | 16x16 |

**每种作物需要**:
- `crop_xxx_seed` - 种子
- `crop_xxx_stage1` - 生长阶段1
- `crop_xxx_stage2` - 生长阶段2
- `crop_xxx_stage3` - 生长阶段3
- `crop_xxx_stage4` - 生长阶段4
- `crop_xxx_harvest` - 可收获状态
- `crop_xxx_harvest_golden` - 金边收获（高品质）

---

### 7. 工具图标 (ui_tools.atlas.json)

**当前状态**: 需新建

| 工具ID | 描述 | 尺寸 |
|--------|------|------|
| `tool_hoe` | 锄头 | 16x16 |
| `tool_watering_can` | 喷壶 | 16x16 |
| `tool_axe` | 斧头 | 16x16 |
| `tool_pickaxe` | 镐子 | 16x16 |
| `tool_sickle` | 镰刀 | 16x16 |
| `tool_hammer` | 锤子 | 16x16 |
| `tool_fishing_rod` | 鱼竿 | 16x16 |
| `tool_sword` | 灵剑 | 16x16 |
| `tool_sprinkler` | 洒水器 | 16x16 |

---

## P1 - 高优先级

### NPC精灵 (13位)

| NPC ID | 名称 | 需要精灵 |
|--------|------|----------|
| acha | 阿茶 | idle/walk/interact/emote |
| lin | 老槐 | idle/walk/interact/emote |
| wanxing | 晚星 | idle/walk/interact/emote |
| xiaoman | 小满 | idle/walk/interact/emote |
| song | 松年 | idle/walk/interact/emote |
| yu | 雾川 | idle/walk/interact/emote |
| mo | 墨 | idle/walk/interact/emote |
| qiao | 锦时 | idle/walk/interact/emote |
| he | 秋棠 | idle/walk/interact/emote |
| ning | 宁 | idle/walk/interact/emote |
| an | 阿巳 | idle/walk/interact/emote |
| shu | 素锦 | idle/walk/interact/emote |
| yan | 玄清 | idle/walk/interact/emote |

### 灵兽精灵 (20种)

| 稀有度 | 灵兽 |
|--------|------|
| ★☆☆☆☆ | 云狐、雾鹿、竹灵、观云雀 |
| ★★☆☆☆ | 岩龟、茶灵猫、温泉灵蟹、玉蝶、灵芝精 |
| ★★★☆☆ | 青鸾、霜魂兔、古藤猿、风铃狐、雷雀 |
| ★★★★☆ | 界影狼、星尘鹿、云梦蝶 |
| ★★★★★ | 太初雏龙、玉泉鹿、云火凤 |

每种需要: idle_anim / follow_anim / interact_anim

---

## P2 - 中期目标

### 战斗敌人 (29种 + 2 BOSS)

| 类型 | 敌人 |
|------|------|
| 普通 | 游荡浊气、迷途灵蝶、躁动藤蔓 |
| 精英 | 愤怒石灵、狂乱云豹、怨念精英灵体 |
| BOSS | 深渊水母、暴怒岩魔、界桥裂隙守卫、云海潮灵 |

每种需要: idle_anim / polluted_effect / purification_vanish_anim

---

## 美术规范

### 像素风格
- **基础尺寸**: 16x16 像素
- **角色尺寸**: 32x48 像素（站立），32x64 像素（完整）
- **缩放比例**: 4x（推荐）或 2x
- **DPI**: 72 DPI
- **调色板**: 建议不超过32色的像素风格

### 文件格式
- **导出格式**: PNG (带透明通道)
- **命名规范**: `类型_名称_状态_帧数.png`
- **示例**: `crop_turnip_harvest.png`, `npc_acha_walk_down_01.png`

### 颜色规范
- **主题色**: 淡蓝 #87CEEB, 淡紫 #DDA0DD, 金色 #FFD700
- **UI背景**: 半透明黑色 rgba(0,0,0,0.7)
- **边框高亮**: #FFFFFF
- **边框阴影**: rgba(0,0,0,0.5)

---

## 更新记录

| 日期 | 版本 | 变更 |
|------|------|------|
| 2026-05-01 | 1.0 | 初始版本，定义所有UI精灵需求 |
