# 《云海山庄》前端 UI 开发任务清单

> **版本**: 1.0
> **日期**: 2026-04-22
> **项目阶段**: V0.1.0 MVP 原型阶段
> **技术栈**: C++20 / SFML 3.0.2 / PixelArt / JSON 配置驱动
> **前置说明**: 本清单严格遵循《云海山庄》前端 UI 与后端拆分规范，前端仅负责**可视化界面渲染、交互逻辑实现、UI 资源适配**，不涉及领域逻辑与底层 IO。所有前端 UI 组件均继承或复用现有 `PixelUiPanel` / `PixelGameHud` 体系，依赖后端提供的接口和数据。

---

## 文档说明

### 拆分原则

| 原则 | 说明 |
|------|------|
| **界面渲染独立** | UI 只负责将后端数据可视化，不执行业务逻辑 |
| **交互转发** | 用户交互事件转发给后端 engine 层处理 |
| **纯 SFML 绘制** | 所有渲染使用 SFML Graphics API，不在 engine 层以外引入渲染依赖 |
| **复用优先** | 优先复用现有 `PixelUiPanel` / `PixelProgressBar` / `PixelToolbar` 等组件 |
| **配置驱动** | 布局常量在 `ui_layout.json` / `PixelUiConfig.hpp` 中定义，代码无硬编码 |

### 前端 UI 与后端接口约定

所有前端 UI 面板应通过以下模式与后端交互：

```cpp
// 1. 数据通过 GameRuntime / GameWorldState 获取（后端提供）
// 2. UI 面板只负责展示，不修改后端状态
// 3. 用户操作 → 调用后端接口 → 后端更新状态 → UI 通过脏标记刷新
// 4. 面板不持有业务数据，只持有展示用视图数据

// 示例：云海预报面板
class CloudSeaForecastPanel : public PixelUiPanel {
public:
    void Refresh(const CloudForecastData& data); // 后端注入数据
    // 不持有 CloudSystem，调用 cloudSystem_->GetForecast() 获取
};
```

### 渲染层级（由后到前）

```
1. BackgroundScene        — 游戏场景（后端渲染）
2. AuraOverlay          — 灵气/云海色调（后端渲染）
3. DayNightGlow         — 昼夜光晕（后端渲染）
4. Minimap              — 迷你地图（M 键）
5. DialogueBox           — 对话框
6. InventoryGrid         — 背包（I 键）
7. QuestMenu             — 任务面板（F 键）
8. TopRightInfo          — 右上角常驻信息
9. BottomRightStatus     — 右下角体力/金币
10. Toolbar              — 快捷栏（底部）
11. WorldTip             — 世界交互提示（头顶）
12. NotificationBanner   — 通知横幅
13. DebugInfo            — 调试面板（F3）
```

---

## 任务总览

| 分类 | 任务数 | 优先级 |
|------|--------|--------|
| **G1 — 像素美术组件库** | 6 | P1 |
| **G2 — 核心 HUD 完善** | 8 | P0 |
| **G3 — 主 UI 面板开发** | 14 | P0/P1 |
| **G4 — 扩展 UI 面板开发** | 12 | P1/P2 |
| **G5 — 交互与动效系统** | 7 | P1 |
| **G6 — 美术资源规划** | 5 | P1 |
| **总计** | **52** | |

---

## G1 — 像素美术组件库（P1）

> 目标：完善像素 UI 基础组件，为所有面板提供统一的设计语言和渲染基元。

### UI-001 | 像素调色板系统扩展

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
在 `PixelArtStyle.hpp` 中补充 `ColorPalette` 类的四季主题色查询接口（`GetSeasonTheme(Season)`），当前仅定义了基础调色板（21 色），需扩展为支持春/夏/秋/冬四套主题色查询。

**技术要求**：
- 扩展 `ColorPalette` 增加 `SeasonTheme` 结构体，包含 `background`、`accent`、`text`、`highlight` 四个 `sf::Color` 字段
- 新增 `static SeasonTheme GetSeasonTheme(Season season)` 接口
- 在 `CLOUD_SEA_MANOR_UI_DESIGN.md` 附录 B 的四季配色基础上实现
- 复用现有常量，不引入新的魔法数字
- **不涉及任何业务逻辑**，纯数据查询

**交付标准**：
- `PixelArtStyle.hpp` 新增 `SeasonTheme` 结构体和 `GetSeasonTheme()` 方法
- 4 套季节配色均可通过 `ColorPalette::GetSeasonTheme(season)` 获取
- 单元测试验证 4×4=16 个颜色值正确返回

---

### UI-002 | 像素边框绘制组件完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 `PixelArtStyle::DrawPixelBorder()` 方法，支持可变边框宽度（当前仅 1px），并补充 8×8 像素角块的绘制逻辑。

**技术要求**：
- 在 `PixelArtStyle.cpp` 中实现 `DrawPixelBorder(sf::RenderTarget&, sf::FloatRect, float border_width)` 重载
- 角块绘制：`PixelBorderConfig::CornerBlockSize`（当前 8px）固定大小
- 横边/竖边：按 `border_width` 参数重复填充
- 内阴影效果：同一颜色深 2 度（调用 `ColorPalette::Darken()`），偏移 1px
- 使用 `sf::VertexArray` 绘制，性能优先（顶点预计算）
- **纯渲染逻辑，不涉及游戏数据**

**交付标准**：
- `DrawPixelBorder()` 支持 1px 和 2px 两种边框宽度
- 角块装饰、四边填充、内阴影三种效果均可见
- 性能：单帧绘制 10 个面板边框 < 0.5ms

---

### UI-003 | 像素字体渲染器中英文混合支持

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
扩展 `PixelFontRenderer`，支持中文回退到系统像素字体，英文使用位图字形（或等宽渲染）。

**技术要求**：
- 当前 `PixelFontRenderer` 使用 TTF 字体模拟像素感，需补充 fallback 机制
- ASCII 字符（0x20-0x7E）使用等宽渲染模式（scale=1.0）
- 非 ASCII（中文）回退到系统字体（如 `msyh.ttc`）
- 新增 `DrawTextWithFallback()` 接口，自动判断字符类型
- 从 `ui_layout.json` 读取 `fonts.primary` / `fonts.fallback` 路径

**交付标准**：
- 中英文混合文本正确渲染（中文回退到系统字体，英文用 TTF）
- 支持加粗/普通两种字重
- 至少支持以下字号：10px、12px、14px、16px

---

### UI-004 | 像素进度条组件完善（多状态）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
扩展 `PixelProgressBar`，支持更多进度条类型（体力/灵气/好感/作物/工坊/契约进度条），各类型有独立的颜色状态和尺寸。

**技术要求**：
- 复用现有 `PixelProgressBar` 类，新增以下变体接口：
  - `SetStaminaStyle()` — 体力条（满蓝/正常绿/低红）
  - `SetFavorStyle()` — 好感心条（满心/半心/空心 12×12px 心形图标）
  - `SetCropStyle()` — 作物生长条（64×8px）
  - `SetWorkshopStyle()` — 工坊进度条（360×12px）
  - `SetContractStyle()` — 契约进度条（160×10px）
- 低值警告闪烁动画（`<20%` 时 0.5s 闪烁间隔）
- 从 `PixelUiConfig.hpp` 的 `StaminaBarConfig` 读取尺寸常量

**交付标准**：
- `PixelProgressBar` 支持 5 种样式变体
- 低值闪烁动画在 `<20%` 时触发
- 所有颜色从 `ColorPalette` 常量获取，无硬编码色值

---

### UI-005 | 通用工具提示组件（Tooltip）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现 `PixelTooltip` 独立组件，用于物品悬停提示、NPC 名字标签、交互提示。

**技术要求**：
- 创建 `PixelTooltip.hpp/.cpp`，继承自 `PixelUiPanel` 的轻量版
- 样式：背景 `ColorPalette::Cream`（透明度 250/255）、1px `BrownOutline` 边框
- 内边距 8px，最大宽度 200px，文字 12px `TextBrown`
- 显示延迟：300ms（防止频繁闪烁）
- 内容格式：`[物品名称]（16px 加粗）\n[品质标签]（12px 彩色）\n---\n[描述文字]（12px）\n[价格/效果]（12px 金色）`
- 跟随鼠标位置（偏移 12px 避免遮挡）

**交付标准**：
- `PixelTooltip` 可在任何面板中被复用
- 支持多行文本和品质颜色标签
- 显示延迟 300ms，离开立即消失

---

### UI-006 | 通知横幅系统（NotificationBanner）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现 `PixelNotificationBanner` 组件，支持升级/成就/节日开始/大潮预警等通知的淡入-停留-淡出动画。

**技术要求**：
- 创建 `PixelNotificationBanner.hpp/.cpp`
- 位置：屏幕顶部居中（y=16），宽 600px × 高 36px
- 背景：`ColorPalette::Cream` + 2px `ColorPalette::LightBrown` 顶部装饰条
- 动画时序：淡入 0.3s → 停留 3.0s → 淡出 0.3s
- 可堆叠多个通知（垂直排列，间隔 4px）
- 通知队列管理：最多同时显示 3 条

**交付标准**：
- `PixelNotificationBanner` 支持队列管理和自动弹出
- 动画时序可配置（淡入/停留/淡出时长）
- 多通知垂直堆叠正确

---

## G2 — 核心 HUD 完善（P0）

> 目标：完善玩家日常接触最多的 HUD 元素，确保基础信息展示正确、美观、稳定。

### UI-007 | 右上角信息面板重构

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
重构 `PixelGameHud::RenderTopRightInfo_()` 方法，将右上角日期/季节/时间/云海/任务提示整合为一个结构化的像素风格面板，替代当前占位矩形。

**技术要求**：
- 位置和尺寸引用 `TopRightInfoConfig`（998, 6），尺寸 276×64px
- 布局：上下两行显示
  - 第一行：`第N天 HH:MM` + `[季节图标] 季节名`
  - 第二行：`[天气图标] 天气名 · 明日预报` + `[灵气图标] 灵气阶段名`
  - 新任务提示：`❗` 图标，0.5s 闪烁
- 背景：`ColorPalette::Cream`，1px `BrownOutline` 边框
- 字体：12px，使用 `TextStyle::TopRightInfo()`

**交付标准**：
- 右上角面板像素风格完整（宣纸质感 + 像素边框）
- 所有信息（时间/季节/天气/云海/灵气/任务提示）均可显示
- 天气图标随 `CloudSystem` 状态变化动态更新

---

### UI-008 | 右下角体力/金币面板重构

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 `PixelGameHud::RenderBottomRightStatus_()` 方法，重构体力条、金币显示、快捷键提示区域。

**技术要求**：
- 位置/尺寸引用 `BottomRightStatusConfig`（998, 658），尺寸 276×58px
- 体力条：272×12px，使用 `PixelProgressBar` 的 `SetStaminaStyle()`
- 金币：`💰 {gold}` 格式，14px 金色 `CoinText()`，`ColorPalette::CoinGold`
- 快捷键提示：10px 深灰色，按 `gameplay.cfg` 键位映射动态显示 `[E] 交互 [Q] 工具`
- 低体力警告：<20% 体力条红色闪烁 + `!!体力低!!` 文案（已在 PixelGameHud 中实现，需确认渲染链路完整）

**交付标准**：
- 体力条满/正常/低三态颜色正确
- 金币数字千分位格式化显示（如 `12,345`）
- 快捷键提示文本随配置变化

---

### UI-009 | 底部快捷栏（Toolbar）完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 `PixelToolbar` 组件，确认 12 格工具栏的选中高亮、快捷键标注、数量渲染功能完整，并增加键盘焦点导航支持。

**技术要求**：
- 12 格布局，引用 `ToolbarConfig`（332, 662），总宽 616px × 高 62px
- 格子尺寸 48×48px，间距 4px
- 选中格：2px `ColorPalette::ActiveGreen` 绿色描边 + 脉冲动画（已部分实现）
- 快捷键标注：格下方 10px 小字 `1` `2` ... `0` `-` `=`
- 数量标注：右下角 12px 粗体
- 新增：键盘方向键导航支持（WSAD 选择格子）
- 背景：半透明米色（透明度 230/255），1px `BrownOutline` 边框

**交付标准**：
- 工具栏 12 格完整渲染
- 选中格金色脉冲高亮可见
- 键盘方向键可切换选中格子
- 快捷键 1-0/-/= 标注正确

---

### UI-010 | 对话框（DialogueBox）完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
确认 `PixelDialogueBox` 的打字机效果、头像渲染、选项列表、继续提示功能完整，补充 NPC 名字显示和选项 hover 效果。

**技术要求**：
- 位置/尺寸引用 `DialogueBoxConfig`（40, 520），宽 1200px × 高 160px
- NPC 头像：左侧 48×48px，像素风格（占位方块）
- 名字：16px 加粗 `ColorPalette::DeepBrown`
- 文字：14px `ColorPalette::TextBrown`，行高 1.5 倍，每行 38 字
- 打字机效果：40ms/字符（`DialogueBoxConfig::TypingSpeedMs`），Enter 跳过
- 继续提示：右下角 `▼` 闪烁（0.5s 间隔）
- 选项：Hover 浅黄（`HighlightYellow`），选中绿底（`ActiveGreen`）
- 对话框打开时其他面板自动隐藏（对话优先）

**交付标准**：
- 打字机效果流畅，Enter 可跳过
- 选项列表 hover/选中效果正确
- NPC 名字和头像正确显示
- 对话框打开时 HUD 其他面板正确隐藏

---

### UI-011 | 任务面板（QuestMenu）功能完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 `PixelQuestMenu`，使其支持任务列表显示、任务状态（未接取/进行中/已完成）、奖励预览。

**技术要求**：
- 位置/尺寸引用 `QuestMenuConfig`（820, 150），宽 360px × 高 420px
- 标签栏高度 40px，每个标签 72px 宽（图标 24×24 + 文字 14px）
- 选中标签底部 3px `ActiveGreen` 下划线
- 任务条目：每条 48px 高（标题 + 进度条 + 奖励预览）
- 完成状态图标：`✓` 绿色（已完成）/ `●` 橙色（进行中）/ `○` 灰色（未开始）
- 进度条：160×10px `PixelProgressBar` 的 `SetContractStyle()`

**交付标准**：
- 任务面板像素风格完整
- 任务列表支持滚动（`ScrollPanel` 复用）
- 任务状态图标正确显示

---

### UI-012 | 迷你地图（Minimap）完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 `PixelMinimap` 组件，实现简化场景轮廓、玩家位置点、重要地点标注。

**技术要求**：
- 位置/尺寸引用 `MinimapConfig`，默认 512×512px，居中显示
- 缩小比例：1:4（每 4 个世界格合并为 1 个地图格）
- 玩家位置：亮黄圆点 `#FFD700`，4px 半径
- NPC 位置：白色小点
- 重要地点：彩色图标（农场=绿、商店=橙、NPC=白）
- 当前位置名称："云海农场 · 主屋前" 标注
- M 键呼出全屏大地图，Esc 关闭

**交付标准**：
- 迷你地图正确显示玩家和 NPC 位置
- 重要地点图标颜色区分正确
- M 键呼出/关闭正确响应

---

### UI-013 | 通知提示（Hint）系统完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 HUD 中的世界交互提示（目标头顶提示）和今日小贴士显示。

**技术要求**：
- 世界提示（WorldTip）：玩家靠近可交互对象时，对象头顶显示 `[E] 收获 春小麦 ×3`
  - 位置：对象头顶 +10px
  - 背景：半透明黑色 + 白色边框
  - 动画：轻微上下浮动（0.5s 周期，2px 振幅）
- 收集提示：`[作物图标] ×3  已收入背包`，屏幕中央偏上，停留 1.5s 后淡出
- 今日小贴士：左下角提示区（`hint_panel`），显示云海预报、NPC 位置等
- 从 `gameplay.cfg` 读取 `hint_display_duration` 配置

**交付标准**：
- 交互高亮提示正确显示在对象头顶
- 收集提示淡入淡出动画流畅
- 提示内容随游戏状态动态更新

---

### UI-014 | 主菜单像素化改造

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现像素风格主菜单，替代当前直接进入游戏的逻辑。

**技术要求**：
- 标题："云海山庄" 使用 `PixelFontRenderer` 渲染大字（可用现有 TTF 放大模拟像素感）
- 三个按钮：新游戏 / 继续（无存档时灰色禁用）/ 设置
- 按钮样式：120×40px，`ColorPalette::LightCream` 背景，1px `BrownOutline` 边框
- Hover：背景变为 `HighlightYellow`，2px 绿色下划线
- 背景：显示游戏主场景缩影（半透明叠加）
- 键盘导航：WS 上下选择，Enter 确认，Esc 无效

**交付标准**：
- 主菜单像素风格完整（宣纸质感 + 像素边框）
- 三个按钮均可用键盘/鼠标操作
- 无存档时"继续"按钮灰色禁用

---

## G3 — 主 UI 面板开发（P0/P1）

> 目标：实现玩家日常使用的核心面板（背包、设置、存档、云海预报、玩家状态）。

### UI-015 | 背包面板像素化改造

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 `PixelInventoryGrid`，实现完整的物品管理界面，包含 5 标签页、8×4 网格、物品详情面板。

**技术要求**：
- 位置/尺寸引用 `InventoryGridConfig`（320, 120），宽 640px × 高 480px
- 5 个标签页（物品/工具/装备/收集/社交），`InventoryTab` 枚举已定义
- 物品格：8×4 = 32 格，每格 48×48px，间距 4px
- 详情面板：右侧 200px 宽，显示名称/品质/描述/价格/操作按钮
- 鼠标悬停显示 tooltip（物品名称/描述/价格）
- 右键快捷使用/丢弃，左键选中拖拽排序
- 支持按类型/品质/数量/价值排序（点击标签栏图标）
- 操作按钮：[使用] [出售] [送礼]（后端接口：点击按钮调用对应后端方法）

**交付标准**：
- 背包面板像素风格完整
- 5 标签页切换正常
- 物品悬停 tooltip 显示正确
- 选中物品详情面板显示完整信息

---

### UI-016 | 设置面板（SettingsPanel）完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 `PixelSettingsPanel`，实现音量控制（全局/Music/BGM/SFX）、画质选择、按键绑定。

**技术要求**：
- 继承 `PixelUiPanel`，宽 480px × 高 360px，居中显示
- 音量滑块：Music / BGM / SFX 三通道独立（使用 `PixelProgressBar` 样式）
- 画质选择：窗口化 / 全屏（使用 `PixelUiPanel` 的 TabBar 切换）
- 按键绑定：点击按钮后监听下一个按键，替换绑定
- 保存后写入 `configs/audio.json`（后端 infrastructure 层处理写入）
- Esc 关闭并自动保存

**交付标准**：
- 音量滑块可拖动，数值实时反馈
- 画质切换正确（调用后端方法）
- 按键绑定重映射功能可用

---

### UI-017 | 多槽位存档/读档界面

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现 3 槽位存档界面，显示存档缩略图、游玩天数、季节、金币等信息。

**技术要求**：
- 槽位卡片：每张 560×120px，共 3 个槽位垂直排列
- 每个槽位显示：存档时间（yyyy-mm-dd）、游玩天数、季节、天气、缩略图（160×90px）
- 空槽位：虚线边框 + "点击创建新存档" 文字
- 删除按钮：弹出二次确认对话框
- 覆盖确认：覆盖已有存档时弹出确认
- 键盘导航：WS 上下选择槽位，Enter 读取，Delete 删除
- 后端接口：`SaveSlotManager` 提供槽位数据（infrastructure 层）

**交付标准**：
- 3 个存档槽位均可显示和操作
- 空槽位、有存档槽位、覆盖确认三种状态正确
- 键盘/鼠标操作均可用

---

### UI-018 | 云海预报面板（CloudSeaForecastPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现云海预报详情弹窗，显示今日云海状态、明日预报、大潮倒计时、今日推荐活动。

**技术要求**：
- 创建 `PixelCloudSeaForecastPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：560×480px，居中显示
- 标题栏 32px，关闭按钮（×）右上角
- 显示内容：
  - 今日云海状态（4 种天气对应的场景配色和描述）
  - 作物/灵气加成数值
  - 明日天气预报
  - 大潮倒计时（`PixelProgressBar` 进度条）
  - 今日推荐活动列表（最多 3 条，可点击高亮对应世界位置）
  - 云海香炉解锁状态
- F5 键或点击右上角云海图标呼出，Esc 关闭
- 数据来源：后端 `CloudSystem::GetForecast()` 接口

**交付标准**：
- 云海预报面板像素风格完整
- 4 种天气状态均有对应颜色和描述
- 大潮倒计时进度条正确显示

---

### UI-019 | 玩家状态面板（PlayerStatusPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现玩家状态详情面板，显示等级、属性、契约进度、今日数据统计。

**技术要求**：
- 创建 `PixelPlayerStatusPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：480×520px，居中显示
- 显示内容：
  - 角色头像（64×64px 占位方块）
  - 名字/等级/山庄阶段
  - 灵气值进度条（`PixelProgressBar`）
  - 山庄评价星级（★）
  - 总资产金币
  - 体力/灵气/疲劳状态条
  - 今日数据统计（采集/制作/社交/探索/节日次数）
  - 契约进度（6 卷总完成度）
- C 键或点击角色头像呼出，Esc 关闭
- 数据来源：后端 `Player` / `CloudGuardianContract` / `GameClock` 数据

**交付标准**：
- 玩家状态面板完整显示所有信息
- 各进度条颜色和数值正确
- 契约进度以 6 格指示器显示

---

### UI-020 | 云海日报自动推送

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现每日 6:00 自动显示的云海日报浮窗，叠加在 HUD 上方。

**技术要求**：
- 复用 `PixelNotificationBanner` 组件
- 内容格式：
  ```
  ☁ 今日云海: {天气}
  作物加成: +{n}%
  大潮倒计时: {n}天
  ```
- 每日 6:00 自动弹出（由后端 `GameClock` 触发 `DayChangedEvent`）
- 停留 4.0s 后自动淡出
- 点击可展开为完整云海预报面板（UI-018）

**交付标准**：
- 每日 6:00 自动显示云海日报
- 4.0s 后自动淡出消失

---

### UI-021 | 今日推荐三件事面板

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现每日推荐活动面板，在屏幕左中部显示 3 条推荐任务。

**技术要求**：
- 位置：屏幕左中部（可整合到左上角 HUD 区域）
- 尺寸：约 200×120px
- 内容：生产/社交/探索各 1 条
- 推荐逻辑由后端 `RecommendationSystem` 提供（engine 层）
- 点击推荐项可高亮对应世界位置（后端提供坐标）
- 每日 6:00 更新

**交付标准**：
- 每日显示 3 条推荐活动
- 点击推荐项有视觉反馈

---

### UI-022 | 新手引导气泡系统

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现前 7 天新手引导的分步气泡提示，包含高亮区域、箭头指向、文字说明。

**技术要求**：
- 半屏遮罩 + 高亮区域 + 箭头指向 + 文字气泡组合
- 引导步骤（参考 `CLOUD_SEA_MANOR_UI_DESIGN.md` §5.3）：
  - 第 1 天：移动操作（高亮玩家角色）
  - 第 1 天：收获作物（高亮作物田）
  - 第 1 天：使用工具（高亮快捷栏）
  - 第 2 天：查看背包（I 键）
  - 第 2 天：出售物品（高亮商店 NPC）
  - 第 3 天：云海预报（高亮右上角天气）
  - 第 3 天：赠送礼物（G 键）
  - 第 4 天：查看契约（H 键）
  - 第 5 天：进入灵界（高亮灵界入口）
  - 第 6 天：喂养灵兽（高亮灵兽园）
  - 第 7 天：节日参与（高亮节日地点）
- 引导气泡最大宽度 320px，背景 `Cream`，箭头指向高亮区域
- 底部按钮：[上一条] [下一条 →]

**交付标准**：
- 11 步新手引导气泡正确显示
- 高亮区域、箭头、文字气泡组合完整
- 进度保存（已引导步骤不重复显示）

---

### UI-023 | 交互高亮提示强化

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
增强可交互对象的视觉高亮效果，使玩家清楚知道哪些对象可以交互。

**技术要求**：
- 可交互对象（作物/NPC/宝箱）：半透明白色矩形轮廓，1px 虚线
- Hover 时：显示物品名称 tooltip（复用 UI-005）
- 采集时：物品图标 + "+1" 上浮动画（+0.3s 淡入，+0.2s 上移消失）
- 采集提示：`[作物图标] ×3  已收入背包`，位置屏幕中央偏上，停留 1.5s 后淡出

**交付标准**：
- 可交互对象有明确的视觉高亮
- 采集提示动画流畅（淡入→上移→消失）

---

### UI-024 | 存档缩略图生成

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现存档时的游戏画面快照功能，生成存档缩略图供读档界面显示。

**技术要求**：
- 存档时调用 `sf::Texture::update(window)` 截取当前画面
- 缩略图尺寸：160×90px（`InventoryGridConfig` 已定义）
- 保存为 `saves/slot_{n}/thumbnail.png`
- 读档选择界面加载并显示缩略图
- 若缩略图不存在，显示默认占位图（淡灰色 + "无预览" 文字）

**交付标准**：
- 存档成功时生成缩略图
- 读档界面正确显示缩略图
- 无缩略图时显示占位图

---

## G4 — 扩展 UI 面板开发（P1/P2）

> 目标：实现各游戏系统的专属 UI 面板，丰富游戏体验。

### UI-025 | 茶园/作物面板（TeaGardenPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现茶园管理面板，显示灵茶生长状态、云海加成预览、今日操作按钮。

**技术要求**：
- 创建 `PixelTeaGardenPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：560×420px，居中显示
- 显示内容：
  - 当前云海状态 + 品质加成预览（4 格天气对比）
  - 当前灵气 + 圣品概率
  - 茶田状态列表（每行：区域名/作物名/进度条/品质标签）
  - 作物生长条：160×10px，使用 `PixelProgressBar`
  - 灵茶品质标签：普通(灰)/优质(绿)/珍品(蓝)/圣品(金)
  - 今日操作按钮：[💧浇水] [🌿施肥] [🍃采摘] [🔧修整]
  - 云海加成预览：4 格天气格，每格显示对应加成
- K 键呼出，Esc 关闭

**交付标准**：
- 茶园面板像素风格完整
- 生长进度条实时更新
- 操作按钮布局正确

---

### UI-026 | 工坊面板（WorkshopPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现工坊加工面板，支持配方列表、制作队列、材料库存显示。

**技术要求**：
- 创建 `PixelWorkshopPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：640×480px，居中显示
- 显示内容：
  - 标签栏：[制茶] [加工] [酿造] [精炼] + 自动制作开关
  - 配方列表：滚动面板，每行 40px，显示材料消耗和产出
  - 当前制作队列：最多 3 个，`PixelProgressBar` 进度（360×12px）
  - 材料库存：当前持有材料数量，不足项红色高亮
  - 操作按钮：[开始制作] [取消制作]
- 配方数据从 `RecipeTable.csv` 读取（后端 `WorkshopSystem` 提供）
- W 键呼出，Esc 关闭

**交付标准**：
- 工坊面板完整显示配方和队列
- 进度条实时更新
- 材料不足项红色高亮

---

### UI-027 | 契约系统面板（ContractPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现云海守护者契约面板，支持 6 卷切换、任务条目显示、章节奖励展示。

**技术要求**：
- 创建 `PixelContractPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：720×560px，居中显示
- 显示内容：
  - 6 个卷册标签页（使用分页组件）
  - 当前卷名 + 进度条（6 卷总完成度）
  - 任务条目列表（每条 48px：状态图标 + 标题 + 进度 + 奖励预览）
  - 章节奖励区（完成全部云海之约获得的内容）
- 完成状态：`✓` 绿色 / `●` 橙色（进行中）/ `○` 灰色（未开始）
- 任务条目进度条：160×10px `PixelProgressBar`
- H 键呼出，Esc 关闭

**交付标准**：
- 6 卷册切换正常
- 任务条目完整显示

---

### UI-028 | NPC 社交面板（NpcDetailPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现 NPC 详情面板，显示好感度、心事件进度、互动按钮。

**技术要求**：
- 创建 `PixelNpcDetailPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：560×480px，居中显示
- 显示内容：
  - NPC 头像（64×64px 占位方块）
  - NPC 名字/称号/生日/喜好/厌恶
  - 好感等级条：160×14px，分 10 格显示心形（使用 `PixelProgressBar` 的 `SetFavorStyle()`）
  - 心事件进度：[✓]/[●]/[○] 各 10 个（H1-H10）
  - 今日互动次数
  - 送礼冷却状态
  - 可触发事件位置提示
  - 操作按钮：[💬对话] [🎁送礼] [📖查看喜好] [💒约会]
- N 键呼出，Esc 关闭

**交付标准**：
- NPC 详情完整显示好感和心事件进度
- 10 心进度条正确显示

---

### UI-029 | 灵兽面板（SpiritBeastPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现灵兽小屋面板，显示灵兽列表、饱食度、技能、互动按钮。

**技术要求**：
- 创建 `PixelSpiritBeastPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：640×520px，居中显示
- 显示内容：
  - 分类标签：[全部] [采集型] [守护型] [辅助型] [传说型] + 当前数量
  - 灵兽卡片：每张 300×120px，显示头像/状态/饱食条/操作按钮
  - 饱食度：80×10px `PixelProgressBar`
  - 状态图标：[跟随] [休息] [工作] [派遣]
  - 个性标签：12px 灰字（如"活泼好奇"）
  - 操作按钮：[喂食🍃] [互动💕] [派遣🗺] [休息😴]
  - 招募按钮：显示解锁条件
- B 键呼出，Esc 关闭

**交付标准**：
- 灵兽列表支持分类筛选
- 饱食度条实时更新

---

### UI-030 | 节日面板（FestivalPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现节日活动面板，显示节日倒计时、活动内容、参与奖励、节日预告。

**技术要求**：
- 创建 `PixelFestivalPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：560×480px，居中显示
- 显示内容：
  - 节日名称 + 倒计时（大字体 24px 金色）
  - 活动内容列表（进度条显示已完成/总数）
  - 参与奖励列表
  - 节日预告（最多 3 个即将到来的节日）
  - 参与模式：轻松参与（80%）/ 全力参与（100%）
  - 操作按钮：[前往活动地点] [查看奖励详情] [轻松参与✓]
- F 键或节日期间自动显示，Esc 关闭

**交付标准**：
- 节日面板像素风格完整
- 倒计时大字体金色显示
- 活动进度条正确

---

### UI-031 | 灵界探索面板（SpiritRealmPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现灵界探索面板，显示区域选择、难度模式、今日剩余次数。

**技术要求**：
- 创建 `PixelSpiritRealmPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：600×480px，居中显示
- 显示内容：
  - 难度模式：[轻松✓] [关闭战斗] [挑战]
  - 区域卡片：每张 540×80px（名称/等级/状态/掉落预览）
  - 锁定状态：🔒 图标 + 解锁条件文字
  - 当前云海加成高亮
  - 可探索次数：3/5 (今日剩余)
  - 操作按钮：[进入浅层] [查看掉落表] [挑战首领]
- R 键呼出，Esc 关闭

**交付标准**：
- 区域卡片显示正确
- 锁定/解锁状态区分明显

---

### UI-032 | 建筑升级面板（BuildingPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现建筑升级面板，显示建筑列表、升级条件、升级效果预览。

**技术要求**：
- 创建 `PixelBuildingPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：640×520px，居中显示
- 显示内容：
  - 分类标签：[全部] [生产] [住宿] [装饰] [功能] + 当前资金
  - 建筑列表（每行：图标/名称/等级/状态）
  - 升级中进度条
  - 升级条件：所需金币和材料，当前持有量（满足=绿色，不满足=红色）
  - 建筑详情区（升级效果预览）
  - 操作按钮：[升级] [查看外观] [取消]
- U 键呼出，Esc 关闭

**交付标准**：
- 建筑列表支持分类筛选
- 材料满足/不满足状态颜色区分正确

---

### UI-033 | 商店面板（ShopPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现商店购买界面，显示商品列表、库存、价格。

**技术要求**：
- 创建 `PixelShopPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：560×480px，居中显示
- 显示内容：
  - 商店名称标题
  - 商品列表（图标/名称/价格/库存）
  - 玩家持有金币
  - 当前选中商品详情
  - 操作按钮：[购买] [取消]
- 购买成功：播放音效 + 金币扣除动画
- 购买失败（金币不足/背包满）：红色提示文案

**交付标准**：
- 商品列表正确显示价格和库存
- 金币不足时购买按钮禁用或提示

---

### UI-034 | 邮件/订单面板（MailPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现邮件/订单面板，显示待收取物品、订单状态。

**技术要求**：
- 创建 `PixelMailPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：480×400px，居中显示
- 显示内容：
  - 邮件列表（发件人/主题/时间）
  - 邮件详情（物品列表/说明文字）
  - 操作按钮：[收取物品] [删除邮件]
- 新邮件到达时：右上角出现❗闪烁提示

**交付标准**：
- 邮件列表正确显示
- 新邮件闪烁提示正常

---

### UI-035 | 成就面板（AchievementPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现成就展示面板，显示已解锁/未解锁成就列表。

**技术要求**：
- 创建 `PixelAchievementPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：520×440px，居中显示
- 显示内容：
  - 已解锁成就（带金色边框）
  - 未解锁成就（灰色半透明）
  - 成就图标 + 名称 + 描述 + 完成条件
  - 已解锁成就计数：`已解锁: 3/20`
- 成就解锁时：自动弹出通知横幅（复用 UI-006）
- 从 `AchievementTable.csv` 读取数据（后端 `AchievementSystem` 提供）

**交付标准**：
- 已解锁/未解锁成就区分明显
- 解锁时通知横幅正确弹出

---

### UI-036 | 灵兽图鉴面板（BeastiaryPanel）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现灵兽图鉴面板，展示已发现/未发现的灵兽。

**技术要求**：
- 创建 `PixelBeastiaryPanel.hpp/.cpp`，继承 `PixelUiPanel`
- 尺寸：600×480px，居中显示
- 显示内容：
  - 分类筛选：[全部] [采集型] [守护型] [辅助型] [传说型]
  - 灵兽卡片网格（头像/名称/发现状态）
  - 已发现：彩色头像 + 名称
  - 未发现：灰色头像 + "???" + 发现条件
  - 选中灵兽详情（描述/技能/分布区域）

**交付标准**：
- 灵兽卡片网格正确显示
- 已发现/未发现状态区分明显

---

## G5 — 交互与动效系统（P1）

> 目标：完善 UI 交互细节和动画效果，提升玩家体验。

### UI-037 | UI 面板开关动画系统

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
为所有 UI 面板统一实现淡入淡出开关动画。

**技术要求**：
- 打开动画：0.15s 淡入（opacity 0→255）
- 关闭动画：0.10s 淡出（opacity 255→0）
- 在 `PixelUiPanel` 基类中实现 `FadeIn()` / `FadeOut()` 方法
- 面板打开时触发动画，动画结束后才响应输入
- 关闭时先播放动画，动画结束后才从渲染树移除

**交付标准**：
- 所有面板淡入淡出动画正常
- 动画期间输入被正确拦截

---

### UI-038 | 手柄/手柄适配完善

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
完善 UI 组件的手柄导航支持，确保所有面板均可用手柄操作。

**技术要求**：
- 所有交互元素最小热区 44×44px（Xbox/PS/Switch 手柄标准）
- 当前焦点元素显示绿色 2px 描边（`ControllerConfig::FocusRingThickness`）
- 方向键导航：WS 上下移动焦点，AD 左右（在 TabBar 时）
- Tab 键：切换标签页
- Enter/A 键：确认/选择
- Esc/B 键：返回/关闭
- 焦点切换时轻微呼吸动画（0.3s ease-in-out，±2px 振幅）
- 焦点扩大：在当前焦点元素周围添加 4px 额外热区（`NavigationPadding`）

**交付标准**：
- 所有面板支持手柄方向键导航
- 当前焦点绿色描边可见

---

### UI-039 | UI 声音反馈接入

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
为所有 UI 交互接入声音反馈。

**技术要求**：
- 在关键 UI 交互点调用 `AudioManager::PlaySFX()`：
  - 按钮 Hover：`ui_hover`
  - 按钮点击：`ui_click`
  - 面板打开：`ui_open`
  - 面板关闭：`ui_close`
  - 选项选择：`ui_select`
  - 错误提示：`ui_error`
  - 成就解锁：`achievement_unlock`
- 从 `configs/audio.json` 读取音效映射配置
- UI 音效与游戏音效独立控制（UI SFX 通道）

**交付标准**：
- 所有 UI 交互均有声音反馈
- UI 音效音量独立可控

---

### UI-040 | 加载画面（LoadingScreen）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现存档读取和场景切换时的加载画面。

**技术要求**：
- 全屏半透明遮罩（黑色透明度 180/255）
- 中央显示"正在加载..."文字
- 加载进度条（不确定进度条，动画效果）
- 可选：显示当前加载阶段提示（如"正在加载地图..."）
- 存档读取和 TMX 地图加载时显示

**交付标准**：
- 加载画面在存档/地图切换时正确显示
- 加载完成后自动消失

---

### UI-041 | 屏幕分辨率适配

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现 UI 的动态分辨率适配，支持 1280×720（PC）和动态适配（Switch）。

**技术要求**：
- 所有 UI 尺寸基于 1280×720 设计
- 动态缩放系数：`min(window_width / 1280.0f, window_height / 720.0f)`
- 所有坐标和尺寸乘以缩放系数后渲染
- 字体使用整数缩放保持像素锐利感
- Switch 适配：竖屏模式下的布局重新排列
- 从 `ui_layout.json` 读取基准分辨率配置

**交付标准**：
- 不同窗口尺寸下 UI 均正确显示
- 像素风格保持锐利（整数缩放）

---

### UI-042 | 像素缩放（Pixel Perfect Rendering）

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
确保所有 UI 渲染保持像素锐利感，禁止抗锯齿。

**技术要求**：
- 所有 SFML 渲染时设置 `sf::ContextSettings::antialiasingLevel = 0`
- 字体渲染使用整数坐标（`std::round()`）
- 所有缩放使用整数倍（1x, 2x, 3x）
- `sf::RenderStates` 设置 `sf::BlendMode::BlendAlpha` 并确保透明度正确
- 设置 `sf::Text::setSmooth(false)` 禁用字体平滑

**交付标准**：
- 所有 UI 渲染无抗锯齿模糊
- 像素风格保持锐利清晰

**已实现要点**：
- 字体与 UI 顶点坐标统一整数对齐（`std::round()`）
- 纹理与字体纹理禁用平滑（`setSmooth(false)`）
- UI 绘制统一显式使用 `sf::BlendMode::BlendAlpha`

---

### UI-043 | 右键菜单系统

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
实现右键上下文菜单，用于物品快速操作。

**技术要求**：
- 右键点击物品格时，在点击位置弹出上下文菜单
- 菜单内容（根据物品类型动态）：
  - 工具：使用 / 丢弃 / 装备
  - 消耗品：使用 / 丢弃 / 送礼
  - 种子：种植 / 丢弃
- 菜单外点击或 Esc 关闭
- 最多显示 4 个菜单项

**交付标准**：
- 右键菜单正确弹出
- 菜单项根据物品类型变化

---

## G6 — 美术资源规划（P1）

> 目标：规划并逐步引入像素美术资源，提升游戏视觉效果。

### UI-044 | UI 图集资源规划

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
规划 `assets/sprites/ui/` 目录下的 UI 图集结构，为未来美术资源制作提供规范。

**技术要求**：
- 规划 `ui_main.png`（1024×1024）：UI 主图集
  - 像素边框 tile（8×8 corner, 8×1 edge）
  - 按钮状态（默认/hover/pressed × 2 种尺寸）
  - 对话框样式
  - 进度条样式
  - 工具栏样式
- 规划 `ui_icons.png`（512×256）：小图标集
  - 天气图标 4×4 格（晴/薄雾/浓云/大潮）
  - 品质图标 4×1 格（普通/优质/稀有/传说）
  - 状态图标（心/星/锁/箭头等）
  - 按钮图标
- 导出 `.atlas.json` 元数据文件
- 当前阶段：使用代码生成的像素方块作为占位符

**交付标准**：
- UI 图集规划文档完成
- 至少 10 个核心图标有占位符

---

### UI-045 | NPC 头像图集规划

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
规划 13 位 NPC 头像精灵图集。

**技术要求**：
- 每个 NPC 头像尺寸：64×64px（`PixelInventoryGrid::SocialAvatarSize`）
- 额外状态头像（表情变化）：开心/普通/生气/惊喜 各 1 个
- 图集排列：13 NPC × 4 表情 = 52 帧，排列为 8×7 网格（最后一格空）
- 当前阶段：使用代码生成的彩色方块作为占位符（每个 NPC 不同颜色）

**交付标准**：
- NPC 头像规划文档完成
- 13 位 NPC 头像占位符正确（每人一独特颜色）

---

### UI-046 | 物品图标规划

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
规划物品图标图集，为所有可交互物品制作像素图标。

**技术要求**：
- 每种物品图标尺寸：16×16px 或 32×32px
- 图集分类：
  - `items_crop.png`：作物/种子（当前 30+ 种作物）
  - `items_tool.png`：工具（锄头/水壶/镰刀/斧头等）
  - `items_material.png`：材料（木材/石材/灵草等）
  - `items_product.png`：加工产品（茶包/灵尘等）
- 当前阶段：使用代码生成的彩色方块作为占位符
- 长期：使用社区像素资源包或自制图标

**交付标准**：
- 物品图标规划文档完成
- 所有物品类型有占位符

---

### UI-047 | 地图瓦片集规划

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
规划游戏地图瓦片集，包含地形、建筑、装饰物。

**技术要求**：
- 瓦片尺寸：32×32px（与 `GameConstants` 中的 `TileSize` 一致）
- 图集分类：
  - `tiles_farm.png`：农场地形（草地/泥土/石头/路径）
  - `tiles_spirit.png`：灵界地形（云雾/灵气池/彼岸花等）
  - `tiles_building.png`：建筑（主屋/工坊/温室等）
  - `tiles_decoration.png`：装饰物（围栏/井/信箱等）
- 当前阶段：使用代码生成的纯色瓦片作为占位符

**交付标准**：
- 地图瓦片规划文档完成
- 核心瓦片类型有占位符

---

### UI-048 | 粒子效果资源规划

**进度标识**：✅ 已完成（2026-04-22）

**任务描述**：
规划游戏中的粒子效果精灵图集。

**技术要求**：
- 粒子精灵尺寸：8×8px 或 16×16px
- 图集分类：
  - `particles_basic.png`：基础粒子（心形/星形/水滴/光点）
  - `particles_nature.png`：自然粒子（落叶/雪花/萤火虫）
  - `particles_magic.png`：魔法粒子（灵气/云雾/净化光效）
- 当前阶段：使用代码生成的简单形状作为占位符
- 参考 `CLOUD_SEA_MANOR_UI_DESIGN.md` §1.3 的云纹/玉石质感描述

**交付标准**：
- 粒子效果规划文档完成
- 至少 4 种基础粒子有占位符

---

## 附录 A：前端 UI 依赖后端接口清单

| UI 面板 | 后端接口/数据 | 后端层级 |
|---------|-------------|---------|
| 右上角信息 | `GameClock::GetDay()`, `GetTime()`, `GetSeason()`, `CloudSystem::CurrentState()` | domain + engine |
| 右下角体力 | `Stamina::GetCurrent()`, `GetMax()` | domain |
| 工具栏 | `Inventory::GetToolbarItems()` | domain |
| 对话框 | `DialogueEngine::GetCurrentDialogue()`, `NpcDialogueManager` | engine |
| 背包 | `Inventory::GetItems()`, `TryAddItem()`, `TryRemoveItem()` | domain |
| 设置 | `AudioManager::SetVolume()`, `InputManager::GetKeyBindings()` | engine + infrastructure |
| 存档 | `SaveSlotManager::GetSlotInfo()`, `SaveGameToSlot()`, `LoadGameFromSlot()` | infrastructure |
| 云海预报 | `CloudSystem::GetForecast()`, `GetTideCountdown()` | domain + engine |
| 玩家状态 | `Player::GetLevel()`, `GetSpiritEnergy()`, `CloudGuardianContract::GetProgress()` | domain |
| 茶园 | `TeaPlot::GetAll()`, `CropGrowthSystem::GetGrowthData()` | domain + engine |
| 工坊 | `WorkshopSystem::GetRecipes()`, `GetQueues()`, `StartCraft()` | engine |
| 契约 | `CloudGuardianContract::GetTasks()`, `CompleteTask()` | domain |
| NPC 社交 | `NpcActor::GetFavor()`, `GetHeartEvents()`, `GiveGift()` | domain + engine |
| 灵兽 | `SpiritBeastSystem::GetBeasts()`, `FeedBeast()`, `InteractWithBeast()` | engine |
| 节日 | `FestivalSystem::GetActiveFestival()`, `GetUpcoming()` | engine |
| 灵界 | `SpiritRealmManager::GetRegions()`, `GetTodayRemaining()` | engine |
| 建筑 | `BuildingSystem::GetBuildings()`, `UpgradeBuilding()` | engine |
| 商店 | `ShopSystem::GetProducts()`, `PurchaseItem()` | engine |
| 邮件 | `MailSystem::GetMails()`, `ClaimMail()` | engine |
| 成就 | `AchievementSystem::GetAchievements()`, `AchievementUnlocked()` | engine |

---

## 附录 B：前端 UI 文件结构

```
include/CloudSeamanor/
  PixelArtStyle.hpp          // 像素美术工具 + 调色板
  PixelFontRenderer.hpp      // 像素字体渲染
  PixelUiPanel.hpp           // 像素面板基类
  PixelProgressBar.hpp       // 像素进度条
  PixelDialogueBox.hpp      // 像素对话框
  PixelToolbar.hpp           // 像素工具栏
  PixelInventoryGrid.hpp     // 像素背包
  PixelQuestMenu.hpp         // 像素任务面板
  PixelMinimap.hpp           // 像素迷你地图
  PixelSettingsPanel.hpp     // 设置面板
  PixelTooltip.hpp          // 工具提示组件
  PixelNotification.hpp      // 通知横幅
  PixelCloudSeaForecastPanel.hpp   // 云海预报
  PixelPlayerStatusPanel.hpp       // 玩家状态
  PixelTeaGardenPanel.hpp         // 茶园面板
  PixelWorkshopPanel.hpp           // 工坊面板
  PixelContractPanel.hpp          // 契约面板
  PixelNpcDetailPanel.hpp         // NPC详情
  PixelSpiritBeastPanel.hpp        // 灵兽面板
  PixelFestivalPanel.hpp          // 节日面板
  PixelSpiritRealmPanel.hpp       // 灵界面板
  PixelBuildingPanel.hpp          // 建筑面板
  PixelShopPanel.hpp              // 商店面板
  PixelMailPanel.hpp              // 邮件面板
  PixelAchievementPanel.hpp       // 成就面板
  PixelBeastiaryPanel.hpp         // 灵兽图鉴
  PixelUiConfig.hpp               // 布局常量

src/engine/
  PixelArtStyle.cpp
  PixelFontRenderer.cpp
  PixelUiPanel.cpp
  PixelProgressBar.cpp
  PixelDialogueBox.cpp
  PixelToolbar.cpp
  PixelInventoryGrid.cpp
  PixelQuestMenu.cpp
  PixelMinimap.cpp
  PixelSettingsPanel.cpp
  PixelTooltip.cpp
  PixelNotification.cpp
  PixelCloudSeaForecastPanel.cpp
  PixelPlayerStatusPanel.cpp
  PixelTeaGardenPanel.cpp
  PixelWorkshopPanel.cpp
  PixelContractPanel.cpp
  PixelNpcDetailPanel.cpp
  PixelSpiritBeastPanel.cpp
  PixelFestivalPanel.cpp
  PixelSpiritRealmPanel.cpp
  PixelBuildingPanel.cpp
  PixelShopPanel.cpp
  PixelMailPanel.cpp
  PixelAchievementPanel.cpp
  PixelBeastiaryPanel.cpp
```

---

*文档版本 1.0 | 2026-04-22 | 前端 UI 任务清单，与《云海山庄》后端任务清单配套使用*
