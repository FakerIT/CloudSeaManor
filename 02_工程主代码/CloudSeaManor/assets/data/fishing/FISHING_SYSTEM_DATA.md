# 钓鱼系统数据表

> 版本：v1.0 | 日期：2026-05-02
> 用途：定义所有可钓鱼类及其属性

---

## 1. 鱼类数据表 (fish_table.csv)

```csv
Id,NameZh,NameEn,BasePrice,MinWeight,MaxWeight,Difficulty,Season,Weather,TimeRange,LocationId,Rarity,Tier,Description
cloudCarp,云鲤,Cloud Carp,30,0.5,2.0,1,spring|summer|autumn|winter,any,0600-2200,main_farm,common,1,你最常钓到的鱼，浑身透着云雾般的色泽。
mistEel,雾鳝,Mist Eel,50,0.8,3.0,2,spring|summer,lightMist,0600-1800,main_farm|spirit_realm,common,1,喜欢在薄雾中出没，身体如流水般柔软。
rainDewFish,雨露鱼,Raindew Fish,45,0.3,1.5,2,summer|autumn,heavyRain,1200-2400,main_farm|river,uncommon,1,雨后更容易钓到，肉质鲜嫩。
tideKoi,潮汐锦鲤,Tide Koi,200,1.5,5.0,4,any,tide,0000-2400,spirit_realm,tide,2,大潮时才会出现的珍稀锦鲤，据说能带来好运。
starfish,星河鱼,Starlight Fish,180,1.0,4.0,4,autumn|winter,clear,1800-0600,river,rare,2,在星空下闪烁着微光，极为罕见。
moonPetunia,月华鱼,Moon Petunia,150,0.5,2.5,3,any,tide,2200-0400,river|spirit_realm,uncommon,2,只在夜晚和大潮时出没，味道鲜美。
springCharm,春灵鲤,Spring Charm,80,1.0,3.5,2,spring,any,0600-2000,main_farm,common,1,春季特有的鱼，寓意吉祥。
summerFlame,夏焰鱼,Summer Flame,90,0.8,3.0,3,summer,clear,1000-1800,river,uncommon,2,夏季阳光下如火焰般闪耀。
autumnLeaf,秋叶鱼,Autumn Leaf,70,1.2,4.0,2,autumn,any,0800-2000,main_farm|river,common,1,秋季飘落的红叶鱼，味道清甜。
winterCrystal,冬晶鱼,Winter Crystal,100,0.5,2.0,3,winter,snow|clear,0600-1800,river|spirit_realm,uncommon,2,冬季特有的鱼，肉质如水晶般透明。
spiritWisp,灵虚鱼,Spirit Wisp,300,0.3,1.0,5,any,any,0000-2400,spirit_realm,rare,3,灵界的游魂所化，触碰会让人心神荡漾。
cloudDragon,云蛟龙,Cloud Dragon,500,3.0,8.0,5,any,tide,1200-2400,spirit_realm,legendary,4,传说中掌控云海的龙族后裔，极为罕见。
primordialKun,太初鲲,Primordial Kun,1000,10.0,30.0,5,any,tide,0600-1800,spirit_realm_deep,legendary,5,太初之鲲，化而为鸟，其名为鹏。
```

---

## 2. 钓鱼地点表 (fishing_locations.csv)

```csv
Id,NameZh,Description,RequiredItem,MinDay,AvailableSeasons,DifficultyModifier
main_farm,云海山庄农场,你的主农场池塘,None,1,all,0
river,观云河,贯穿山庄的河流,None,1,all,1
spirit_realm,灵界浅层,灵气充沛的水域,FishingRod,10,spring|summer|autumn|winter,2
spirit_realm_deep,灵界深层,危险而神秘的水域,SpiritFishingRod,30,all,4
```

---

## 3. 钓鱼装备表 (fishing_gear.csv)

```csv
Id,NameZh,NameEn,Price,EffectType,EffectValue,Description,UnlockCondition
basic_rod,普通鱼竿,Basic Rod,0,castDistance,1,可抛向近距离的鱼,default
quality_rod,优质鱼竿,Quality Rod,500,castDistance,2,可抛向中距离的鱼,fishing_level>=1
iridium_rod,铱金鱼竿,Iridium Rod,1800,castDistance,3|autoCatchBonus,10,可抛向远距离的鱼，有自动收竿几率,fishing_level>=3
bamboo_float,竹制浮漂,Bamboo Float,100,fishingPower,5,基础的浮漂，提升钓鱼成功率,default
cloud_float,云雾浮漂,Cloud Float,300,fishingPower,10,云雾制成的浮漂，更易感知鱼讯,shop
spirit_float,灵界浮漂,Spirit Float,800,fishingPower,20,能感知灵界鱼类的特殊浮漂,spirit_realm_enter
```

---

## 4. 鱼饵表 (bait.csv)

```csv
Id,NameZh,NameEn,Price,Effect,StackSize,UnlockCondition
common_bait,普通鱼饵,Common Bait,2,chanceIncrease,5,default
quality_bait,优质鱼饵,Quality Bait,5,chanceIncrease,10,quality_rod
spirit_bait,灵气鱼饵,Spirit Bait,20,spiritChance,20,spring_grass_x3
cloud_bait,云雾鱼饵,Cloud Bait,15,rareChance,30,spring_cloud_x5
tide_bait,潮汐鱼饵,Tide Bait,50,tideFishChance,100,tide_essence_x1
```

---

## 5. 钓鱼技艺技能表 (fishing_skills.csv)

```csv
Level,SkillName,FishingPowerBonus,NewUnlocks
1,初学渔者,+5,解锁普通鱼饵
2,云海钓手,+10,解锁云雾浮漂图纸
3,灵界渔夫,+15,解锁铱金鱼竿
4,潮汐大师,+20,解锁潮汐鱼饵
5,太初钓者,+25,解锁所有传说鱼类
```

---

## 6. 加工配方 (fish_recipes.csv)

```csv
Id,NameZh,Ingredients,ResultId,ResultCount,Description,UnlockCondition
cook_cloud_carp,清蒸云鲤,cloudCarpx1,cookedCloudCarpx1,烤云鲤,+30体力,default
cook_moon_petunia,月华鱼汤,moonPetuniax1,moonPetuniaSoups1,月华鱼汤,+50体力+幸运buff,rare_fish_cook
cook_tide_koi,锦鲤炖汤,tideKoix1,koisSoupx1,锦鲤炖汤,+80体力+魅力buff,fishing_level>=3
cook_primordial,太初鲲膳,primordialKunx1,primordialDishx1,太初鲲膳,+200体力+全属性buff,fishing_level>=5
```

---

## 7. 赠礼偏好表 (fish_gift.csv)

```csv
NpcId,FishId,Loved,Gifted
lin,cloudCarp,false,true
lin,tideKoi,true,false
wanxing,mistEel,false,true
wanxing,moonPetunia,true,false
xiaoman,rainDewFish,false,true
xiaoman,starfish,true,false
acha,springCharm,false,true
acha,tideKoi,true,false
xuanqing,spiritWisp,true,false
xuanqing,primordialKun,true,false
song,summerFlame,false,true
song,autumnLeaf,false,true
```

---

## 8. 成就表 (fishing_achievements.csv)

```csv
Id,NameZh,Description,Condition,Reward
fishing_001,初次下竿,第一次成功钓到鱼,fish_count>=1,"{gold:50}"
fishing_010,云海钓手,累计钓到10条鱼,fish_count>=10,"{gold:200}"
fishing_050,资深渔夫,累计钓到50条鱼,fish_count>=50,"{gold:500}"
fishing_100,钓鱼达人,累计钓到100条鱼,fish_count>=100,"{gold:1000,item:cloud_float}"
fishing_tide,潮汐猎人,首次钓到潮汐锦鲤,fish_tideKoi>=1,"{gold:300}"
fishing_legend,传说钓者,首次钓到太初鲲,fish_primordialKun>=1,"{gold:5000,title:太初钓者}"
fishing_master,钓鱼宗师,达到钓鱼等级5,skill_fishing_level>=5,"{gold:2000,title:太初钓者}"
```

---

## 9. 实现检查清单

### 数据文件创建
- [x] `fish_table.csv` - 14种鱼类定义
- [x] `fishing_locations.csv` - 4个钓鱼地点
- [x] `fishing_gear.csv` - 5种鱼竿/浮漂
- [x] `bait.csv` - 5种鱼饵
- [x] `fishing_skills.csv` - 5级技能
- [x] `fish_recipes.csv` - 4种烹饪配方
- [x] `fish_gift.csv` - NPC赠礼偏好
- [x] `fishing_achievements.csv` - 7个钓鱼成就

### 后续实现
- [ ] `FishingSystem.hpp/cpp` - 钓鱼核心系统
- [ ] `FishingMiniGame.hpp/cpp` - 钓鱼小游戏
- [ ] `PixelFishingUI.hpp/cpp` - 钓鱼UI
- [ ] 鱼竿工具类型接入
- [ ] 钓鱼小游戏UI渲染
- [ ] 鱼类物品sprite

---

**最后更新**: 2026-05-02
**状态**: 数据设计完成，等待代码实现
