# 音频资源需求清单

> 版本：1.0 | 日期：2026-05-01 | 状态：待填充
>
> 本文档列出云海山庄所有需要的音频资源，按优先级分类。

---

## 目录结构

```
assets/audio/
├── bgm/          # 背景音乐
├── sfx/          # 音效
└── ambient/      # 环境音
```

---

## P0 - 立即需要（影响力最大）

### BGM 背景音乐

| 文件名 | 用途 | 时长建议 | 风格 |
|--------|------|----------|------|
| `main_theme.ogg` | 主菜单/庄园 | 3-5分钟循环 | 舒缓治愈 |
| `farm_theme.ogg` | 农场主题 | 2-3分钟循环 | 轻松愉快 |
| `spring_theme.ogg` | 春季 | 2-3分钟循环 | 明快活泼 |
| `summer_theme.ogg` | 夏季 | 2-3分钟循环 | 阳光热情 |
| `autumn_theme.ogg` | 秋季 | 2-3分钟循环 | 温婉惆怅 |
| `winter_theme.ogg` | 冬季 | 2-3分钟循环 | 静谧温暖 |

### SFX 音效

| 文件名 | 触发时机 | 时长建议 |
|--------|----------|----------|
| `harvest.ogg` | 收割作物 | <1秒 |
| `plant.ogg` | 种植作物 | <1秒 |
| `water.ogg` | 浇水 | <1秒 |
| `dialogue_continue.ogg` | 对话继续 | <1秒 |
| `level_up.ogg` | 升级 | 1-2秒 |
| `dialogue_choice.ogg` | 对话选择 | <1秒 |
| `shop_purchase.ogg` | 购买物品 | <1秒 |
| `shop_sell.ogg` | 出售物品 | <1秒 |
| `gift.ogg` | 送礼 | 1-2秒 |
| `heart_event.ogg` | 心事件触发 | 2-3秒 |
| `heart_gain.ogg` | 好感提升 | <1秒 |
| `ui_click.ogg` | 界面点击 | <1秒 |
| `ui_open.ogg` | 界面打开 | <1秒 |
| `ui_close.ogg` | 界面关闭 | <1秒 |
| `notification.ogg` | 通知提示 | 1-2秒 |
| `error.ogg` | 错误提示 | 1秒 |
| `footstep_grass.ogg` | 草地脚步声 | <1秒 |
| `footstep_stone.ogg` | 石板脚步声 | <1秒 |

### 环境音 Ambient

| 文件名 | 场景 | 时长建议 |
|--------|------|----------|
| `farm_ambient.ogg` | 农场环境 | 3-5分钟循环 |
| `rain.ogg` | 下雨 | 3-5分钟循环 |
| `wind_mist.ogg` | 薄雾风声 | 2-3分钟循环 |
| `wind_strong.ogg` | 浓云风声 | 2-3分钟循环 |

---

## P1 - 高优先级

### BGM 背景音乐

| 文件名 | 用途 | 风格 |
|--------|------|------|
| `battle_theme.ogg` | 战斗主题 | 紧张但治愈 |
| `tide_theme.ogg` | 大潮天气 | 史诗空灵 |
| `festival_theme.ogg` | 节日通用 | 欢快热闹 |
| `morning_theme.ogg` | 清晨时段 | 清新宁静 |
| `afternoon_theme.ogg` | 午后时段 | 慵懒舒适 |
| `evening_theme.ogg` | 傍晚时段 | 温暖柔和 |
| `night_theme.ogg` | 夜晚时段 | 静谧安详 |
| `spirit_realm.ogg` | 灵界 | 神秘飘渺 |
| `teahouse.ogg` | 茶室 | 雅致禅意 |
| `shop.ogg` | 商店 | 轻快活泼 |
| `home.ogg` | 室内 | 温馨舒适 |

### SFX 音效

| 文件名 | 触发时机 |
|--------|----------|
| `rain.ogg` | 下雨 |
| `wind_light.ogg` | 轻风 |
| `tide_magic.ogg` | 大潮环境 |
| `battle_purify.ogg` | 净化成功 |
| `battle_hit.ogg` | 技能命中 |
| `battle_buff.ogg` | 增益生效 |
| `beast_happy.ogg` | 灵兽开心 |
| `beast_hungry.ogg` | 灵兽饥饿 |
| `craft_success.ogg` | 制作成功 |
| `craft_fail.ogg` | 制作失败 |
| `unlock.ogg` | 解锁成功 |
| `achievement.ogg` | 成就达成 |
| `mail_arrive.ogg` | 邮件到达 |
| `festival_firework.ogg` | 节日烟花 |
| `spring_bell.ogg` | 春季钟声 |
| `sleigh_bell.ogg` | 冬季铃声 |

### 环境音 Ambient

| 文件名 | 场景 |
|--------|------|
| `spirit_realm_ambient.ogg` | 灵界环境 |
| `forest_ambient.ogg` | 森林环境 |
| `mountain_ambient.ogg` | 山间环境 |
| `teahouse_ambient.ogg` | 茶室环境 |
| `spring_water.ogg` | 泉水声 |

---

## P2 - 中期目标

### 12个节日专属BGM

| 文件名 | 节日 |
|--------|------|
| `festival_spring.ogg` | 春节 |
| `festival_lantern.ogg` | 元宵节 |
| `festival_flower.ogg` | 花朝节 |
| `festival_qingming.ogg` | 清明节 |
| `festival_dragon.ogg` | 端午节 |
| `festival_qixi.ogg` | 七夕节 |
| `festival_midautumn.ogg` | 中秋节 |
| `festival_double9.ogg` | 重阳节 |
| `festival_winter_solstice.ogg` | 冬至节 |
| `festival_tide.ogg` | 大潮祭 |
| `festival_tea.ogg` | 茶文化节 |
| `festival_harvest.ogg` | 丰收祭 |

### SFX 音效

| 文件名 | 触发时机 |
|--------|----------|
| `festival_drum.ogg` | 节日鼓声 |
| `festival_firework_burst.ogg` | 烟花绽放 |
| `boat_row.ogg` | 龙舟划船 |
| `lantern_lantern.ogg` | 元宵灯笼 |
| `dance_music.ogg` | 舞蹈音乐 |
| `firework_launch.ogg` | 烟花发射 |
| `meditation.ogg` | 冥想 |
| `bell_temple.ogg` | 道观钟声 |
| `cloud_form.ogg` | 云雾形成 |
| `wisp_disappear.ogg` | 灵体消散 |
| `mysterious_appear.ogg` | 神秘出现 |
| `rare_drop.ogg` | 稀有掉落 |
| `legendary_item.ogg` | 传说物品 |

---

## 音频规格要求

### 格式
- **推荐格式**: OGG Vorbis (`.ogg`)
- **备选格式**: MP3 (`.mp3`)
- **采样率**: 44100 Hz
- **位深**: 16-bit
- **声道**: 立体声 (BGM), 单声道或立体声 (SFX)

### 文件大小参考
- BGM: 2-5 MB / 首
- SFX: 10-100 KB / 个
- Ambient: 500 KB - 2 MB / 个

---

## 备注

1. **占位文件**: 在正式音频准备好之前，可以创建静音的 `.ogg` 文件作为占位符
2. **音量标准化**: 所有音频应进行音量标准化，避免游戏内声音忽大忽小
3. **循环点**: BGM应设置好循环点，无缝衔接
4. **渐进渐出**: 场景切换时应有淡入淡出效果

---

## 更新记录

| 日期 | 版本 | 变更 |
|------|------|------|
| 2026-05-01 | 1.0 | 初始版本，列出所有音频需求 |
