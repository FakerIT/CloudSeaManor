# Issue 标签定义

本文档定义项目中使用的所有 Issue 和 PR 标签，包括颜色、含义和使用场景。

## 标签颜色规范

| 颜色 | 用途 |
|------|------|
| 🔴 红色 (#d73a4a) | 阻塞/紧急 |
| 🟠 橙色 (#d26937) | 美术/资源 |
| 🟡 黄色 (#fbca04) | 中优先级 |
| 🟢 绿色 (#008672) | 增强/成功 |
| 🔵 蓝色 (#0052cc) | 重构/技术 |
| 🔷 靛蓝 (#0451a5) | 信息/说明 |
| 🟣 紫色 (#8b5cf6) | 测试/质量 |
| ⚪ 灰色 (#6a737d) | 维护/其他 |

---

## 类型标签 (Type Labels)

| 标签 | 颜色 | 描述 | 使用场景 |
|------|------|------|----------|
| `bug` | 🔴 红色 | 功能缺陷或错误 | 报告程序错误、崩溃、无效行为 |
| `enhancement` | 🟢 绿色 | 功能增强 | 改进现有功能、增加新功能 |
| `feature` | 🟢 绿色 | 新功能请求 | 全新功能提案 |
| `refactor` | 🔵 蓝色 | 代码重构 | 改善代码结构但不改变功能 |
| `docs` | ⚪ 灰色 | 文档更新 | README、注释、API 文档更新 |
| `test` | 🟣 紫色 | 测试相关 | 添加/修改测试代码 |
| `assets` | 🟠 橙色 | 美术/音频资源 | 精灵图、音效、音乐等资源 |

---

## 优先级标签 (Priority Labels)

| 标签 | 颜色 | 描述 | 使用场景 |
|------|------|------|----------|
| `P0-critical` | ⬛ 黑色 (#000000) | 阻塞性问题 | 崩溃、数据丢失、阻塞发布 |
| `P1-high` | 🔴 红色 | 高优先级 | 本迭代必须完成，影响核心体验 |
| `P2-medium` | 🟡 黄色 | 中优先级 | 计划内但可延后 |
| `P3-low` | ⚪ 灰色 | 低优先级 | 有空再做，锦上添花 |

### 优先级定义

```
P0: 立即修复，阻塞一切
├── 游戏崩溃
├── 存档损坏
├── 无法编译
└── 核心功能完全失效

P1: 本周完成，影响体验
├── 主要玩法问题
├── UI 显示异常
├── 数值严重失衡
└── 关键路径不通

P2: 计划内，可延后
├── 功能不完整
├── 次要问题
├── 性能优化
└── 代码改进

P3: 空闲时处理
├── UI 美化
├── 文案优化
├── 注释补充
└── 技术债务清理
```

---

## 层级标签 (Layer Labels)

| 标签 | 颜色 | 描述 | 使用场景 |
|------|------|------|----------|
| `layer:app` | 🔷 靛蓝 | 应用层 | main.cpp、程序入口、生命周期 |
| `layer:engine` | 🔷 靛蓝 | 引擎层 | 渲染、输入、系统协调 |
| `layer:domain` | 🔷 靛蓝 | 领域层 | 纯玩法规则、游戏逻辑 |
| `layer:infra` | 🔷 靛蓝 | 基础设施层 | IO、配置、存档、解析 |

---

## 特殊标签 (Special Labels)

| 标签 | 颜色 | 描述 | 使用场景 |
|------|------|------|----------|
| `breaking` | 🔴 红色 | 破坏性变更 | API 变更、存档格式变更、不可逆改动 |
| `tech-debt` | 🟡 黄色 | 技术债务 | 临时解决方案、待重构代码 |
| `good-first-issue` | 🟢 绿色 | 适合新手 | 简单的第一个任务 |
| `needs-design` | 🟠 橙色 | 需要设计确认 | 功能设计待讨论 |
| `needs-tests` | 🟣 紫色 | 需要测试 | 缺少测试覆盖 |
| `needs-docs` | ⚪ 灰色 | 需要文档 | 缺少使用说明 |
| `wontfix` | ⚪ 灰色 | 不会修复 | 明确不处理的问题 |
| `duplicate` | ⚪ 灰色 | 重复问题 | 已有相同 Issue |
| `question` | 🔷 靛蓝 | 问题/疑问 | 需要进一步澄清 |

---

## 状态标签 (Status Labels)

| 标签 | 颜色 | 描述 | 使用场景 |
|------|------|------|----------|
| `in-progress` | 🔵 蓝色 | 进行中 | 正在处理的 Issue |
| `in-review` | 🟣 紫色 | 代码审查中 | PR 等待 review |
| `blocked` | 🔴 红色 | 被阻塞 | 等待其他 Issue 或决策 |
| `waiting-feedback` | 🟡 黄色 | 等待反馈 | 等待用户或 reviewer 回复 |

---

## 系统标签 (System Labels)

用于标识 Issue 相关的游戏系统。

| 标签 | 描述 |
|------|------|
| `system:farming` | 农业/种植系统 |
| `system:combat` | 战斗系统 |
| `system:npc` | NPC/社交系统 |
| `system:dialogue` | 对话系统 |
| `system:inventory` | 背包/物品系统 |
| `system:save` | 存档系统 |
| `system:audio` | 音频系统 |
| `system:ui` | 用户界面 |
| `system:map` | 地图/场景 |
| `system:spirit-realm` | 灵界系统 |
| `system:festival` | 节日系统 |
| `system:pet` | 灵兽系统 |
| `system:build` | 构建/CI/CD |

---

## 版本标签 (Version Labels)

| 标签 | 描述 |
|------|------|
| `version:0.1.0` | MVP 原型阶段 |
| `version:0.2.0` | MVP 完成 |
| `version:0.3.0` | 美术集成 |
| `version:0.5.0` | Demo 可玩 |
| `version:1.0.0` | Early Access |

---

## 标签使用建议

### 新建 Issue 时

1. 至少添加一个 **类型标签**（bug/enhancement/feature）
2. 添加一个 **优先级标签**（P0-P3）
3. 如涉及特定系统，添加 **系统标签**
4. 根据影响范围添加 **层级标签**

### 创建 PR 时

1. 继承 Issue 的所有标签
2. 添加 `in-review` 状态标签
3. 如有破坏性变更，添加 `breaking` 标签

### Issue 关闭时

- `bug` → 确认已修复
- `enhancement`/`feature` → 确认已实现
- `refactor` → 确认已完成
- 其他 → 根据实际情况保留或移除

---

## GitHub Actions 标签同步

如需自动同步标签，可使用 [github-label-sync](https://github.com/Financial-Times/github-label-sync)：

```bash
npm install -g github-label-sync
github-label-sync --token YOUR_TOKEN \
  --labels labels.yml \
  --repository owner/repo
```
