# 观云镜系统数据表

> 版本：v1.0 | 日期：2026-05-02
> 用途：替代星露谷电视机，提供每日运势、食谱提示、天气预报

---

## 1. 每日运势表 (fortune_tips.csv)

```csv
Id,Season,Weather,TideType,FavorMin,FortuneTextZh,FortuneTextEn
fortune_001,all,any,normal,0,今日宜：种植，新芽将茁壮成长。,Today is good for: Planting. New sprouts will thrive.
fortune_002,all,any,normal,0,今日宜：浇水，茶树需要滋润。,Today is good for: Watering. Tea trees need care.
fortune_003,all,any,normal,0,今日宜：送礼，与人结善缘。,Today is good for: Gifting. Build good relationships.
fortune_004,all,any,normal,0,今日宜：探索，灵界等待你。,Today is good for: Exploration. The spirit realm awaits.
fortune_005,all,any,tide,0,大潮将至！今日所有灵物效果翻倍！,The great tide approaches! All spirit effects are doubled!
fortune_006,spring,any,normal,0,春茶初芽，今日采茶品质上佳。,Spring tea is sprouting. Great quality today.
fortune_007,summer,any,normal,0,夏日炎炎，记得多喝水解暑。,Hot summer day. Remember to stay hydrated.
fortune_008,autumn,any,normal,0,秋高气爽，适合在后山漫步。,Clear autumn sky. Perfect for walking in the hills.
fortune_009,winter,any,normal,0,冬日静养，炉火旁品茶最佳。,Winter rest. Best time for tea by the fire.
fortune_010,all,heavyRain,normal,0,雨润万物，今日适合整理茶园。,Rain nurtures all. Good day to tend the garden.
fortune_011,all,lightMist,normal,0,薄雾缭绕，灵茶将吸收更多灵气。,Misty morning. Spirit tea will absorb more energy.
fortune_012,all,heavyCloud,normal,0,浓云压顶，茶叶将更加醇厚。,Thick clouds overhead. Tea leaves will be richer.
fortune_013,all,tide,normal,0,大潮日！所有种植收益+50%！,Great Tide Day! All farming yields +50%!
fortune_014,all,any,normal,50,有人记挂你，今天去见见朋友吧。,Someone is thinking of you. Go visit a friend today.
fortune_015,all,any,normal,100,云海眷顾，今日诸事顺遂。,The cloud sea favors you. Everything goes well today.
fortune_016,all,any,normal,200,心中有光，今日可尝试制作高品质茶。,Your heart shines. Try making high-quality tea today.
fortune_017,all,any,normal,500,缘分将至，留意身边的邂逅。,Fate approaches. Watch for encounters.
fortune_018,all,any,normal,800,心意相通，某人正在思念你。,Hearts align. Someone is thinking of you.
fortune_019,spring,any,normal,0,春回大地，万物复苏，正是播种时。,Spring returns. All things revive. Time to plant.
fortune_020,summer,any,normal,0,蝉鸣阵阵，作物生长迅猛。,Cicadas sing. Crops grow rapidly.
fortune_021,autumn,any,normal,0,金风送爽，收获的季节来临。,Golden winds blow. Harvest season arrives.
fortune_022,winter,any,normal,0,瑞雪兆丰年，冬眠也是修行。,Snow promises abundance. Rest is also cultivation.
```

---

## 2. 食谱预告表 (recipe_tips.csv)

```csv
Id,Season,Weather,TideType,RecipeId,UnlockedTextZh,UnlockedTextEn
recipe_tip_001,spring,any,normal,,spring,春季宜清淡，推荐：春茶糕。,Spring suits light dishes. Try: Spring Tea Cake.
recipe_tip_002,summer,any,normal,,summer,夏季解暑，推荐：云雾凉茶。,Summer heat relief. Try: Cloud Mist Tea.
recipe_tip_003,autumn,any,normal,,autumn,秋季润燥，推荐：秋梨灵茶。,Autumn dryness relief. Try: Autumn Pear Tea.
recipe_tip_004,winter,any,normal,,winter,冬季暖身，推荐：红枣姜茶。,Winter warmth. Try: Red Date Ginger Tea.
recipe_tip_005,all,heavyRain,normal,,rain,雨天适合炖汤，推荐：灵草鱼汤。,Rainy days good for soup. Try: Spirit Herb Fish Soup.
recipe_tip_006,all,tide,normal,,tide,大潮日灵感涌现，推荐：潮汐灵茶。,Great Tide creativity. Try: Tide Spirit Tea.
recipe_tip_007,all,any,normal,1,今日有新食谱解锁！查看工坊看看。,New recipe unlocked today! Check the workshop.
recipe_tip_008,all,any,normal,5,灵茶知识精进，可尝试更高阶配方。,Spirit tea knowledge grows. Try advanced recipes.
```

---

## 3. 天气预报显示内容 (weather_forecast.csv)

```csv
Id,WeatherCode,ForecastTextZh,ForecastTextEn,ActivityHint
forecast_clear,clear,明日晴朗,Clear tomorrow,适合户外劳作
forecast_lightMist,lightMist,明日薄雾,Light mist tomorrow,茶叶品质+
forecast_heavyCloud,heavyCloud,明日浓云,Heavy clouds tomorrow,茶叶产量+
forecast_heavyRain,heavyRain,明日大雨,Heavy rain tomorrow,鱼类活跃
forecast_snow,snow,明日降雪,Snow tomorrow,保暖措施
forecast_tide,tide,明日大潮！,Great tide tomorrow!,所有收益翻倍
forecast_unknown,unknown,天气未知,Weather unknown,随机事件概率+
```

---

## 4. 观云镜NPC关联 (mystic_mirror_npc.csv)

```csv
NpcId,InteractionTextZh,InteractionTextEn,UnlockCondition
xuanqing,玄清正在擦拭观云镜，他似乎在观察天象。,Xuanqing is polishing the mystic mirror, observing the sky.,day>=5
lin,林伯说这镜子能映照人心。,Lin says the mirror reflects the heart.,heart_level>=2
wanxing,晚星说她能看到镜中的星辰。,Wanxing says she can see stars in the mirror.,heart_level>=4
```

---

## 5. 观云镜历史记录 (mirror_history.csv)

```csv
Id,Day,Season,Year,FortuneTextZh,WeatherForecastZh,RecipeHintZh
history_001,1,spring,1,今日宜：种植,明日晴朗,春季宜清淡
history_002,2,spring,1,大潮将至！,明日大潮！,大潮日灵感涌现
history_003,3,spring,1,雨润万物,明日大雨,雨天适合炖汤
```

---

## 6. 实现检查清单

### 数据文件创建
- [x] `fortune_tips.csv` - 22条每日运势
- [x] `recipe_tips.csv` - 8条食谱提示
- [x] `weather_forecast.csv` - 7种天气预报
- [x] `mystic_mirror_npc.csv` - NPC关联
- [x] `mirror_history.csv` - 历史记录

### 后续实现
- [ ] `MysticMirrorSystem.hpp/cpp` - 观云镜核心系统
- [ ] `PixelMysticMirrorPanel.hpp/cpp` - 观云镜UI
- [ ] 主屋交互点配置
- [ ] 每日自动更新逻辑

---

## 7. UI布局设计

```
┌─────────────────────────────────────────────────────────────┐
│                     ☁️ 观 云 镜 ☁️                          │
│                                                             │
│  ═══════════════════════════════════════════════════════   │
│                                                             │
│  【今日运势】                                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ "今日宜：种植，新芽将茁壮成长。"                      │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  【天气预报】                                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  明日：晴朗    后日：薄雾    大潮日：第7天          │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  【今日食谱】                                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  推荐：春茶糕 - 春季清淡佳品                         │   │
│  │  材料：春茶 ×2 + 灵露 ×1                          │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  【云海观测】                                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  云海浓度：65%    灵气充沛                          │   │
│  │  ████████████████░░░░░░░░                        │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│                         [ 关闭 ]                            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

**最后更新**: 2026-05-02
**状态**: 数据设计完成，等待代码实现
