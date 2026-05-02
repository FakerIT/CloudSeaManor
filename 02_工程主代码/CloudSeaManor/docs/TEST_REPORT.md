# 《云海山庄》QA 缺陷报告 v1.0

> **报告日期**：2026-05-02
> **报告人**：QA 测试工程师
> **测试方法**：静态代码分析 + 设计文档对照
> **版本**：0.1.0 MVP 原型

---

## 一、缺陷汇总

| 缺陷 ID | 严重度 | 模块 | 标题 | 状态 |
|---------|--------|------|------|------|
| TBD-001 | **P1** | 战斗系统 | 元素枚举双轨不一致（旧五行 vs 新云海元素） | 待确认 |
| TBD-002 | **P1** | 战斗系统 | 失衡段胜利条件语义冲突 | 待确认 |
| TBD-003 | **P1** | 存档系统 | 战斗中间态存档策略不明确 | 待确认 |
| TBD-004 | **P1** | 战斗系统 | 茶道连携序列硬编码 | 待确认 |
| TBD-005 | **P2** | 战斗系统 | 反噬数值未从配置驱动 | 待确认 |
| TBD-006 | **P2** | 种植系统 | 季节更替边界条件未验证 | 待确认 |
| TBD-007 | **P2** | 存档系统 | 旧档迁移缺少字段校验 | 待确认 |
| TBD-008 | **P3** | 战斗系统 | 失衡段 UI 渲染逻辑验证 | 待确认 |

---

## 二、详细缺陷分析

### TBD-001 | 元素枚举双轨不一致（数据一致性）

**严重度**：P1  
**模块**：战斗系统 - `BattleField.cpp`  
**发现位置**：`src/engine/BattleField.cpp:33-42`

**问题描述**：

当前代码使用旧五行元素枚举（`Water/Fire/Wood/Metal/Earth`），而设计文档（`战斗数值_武器_任务技能_灵兽技能_统一设计稿.md`）定义了新云海元素体系（`Cloud/Wind/Dew/Glow/Tide`）。

**代码现状**：

```cpp:33:42:02_工程主代码\CloudSeaManor\src\engine\BattleField.cpp
ElementType ParseElement_(const std::string& value) {
    if (value == "water") return ElementType::Water;
    if (value == "fire") return ElementType::Fire;
    if (value == "wood") return ElementType::Wood;
    if (value == "metal") return ElementType::Metal;
    if (value == "earth") return ElementType::Earth;
    if (value == "light") return ElementType::Light;
    if (value == "dark") return ElementType::Dark;
    return ElementType::Neutral;
}
```

**设计文档要求**（v1.1 升华补丁）：

```
五环循环（克制倍率默认 1.50，被克制 0.75）：
风 > 云 > 露 > 霞 > 潮 > 风
光暗互克：光 > 暗、暗 > 光（倍率 1.50）
```

**影响范围**：

1. `IsCounterElement_` 函数使用旧循环顺序
2. `InitializeImbalanceSegments_` 使用旧元素池
3. 技能表 CSV 字段仍使用 legacy element

**建议修复**：

1. 实现双轨映射层（`LegacyElement -> AuraElement`）
2. JSON 真值源逐步迁移到新元素
3. 在 `BattleDataLoader` 中统一处理

**相关设计文档**：
- `docs/design/battle/战斗数值_武器_任务技能_灵兽技能_统一设计稿.md` - 第 1.2 节
- `docs/design/battle/战斗数据接入计划_元素迁移_探索战斗技能.md` - 第 3 节

---

### TBD-002 | 失衡段胜利条件语义冲突

**严重度**：P1  
**模块**：战斗系统 - `BattleField.cpp`  
**发现位置**：`src/engine/BattleField.cpp:1087-1096`

**问题描述**：

代码中存在两种胜利判定逻辑：

1. **传统净化**：`current_pollution <= 0.0f` → `is_defeated = true`
2. **失衡段胜利**：`all_segments_cleared` → `current_pollution = 0.0f`

设计文档要求"全段归零时战斗胜利"，但当前实现允许以下边界情况：

```cpp:1087:1096:02_工程主代码\CloudSeaManor\src\engine\BattleField.cpp
    bool all_segments_cleared = !spirit.imbalance_segments.empty();
    for (const auto& seg : spirit.imbalance_segments) {
        if (seg.is_active) {
            all_segments_cleared = false;
            break;
        }
    }
    if (all_segments_cleared) {
        spirit.current_pollution = 0.0f;
    }
```

**问题**：

- `all_segments_cleared` 检查只在段位变化后执行一次，不是实时判定
- 如果 `pollution > 0` 但段位全清，浊灵不会立即被判定为 `is_defeated`
- 直到下一次技能命中才会触发净化检查

**建议修复**：

1. 在 `CheckVictoryCondition_` 中增加独立段位胜利检查
2. 或确保每次段位变化后立即调用净化检查
3. 统一"调和"语义

---

### TBD-003 | 战斗中间态存档策略不明确

**严重度**：P1  
**模块**：存档系统 - `GameAppSave.cpp`  
**发现位置**：`src/engine/GameAppSave.cpp:658-661`

**问题描述**：

设计文档（v1.1 升华补丁）要求：
> "战斗短局不存中间态，只存结果"

但当前存档实现：

```cpp:658:661:02_工程主代码\CloudSeaManor\src\engine\GameAppSave.cpp
    if (in_battle_mode && battle_state) {
        lines.push_back(
            "battle|" + std::to_string(*in_battle_mode ? 1 : 0) + "|"
            + std::to_string(*battle_state));
    }
```

**问题**：

1. 仅存储 `battle_state` 整数（0-3），不存储实际战斗进度
2. 浊灵污染值、失衡段状态未持久化
3. 如果玩家在战斗中强退，重载后战斗状态丢失

**设计意图推断**：

根据升华补丁原则，战斗应设计为"快进快出"，不需要中间态存档。但如果存在崩溃恢复需求，需要明确定义策略。

**建议修复**：

1. **方案 A**：战斗中途强退视为撤离（不扣奖励）
2. **方案 B**：实现战斗中间态完整存档
3. 在设计文档中明确战斗状态持久化策略

---

### TBD-004 | 茶道连携序列硬编码

**严重度**：P1  
**模块**：战斗系统 - `BattleField.cpp`  
**发现位置**：`src/engine/BattleField.cpp:1222-1235`

**问题描述**：

茶道连携序列在代码中硬编码，未从数据表驱动：

```cpp:1222:1235:02_工程主代码\CloudSeaManor\src\engine\BattleField.cpp
void BattleField::TryTriggerTeaCombo_() {
    static const std::array<const char*, 3> kTeaCombo = {
        "player_stone_shield",
        "player_cloud_domain",
        "player_wind_blade",
    };
    if (tea_combo_history_.size() != kTeaCombo.size()) {
        return;
    }
    for (std::size_t i = 0; i < kTeaCombo.size(); ++i) {
        if (tea_combo_history_[i] != kTeaCombo[i]) {
            return;
        }
    }
    // ...
}
```

**问题**：

1. 连携序列写死在代码中，策划无法通过配置调整
2. MVP 仅支持 1 条固定连携（符合设计）
3. 但序列内容应从 `balance.json` 或 `quest_skills.json` 读取

**设计文档要求**（v1.1）：
> "茶道连携：按固定序列释放技能触发额外净化/回能（MVP 先做 1 条连携）"

**建议修复**：

1. 在 `battle/balance.json` 中添加连携序列配置
2. 加载时注入到 `BattleField`
3. 预留多连携扩展接口

---

### TBD-005 | 反噬数值未从配置驱动

**严重度**：P2  
**模块**：战斗系统 - `BattleField.cpp`  
**发现位置**：`src/engine/BattleField.cpp:1069-1073`

**问题描述**：

同元素反噬的数值在代码中硬编码：

```cpp:1069:1073:02_工程主代码\CloudSeaManor\src\engine\BattleField.cpp
    if (resonance_backfire) {
        const float backlash = std::max(3.0f, damage * 0.15f);
        player_state_.current_energy = std::max(0.0f, player_state_.current_energy - backlash);
        spirit.current_pollution = std::min(spirit.max_pollution, spirit.current_pollution + damage * 0.06f);
        AddLog("⚠ 灵气反噬！同元素共鸣导致你额外损失能量。", false, true);
    }
```

**硬编码数值**：
- `backlash = max(3.0f, damage * 0.15f)` - 能量消耗基础值
- `pollution_restore = damage * 0.06f` - 污染值回复比例

**设计文档要求**（v1.1 升华补丁）：
```json
"purify_battle.backfire_segment_restore_ratio": 0.06,
"purify_battle.backfire_extra_energy_cost": 8
```

**建议修复**：

1. 在 `battle/balance.json` 中定义反噬参数
2. `BattleField` 加载时读取配置
3. 代码中引用配置值而非魔法数

---

### TBD-006 | 季节更替边界条件未验证

**严重度**：P2  
**模块**：种植系统 - `CropGrowthSystem.cpp`  
**发现位置**：`src/engine/systems/CropGrowthSystem.cpp:227-247`

**问题描述**：

季节更替时作物处理逻辑：

```cpp:227:247:02_工程主代码\CloudSeaManor\src\engine\systems\CropGrowthSystem.cpp
void CropGrowthSystem::HandleSeasonChanged(
    GameWorldState& world_state,
    CloudSeamanor::domain::Season current_season
) {
    for (auto& plot : world_state.MutableTeaPlots()) {
        if (!ShouldPlotWiltBecauseSeason(plot, current_season)) {
            continue;
        }
        plot.seeded = false;
        plot.watered = false;
        plot.ready = false;
        plot.growth = 0.0f;
        plot.stage = 0;
        // ...
    }
}
```

**潜在边界问题**：

1. **作物在成熟临界点**：如果 `plot.ready == true` 且刚收获，是否会错误重置？
2. **季节边界日触发**：第 1 天播种植株，季节更替立即发生，作物状态？
3. **温室作物处理**：温室中的作物是否应跳过季节枯萎？

**代码分析**：

`ShouldPlotWiltBecauseSeason` 函数需要验证以下条件：
- 作物是否属于当前季节允许种植
- 温室保护是否生效
- 是否已有收获但未清除

**建议测试用例**：

| 用例 | 操作 | 预期 |
|------|------|------|
| S-001 | 季节最后一天播种，次日季节更替 | 作物状态？ |
| S-002 | 作物成熟当天立即收获 | 收获后状态？ |
| S-003 | 温室中作物过季 | 是否受保护？ |
| S-004 | 多地块不同作物同时过季 | 是否全部正确处理？ |

---

### TBD-007 | 旧档迁移缺少字段校验

**严重度**：P2  
**模块**：存档系统 - `GameAppSave.cpp`  
**发现位置**：`src/engine/GameAppSave.cpp:294-315`

**问题描述**：

`MigrateLegacyLinesToV5` 仅处理邮件格式转换，未校验其他字段：

```cpp:294:315:02_工程主代码\CloudSeaManor\src\engine\GameAppSave.cpp
std::vector<std::string> MigrateLegacyLinesToV5(
    const std::vector<std::string>& legacy_lines,
    int save_version) {
    if (save_version >= 5) {
        return legacy_lines;
    }
    std::vector<std::string> migrated;
    migrated.reserve(legacy_lines.size());
    for (const auto& line : legacy_lines) {
        const auto fields = SplitSaveFields(line);
        if (fields.empty()) {
            continue;
        }
        // v1~v3 历史格式兼容：mail_order|item|count|day -> mail|item|day|count
        if (fields[0] == "mail_order" && fields.size() >= 4) {
            migrated.push_back("mail|" + fields[1] + "|" + fields[3] + "|" + fields[2]);
            continue;
        }
        migrated.push_back(line);
    }
    return migrated;
}
```

**潜在风险**：

1. 旧档缺少新版本必填字段时，可能导致加载后状态异常
2. `plot_schema` 字段缺失时，`GetPlotField` 使用 legacy index 但可能不匹配
3. 迁移后未校验必填字段完整性

**建议修复**：

1. 迁移函数增加字段存在性检查
2. 缺失必填字段时使用默认值
3. 迁移后输出警告日志

---

### TBD-008 | 失衡段 UI 渲染逻辑验证

**严重度**：P3  
**模块**：战斗 UI - `BattleUI.cpp`  
**发现位置**：`src/engine/BattleUI.cpp:365-398`

**问题描述**：

失衡段 UI 渲染逻辑存在多层嵌套条件：

```cpp:365:398:02_工程主代码\CloudSeaManor\src\engine\BattleUI.cpp
        bar.imbalance_segments.clear();
        if (!spirit.imbalance_segments.empty()) {
            // ...
            for (const auto& seg : spirit.imbalance_segments) {
                // 渲染段位
            }
            for (const auto& seg : spirit.imbalance_segments) {
                // 渲染推荐克制元素
                if (IsCounterElement_(counter, seg.element) && seg.is_active) {
                    // 设置推荐标签
                }
            }
            bar.recommend_label.setString(ToSfString("失衡已清零"));
        }
```

**验证需求**：

1. 当所有段位 `is_active == false` 时，`recommend_label` 是否正确显示"失衡已清零"？
2. 克制元素推荐逻辑是否与 `IsCounterElement_` 一致？
3. 切换目标时 UI 是否正确更新？

**建议测试用例**：

| 用例 | 场景 | 预期 UI |
|------|------|---------|
| UI-001 | 失衡段全清 | 显示"失衡已清零" |
| UI-002 | 存在活跃段位 | 显示克制元素推荐 |
| UI-003 | 切换目标 | UI 立即更新 |
| UI-004 | 多目标战斗 | 各目标段位独立显示 |

---

## 三、测试用例矩阵

### 3.1 战斗系统核心测试

| 用例 ID | 测试场景 | 前置条件 | 操作步骤 | 预期结果 | 严重度 |
|---------|----------|----------|----------|----------|--------|
| B-T001 | 新手战斗流程 | 新游戏 Lv1 | 进入浊灵区域，遭遇游荡浊气，使用基础技能 | 8-15 秒净化 | P0 |
| B-T002 | 元素克制验证 | 任意等级 | 使用克制元素攻击浊灵 | 伤害 1.5x，日志显示 | P0 |
| B-T003 | 同元素反噬 | 任意等级 | 连续使用同元素技能 | 显示反噬警告，能量减少 | P1 |
| B-T004 | 茶道连携触发 | 解锁 3 个技能 | 按石盾→云域→风刃顺序释放 | 额外净化 2 段 | P1 |
| B-T005 | 失衡段胜利 | 战斗进行中 | 将所有失衡段打空 | 浊灵立即净化 | P1 |
| B-T006 | 战斗撤离 | 战斗中 | 按撤离键 | 安全返回庄园，无惩罚 | P0 |
| B-T007 | 战斗崩溃恢复 | 战斗中强退 | 强制退出并重载 | 战斗状态正确处理 | P0 |

### 3.2 种植系统核心测试

| 用例 ID | 测试场景 | 前置条件 | 操作步骤 | 预期结果 | 严重度 |
|---------|----------|----------|----------|----------|--------|
| F-T001 | 完整种植流程 | 新游戏 | 整地→播种→浇水→等待→收获 | 收获成功，物品入背包 | P0 |
| F-T002 | 季节影响 | 任意季节 | 在季节末播种作物 | 作物枯萎处理正确 | P1 |
| F-T003 | 灵化变种触发 | 浓云海/大潮 | 作物成熟时为浓云海 | 15%/30% 概率灵化 | P1 |
| F-T004 | 温室保护 | 有温室 | 温室中作物过季 | 不枯萎 | P2 |

### 3.3 存档系统核心测试

| 用例 ID | 测试场景 | 前置条件 | 操作步骤 | 预期结果 | 严重度 |
|---------|----------|----------|----------|----------|--------|
| S-T001 | 新游戏保存/读取 | 有庄园状态 | 保存→退出→读取 | 状态完全恢复 | P0 |
| S-T002 | 旧档迁移 | v1-v4 存档 | 读取旧档 | 正确迁移，无崩溃 | P0 |
| S-T003 | 存档损坏恢复 | 存档损坏 | 读取损坏存档 | 回退到备份 | P1 |
| S-T004 | 战斗状态存档 | 战斗后 | 保存→读取 | 战斗奖励正确恢复 | P1 |

---

## 四、玩法体验评估

### 4.1 治愈感评估

| 维度 | 设计要求 | 代码实现 | 评估 |
|------|----------|----------|------|
| 撤离无惩罚 | 设计要求 | `Retreat()` 无惩罚逻辑 | ✅ 符合 |
| 失败仅时间成本 | 设计要求 | 无永久失败机制 | ✅ 符合 |
| 战斗可随时撤离 | 设计要求 | `BattleState::Retreat` 支持 | ✅ 符合 |

### 4.2 节奏护栏评估

| 维度 | 设计目标 | 代码验证 | 评估 |
|------|----------|----------|------|
| 普通战斗 15-30s | 设计要求 | 需实际测试验证 | ⏳ 待动态测试 |
| 能量收支平衡 | 设计要求 | `energy_cost = damage * 0.5` | ✅ 公式存在 |
| 操作点 ≤ 4 | 设计要求 | Q/W/E/R + 宠物 + 撤离 | ✅ 符合 |

### 4.3 策略深度评估

| 维度 | 设计要求 | 代码实现 | 评估 |
|------|----------|----------|------|
| 5 环克制简化 | 设计要求 | `IsCounterElement_` 实现 | ✅ 符合 |
| 光暗互克 | 设计要求 | 代码中处理 | ✅ 符合 |
| 茶道连携可视化 | 设计要求 | `AddLog` 输出连携日志 | ⚠️ 仅日志，无专用 UI |

---

## 五、修复优先级建议

### 立即修复（P0）

1. **TBD-003**：明确战斗中间态存档策略
2. **B-T006/B-T007**：战斗撤离与崩溃恢复测试

### 下个迭代修复（P1）

1. **TBD-001**：元素枚举双轨映射实现
2. **TBD-002**：失衡段胜利条件语义统一
3. **TBD-004**：茶道连携配置化

### 计划中修复（P2）

1. **TBD-005**：反噬数值配置化
2. **TBD-006**：季节更替边界条件测试
3. **TBD-007**：旧档迁移字段校验

---

## 六、附录

### A. 代码位置索引

| 文件 | 关键函数 | 行号 |
|------|----------|------|
| `BattleField.cpp` | `ParseElement_` | 33-42 |
| `BattleField.cpp` | `InitializeImbalanceSegments_` | 1195-1213 |
| `BattleField.cpp` | `ApplySkillHit_` | 1019-1104 |
| `BattleField.cpp` | `TryTriggerTeaCombo_` | 1222-1261 |
| `BattleField.cpp` | `IsCounterElement_` | 125-141 |
| `CropGrowthSystem.cpp` | `HandleSeasonChanged` | 227-247 |
| `GameAppSave.cpp` | `MigrateLegacyLinesToV5` | 294-315 |
| `BattleUI.cpp` | `SyncSpiritBar_` | 365-398 |

### B. 设计文档对照

| 文档 | 相关章节 |
|------|----------|
| `战斗数值_武器_任务技能_灵兽技能_统一设计稿.md` | 1.2 元素体系、1.3 净化公式、v1.1 升华补丁 |
| `战斗数据接入计划_元素迁移_探索战斗技能.md` | 6.1-6.3 机制增补 |
| `污染灵体完全设计手册.md` | 1.3 元素与克制、8.1 DEBUFF 表 |
| `TASKS.md` | Phase 12 测试任务 |

---

**报告结束**

*本报告基于静态代码分析生成，部分结论需通过动态测试验证。建议优先执行 P0 用例进行实际游戏测试。*
