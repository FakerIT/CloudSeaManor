# 数据表 Schema 说明

## 总则
- 字段名统一使用 `PascalCase`
- 关联字段统一使用全局唯一字符串 ID
- 复杂列表字段允许用 JSON 字符串保存
- 平铺表优先 CSV，嵌套结构可后续切换 JSON

## `npc_data.csv`
| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `Id` | string | NPC 唯一 ID | `lin` |
| `DisplayName` | string | 显示名 | `林茶师` |
| `StageId` | string | 当前主阶段 profile | `tea_master` |
| `BirthdaySeason` | string | 生日季节 | `Spring` |
| `BirthdayDay` | int | 生日日期 | `12` |
| `HomeLocation` | string | 住所锚点 | `TeaField` |
| `WorkLocation` | string | 工作锚点 | `Workshop` |
| `FavoriteTeaIds` | json array | 偏好茶 ID 列表 | `["tea_green_bud"]` |
| `DialogueProfileId` | string | 日常对话 profile | `npc_lin_daily` |
| `ScheduleProfileId` | string | 日程 profile | `schedule_lin` |
| `GiftProfileId` | string | 礼物偏好 profile | `gift_lin` |
| `LifeStageProfileId` | string | 人生阶段 profile | `life_lin` |
| `PositionX` | float | 初始坐标 X | `420` |
| `PositionY` | float | 初始坐标 Y | `320` |

## `pet_data.csv`
| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `Id` | string | 灵宠 ID | `pet_cat_cloud` |
| `DisplayName` | string | 名称 | `云绒猫` |
| `Element` | string | 元素 | `Cloud` |
| `Rarity` | string | 稀有度 | `Rare` |
| `BaseHealth` | int | 基础生命 | `52` |
| `BaseAttack` | int | 基础攻击 | `16` |
| `BaseDefense` | int | 基础防御 | `10` |
| `GrowthCurveId` | string | 成长曲线 ID | `growth_pet_agile` |
| `SkillIds` | json array | 技能列表 | `["pet_mist_burst","pet_find_seed"]` |
| `BehaviorPresetId` | string | 行为预设 | `pet_patrol_swift` |
| `FavorItemIds` | json array | 偏好物品 | `["Feed"]` |
| `UnlockCondition` | string | 解锁条件 | `festival_summer_beast_sports` |

## `tea_data.csv`
| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `Id` | string | 茶品 ID | `tea_cloud_mist` |
| `DisplayName` | string | 名称 | `云岚乌龙` |
| `Element` | string | 元素 | `Wind` |
| `Rarity` | string | 稀有度 | `Rare` |
| `GrowthDays` | int | 生长天数 | `6` |
| `BaseYield` | int | 基础产量 | `3` |
| `BasePrice` | int | 基础售价 | `150` |
| `FertilizerMultiplier` | float | 肥料系数 | `1.15` |
| `CloudMin` | float | 最低云海需求 | `0.25` |
| `HarvestTime` | string | 收获时段 | `noon` |
| `TeaType` | string | 茶类 | `Oolong` |
| `BuffEffectId` | string | 饮用效果 | `buff_stamina_small` |
| `RecipeIds` | json array | 配方列表 | `["recipe_cloud_mist"]` |
| `GiftTags` | json array | 赠礼标签 | `["gift_like","festival"]` |
| `TeaSpiritId` | string | 对应茶灵 | `spirit_mist_bird` |
| `UnlockCondition` | string | 解锁条件 | `chapter_ch2` |

## `weapon_data.csv`
| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `Id` | string | 武器 ID | `weapon_cloud_blade` |
| `DisplayName` | string | 名称 | `云纹长刃` |
| `WeaponType` | string | 类型 | `Melee` |
| `Element` | string | 元素 | `Cloud` |
| `BaseAttack` | int | 攻击力 | `30` |
| `PurifyRateBonus` | float | 净化倍率加成 | `0.10` |
| `CritChanceBonus` | float | 暴击率加成 | `0.03` |
| `CritMultiplierBonus` | float | 暴击伤害加成 | `0.20` |
| `EnergyRecoverBonus` | float | 能量回复加成 | `0.00` |
| `SkillCooldownScale` | float | 技能冷却倍率 | `0.95` |
| `Quality` | int | 品质级别 | `2` |
| `Effects` | json array | 特效标签 | `["purify_up"]` |
| `AcquireMethod` | string | 获取方式 | `default` |

## `skill_data.csv`
| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `Id` | string | 技能 ID | `weapon_tide_slash` |
| `DisplayName` | string | 名称 | `潮刃斩` |
| `Category` | string | 分类 | `Weapon` |
| `Element` | string | 元素 | `Cloud` |
| `Power` | int | 威力 | `34` |
| `Cost` | int | 消耗 | `16` |
| `Cooldown` | float | 冷却 | `6` |
| `StatusEffects` | json array | 附加状态 | `["purify_up"]` |
| `ComboTags` | json array | 连携标签 | `["melee","tide"]` |
| `UnlockCondition` | string | 解锁条件 | `weapon_cloud_blade` |
| `Description` | string | 描述 | `云系近战核心技能` |

## `tool_data.csv`
| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `Id` | string | 工具阶级 ID | `hoe_spirit` |
| `ToolType` | string | 工具类型 | `Hoe` |
| `Tier` | string | 品级阶段 | `Tier5` |
| `DisplayName` | string | 显示名 | `灵金锄` |
| `Description` | string | 描述 | `灵金锄头！翻土范围最大、最省力` |
| `EfficiencyMultiplier` | float | 效率倍率 | `2.2` |
| `RangeMultiplier` | float | 范围倍率 | `1.3` |
| `CostReduction` | float | 体力减免 | `0.35` |
| `SprinklerCoverage` | int | 自动浇水覆盖 | `6` |
| `CollectEfficiency` | float | 收割/采集加成 | `1.5` |
| `CollectRange` | float | 收集范围 | `1.3` |

## `festival_rewards.csv`
| 字段 | 类型 | 说明 | 示例 |
|---|---|---|---|
| `FestivalId` | string | 节日 ID | `cloud_tide_ritual` |
| `RewardType` | string | 奖励类型 | `gold` |
| `RewardValue` | string | 奖励值 | `1000` |
| `GrantItems` | json array | 赠送物品 | `["TideLantern:1"]` |
| `HintLines` | json array | 提示文本 | `["【界桥祭】前往灵界触发三波净化战。"]` |
| `NoticeDays` | int | 预告天数 | `7` |
| `Type` | string | 节日类型 | `cloud_tide` |
