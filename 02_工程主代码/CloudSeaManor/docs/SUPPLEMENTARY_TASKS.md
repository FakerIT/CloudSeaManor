# 《云海山庄》项目不足检查与补充任务清单

> **版本**: 1.0
> **日期**: 2026-04-22
> **项目阶段**: V0.1.0 MVP 原型阶段
> **前置说明**: 本清单基于对项目全部源码（前端 UI engine 层 11 个组件 + 后端 engine/domain/infrastructure 层 58 个源文件 + 测试系统 + 构建系统）的全面扫描，识别出 **146 项具体不足**，归纳为 **78 个可落地执行的补充任务**。

---

## 文档说明

### 不足分类维度

| 维度 | 说明 |
|------|------|
| **Critical** | 阻塞性问题，影响游戏正常运行或数据安全 |
| **High** | 重要缺陷，影响功能正确性或玩家体验 |
| **Medium** | 一般问题，影响代码可维护性 |
| **Low** | 轻微问题，影响代码整洁度 |

### 补充任务结构

每个补充任务包含：
- **不足描述**：具体的问题及其位置
- **任务目的**：修复后应达到的状态
- **技术要求**：修复所需的技术方案
- **交付标准**：验收条件，可直接检查

### 前后端拆分原则

- **前端 UI 补充任务**：修复 UI 组件的交互缺陷、视觉问题、状态管理
- **后端补充任务**：修复 engine/domain/infrastructure 层的逻辑缺陷、架构违规、测试缺失
- **后端任务标注层级**：`app` / `engine` / `domain` / `infrastructure`

---

## 第一部分：前端 UI 补充任务

> 共 **43 个补充任务**，全部由前端 engine 层负责（`src/engine/` 目录下的 `Pixel*` 组件）

---

### UI-101 | 提示框（Tooltip）渲染链路完全缺失

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelInventoryGrid.cpp` 第 121-127 行追踪 `tooltip_dirty_`，第 174-179 行在 hover 时设置脏标记，但 `Render()` 方法（第 192-203 行）从未渲染 `tooltip_vertices_`。玩家在物品上悬停时提示框完全不显示。

**任务目的**：
实现物品悬停提示框的完整渲染链路。

**技术要求**：
1. 在 `PixelInventoryGrid::Render()` 的末尾添加 tooltip 渲染调用
2. tooltip 使用 `PixelUiPanel` 风格的背景（米色 `#FBF4E0` + 1px `#5C3A1E` 边框）
3. 内容格式：`[物品名称]（16px 加粗）\n[品质标签]（12px）\n---\n[描述文字]（12px）\n[价格]（12px 金色）
4. tooltip 跟随鼠标位置（偏移 12px 避免遮挡）
5. 显示延迟 300ms（防止频繁闪烁）

**交付标准**：
- 悬停物品格 300ms 后显示 tooltip
- tooltip 内容包含名称、品质、描述、价格
- tooltip 渲染使用正确的像素风格边框

---

### UI-102 | 空状态界面完全缺失

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelInventoryGrid.cpp` 第 409 行和 `PixelQuestMenu.cpp` 第 195-246 行在背包/任务列表为空时只渲染空白白色格子，无任何提示文本。

**任务目的**：
为空状态提供清晰的视觉反馈。

**技术要求**：
1. 背包空状态：显示 "背包空空如也" 文字（14px `TextBrown`）
2. 任务列表空状态：显示 "暂无任务" 文字
3. 文字居中于面板内容区域
4. 复用 `PixelFontRenderer::DrawText()` 渲染

**交付标准**：
- 背包为空时显示提示文本
- 任务列表为空时显示提示文本
- 提示文本使用像素风格字体

---

### UI-103 | 全局键盘导航系统缺失

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelGameHud.cpp` 第 387-389 行的 `HandleKeyPressed` 方法直接 `(void)key; return false;` 不处理任何按键。玩家无法使用键盘操作背包、任务、工坊等面板。`PixelInventoryGrid.cpp` 的格子选择只能通过鼠标。

**任务目的**：
实现完整的键盘导航系统，支持所有面板的方向键和数字键操作。

**技术要求**：
1. 在 `PixelGameHud::HandleKeyPressed()` 中实现主键盘路由：
   - `I` 键：打开/关闭背包
   - `W/S`：上下移动焦点（所有面板）
   - `A/D`：左右移动（TabBar 场景）
   - `Enter`：确认选择
   - `1-9`：快捷选择背包格子
   - `Tab`：切换标签页
2. 在 `PixelInventoryGrid` 中添加 `keyboard_focused_slot_` 成员
3. 在 `PixelToolbar` 中添加键盘快捷键支持（1-0, -, =）
4. 键盘焦点元素显示绿色 2px 描边（`ColorPalette::ActiveGreen`）
5. 在 `PixelUiConfig.hpp` 中添加键盘导航常量

**交付标准**：
- 背包支持方向键导航和数字键选择
- 工具栏支持数字键快捷选择
- 当前键盘焦点有绿色描边高亮
- Esc 键关闭面板

---

### UI-104 | 对话框选项悬停状态缺失

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelDialogueBox.cpp` 第 108-114 行实现了 `HoverChoice()` 方法存储悬停索引，但从未渲染悬停视觉效果。玩家无法通过鼠标预览要选择的选项。

**任务目的**：
为对话框选项添加鼠标悬停视觉高亮。

**技术要求**：
1. 在 `PixelDialogueBox::Render()` 中，当存在选项时，检测鼠标位置的悬停选项
2. 悬停选项使用 `ColorPalette::HighlightYellow`（#FFFACD）背景高亮
3. 选项高度使用 `DialogueBoxConfig` 中的常量（28px）
4. 悬停时显示右侧箭头 `>` 指示器

**交付标准**：
- 鼠标悬停选项时显示黄色背景高亮
- 悬停选项右侧显示箭头指示器
- 选项之间有正确的间距

---

### UI-105 | 进度条标签文本从不渲染

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelProgressBar.cpp` 第 67-69 行的 `SetLabel()` 方法存储标签文本，但 `Render()` 从不绘制该标签。体力条百分比、金币数量等数值均未显示。

**任务目的**：
实现进度条标签文本渲染，支持百分比和自定义文本。

**技术要求**：
1. 在 `PixelProgressBar::Render()` 中，填充条上方或右侧显示标签文本
2. 标签格式：可配置为百分比（如 `80%`）或自定义文本（如 `280/350`）
3. 字体：12px `ColorPalette::TextBrown`
4. 位置：填充条下方 4px，居中对齐

**交付标准**：
- 体力条显示百分比（如 `80%`）
- 灵气条显示数值（如 `兴盛`）
- 标签文本正确渲染在填充条附近

---

### UI-106 | 设置面板修改不持久化

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`PixelSettingsPanel.cpp` 第 26-42 行的 `AdjustValue()` 方法修改了 `bgm_volume_`、`sfx_volume_`、`fullscreen_` 等值，但关闭面板时没有任何保存逻辑。修改被丢弃。

**任务目的**：
实现设置面板的确认/取消/应用机制。

**技术要求**：
1. 添加"确认"和"取消"按钮到设置面板
2. 确认：调用 `AudioManager::SetVolume()` 和 `GameApp::ToggleFullscreen()`，并将设置写入 `configs/audio.json`
3. 取消：放弃所有修改，恢复原始值
4. 或实现自动保存（修改后立即应用）

**交付标准**：
- 音量滑块拖动后数值实时更新
- 关闭面板后设置正确保存
- 全屏切换生效

---

### UI-107 | 窗口大小改变（Resize）未处理

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
所有 UI 组件均未处理 `sf::Event::Resized` 事件。所有坐标基于 `1280×720` 硬编码，分辨率改变时 UI 布局完全崩溃。

**任务目的**：
实现分辨率自适应缩放系统。

**技术要求**：
1. 在 `GameApp::ProcessEvents()` 中添加 resize 事件处理
2. 计算缩放系数：`scale = min(window_width / 1280.0f, window_height / 720.0f)`
3. 所有 UI 组件应用该缩放系数
4. `PixelUiConfig` 中的坐标乘以缩放系数
5. 使用 `sf::View` 的缩放而非每个组件单独缩放

**交付标准**：
- 分辨率改变后 UI 布局正确缩放
- 像素风格保持锐利（整数缩放）
- 无元素超出屏幕边界

---

### UI-108 | 像素直线算法实现错误

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelArtStyle.cpp` 第 199-204 行的 `DrawPixelLine()` 方法计算方向向量时，分子分母均为差值，导致方向向量始终为 `+1.0` 或 `-1.0`，而非归一化单位向量。

```cpp
// 错误：dir.x = (end.x - start.x) / dx，dx = |end.x - start.x|
// 结果：dir.x = sign(end.x - start.x) = ±1，而非归一化
```

**任务目的**：
修复直线绘制算法，确保正确归一化。

**技术要求**：
1. 计算实际距离：`float dist = std::sqrt(dx*dx + dy*dy)`
2. 如果 `dist < 1e-5f`，绘制单点返回
3. 否则：`dir.x = (end.x - start.x) / dist`

**交付标准**：
- `DrawPixelLine()` 绘制倾斜直线时方向正确
- 直线端点正确（首尾各一个像素）
- 单像素线段正确渲染

---

### UI-109 | 金币显示使用 Emoji

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelGameHud.cpp` 第 701 行使用硬编码的 emoji "💰" 字符，与像素游戏风格完全不协调。

**任务目的**：
将 emoji 替换为像素风格的金币图标。

**技术要求**：
1. 创建金币图标占位符（使用 `sf::Color(255, 215, 0)` 绘制圆形像素图标）
2. 在 `PixelArtStyle` 中添加 `DrawCoinIcon()` 方法
3. 或在 `assets/sprites/ui/ui_main.png` 中添加金币图标精灵帧
4. 更新 `DrawCoinIcon()` 使用精灵帧而非 emoji

**交付标准**：
- 金币显示使用像素风格图标
- 图标颜色为 `ColorPalette::CoinGold`（#FFD700）
- 无 emoji 字符出现在像素风格 UI 中

---

### UI-110 | NPC 头像无边界裁剪

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelDialogueBox.cpp` 第 172-178 行渲染 NPC 头像时不做任何边界裁剪，大尺寸纹理会溢出对话框。

**任务目的**：
为 NPC 头像添加正确的尺寸限制和裁剪。

**技术要求**：
1. 头像尺寸限制为 `DialogueBoxConfig::AvatarSize`（48×48px）
2. 如果纹理大于限制，使用 `sprite.setScale()` 等比缩放
3. 如果等比缩放后仍超出，使用 `sf::RenderStates` 的 `sf::BlendMode` 和裁剪矩形

**交付标准**：
- NPC 头像最大显示尺寸为 48×48px
- 大纹理自动缩放适应
- 头像不会溢出对话框边界

---

### UI-111 | Tab 标签栏无悬停状态

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelInventoryGrid.cpp` 第 304-313 行的 Tab 栏只区分 `active` 状态，鼠标悬停时无任何视觉反馈。

**任务目的**：
为 Tab 标签栏添加悬停状态。

**技术要求**：
1. 在 `PixelInventoryGrid::MouseHover()` 中检测悬停的 Tab
2. 悬停 Tab 使用 `ColorPalette::HighlightYellow` 背景
3. 悬停时显示下划线（颜色 `ColorPalette::LightBrown`）

**交付标准**：
- Tab 标签栏悬停时有浅黄色背景
- Tab 标签栏悬停时有下划线指示
- Tab 标签栏 active 状态保持绿色下划线

---

### UI-112 | 已选物品无视觉反馈

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelInventoryGrid.cpp` 第 446-477 行只渲染选中格子的边框，无物品详情面板显示选中物品的完整信息。

**任务目的**：
为选中物品显示详情面板。

**技术要求**：
1. 当 `selected_slot_ >= 0` 时，在网格右侧显示详情面板
2. 详情面板宽 200px（`InventoryGridConfig::InfoPanelWidth`）
3. 内容：名称（16px 加粗）/ 品质（彩色）/ 描述（14px）/ 价格（金色）/ 操作按钮
4. 详情面板使用像素风格边框

**交付标准**：
- 选中物品格后右侧显示详情面板
- 详情面板包含名称、品质、描述、价格
- 详情面板像素风格完整

---

### UI-113 | 任务菜单悬停检测忽略 X 坐标

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelQuestMenu.cpp` 第 93-100 行的 `MouseHover()` 方法中 `(void)mx;` 显式忽略鼠标 X 坐标，导致水平移动鼠标也会触发行悬停变化。

**任务目的**：
修复任务菜单的悬停检测。

**技术要求**：
1. 移除 `(void)mx;` 注释
2. 添加 X 坐标范围检测：`if (mx >= r.position.x && mx <= r.position.x + r.size.x)`
3. 只有鼠标同时处于正确 Y 行且在行的 X 范围内才触发悬停

**交付标准**：
- 任务菜单悬停需要鼠标在正确行的 X 范围内
- 水平移动不触发行悬停变化

---

### UI-114 | 面板可见性检查不一致

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelUiPanel::draw()`（第 190-193 行）和 `PixelUiPanel::Render()`（第 108-127 行）都检查 `visible_` 和 `alpha_`，但 `PixelGameHud::Render()` 依赖 `panel_state_` 而非组件自身的可见性状态。

**任务目的**：
统一面板可见性检查逻辑。

**技术要求**：
1. `PixelGameHud::Render()` 中的面板渲染直接调用组件的 `Render()`，由组件内部处理可见性检查
2. 删除 `PixelGameHud::Render()` 中 `panel_state_.xxx_open` 的前置条件检查

**交付标准**：
- 面板的可见性由自身管理
- `GameHud::Render()` 只负责调用子组件的 Render 方法

---

### UI-115 | 魔法数字遍布代码

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
多个 UI 组件中存在大量硬编码的魔法数字：
- `PixelDialogueBox.cpp` 第 121 行：`40.0f`（底边偏移）
- `PixelDialogueBox.cpp` 第 121 行：`28`（选项高度）
- `PixelMinimap.cpp` 第 150 行：`8.0f`（角块尺寸）
- `PixelProgressBar.cpp` 第 137-145 行：`1.0f`（边框厚度）
- `PixelSettingsPanel.cpp` 第 56 行：`16.0f`, `12.0f`, `28.0f`, `22.0f`
- `PixelQuestMenu.cpp` 第 71 行：`6.3f`, `7.3f`（行高倍数）

**任务目的**：
提取所有魔法数字到 `PixelUiConfig.hpp` 或对应的 Config 结构体中。

**技术要求**：
1. 在 `PixelUiConfig.hpp` 中为每个组件添加 Config 结构体
2. 所有硬编码数值迁移到对应的 Config 常量
3. 更新所有调用点使用 Config 常量

**交付标准**：
- 所有 UI 组件无硬编码魔法数字
- 所有数值常量在 Config 结构体中定义
- 代码审查无遗漏

---

### UI-116 | 社交页 NPC 卡片无悬停反馈

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelInventoryGrid.cpp` 第 149-162 行社交页卡片的 `MouseClick()` 只在点击时更新选择索引，悬停时无任何视觉变化。

**任务目的**：
为社交页 NPC 卡片添加悬停高亮。

**技术要求**：
1. 在 `RebuildGeometry_()` 中为社交卡片添加悬停状态渲染
2. 悬停时卡片背景变为 `ColorPalette::HighlightYellow`
3. 悬停状态由 `MouseHover()` 更新（已有逻辑，需在渲染中使用）

**交付标准**：
- 社交页 NPC 卡片悬停时有浅黄色背景
- 悬停状态由 `MouseHover()` 正确更新

---

### UI-117 | 无加载状态显示

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
所有面板在过渡（FadeIn/FadeOut）期间没有加载指示器，文本/内容在淡入完成后立即出现，无渐变感觉。

**任务目的**：
为面板过渡添加内容渐入效果。

**技术要求**：
1. 在 `PixelUiPanel::Render()` 中，内容 alpha 与面板 alpha 分离
2. 淡入期间：面板边框先出现，内容在 0.15s 后开始渐入
3. 淡出期间：内容先消失，面板边框在 0.10s 后跟随
4. 在内容顶点数组渲染前应用 alpha 系数

**交付标准**：
- 面板边框和内容有区分的淡入淡出时序
- 视觉效果更流畅有层次感

---

### UI-118 | 进度条低值闪烁性能浪费

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelProgressBar.cpp` 第 74-83 行每次闪烁状态切换都触发完整的 `RebuildGeometry_()`，重建所有顶点数据。大量 UI 面板闪烁时会浪费 CPU。

**任务目的**：
优化进度条低值闪烁，不重建几何数据。

**技术要求**：
1. 缓存填充色顶点的两套颜色（亮色和暗色）
2. 闪烁时只交换颜色数组引用，不重建几何
3. 使用 `std::swap()` 在两个预构建的顶点数组之间切换

**交付标准**：
- 进度条低值闪烁不触发 `RebuildGeometry_()`
- 闪烁性能开销降低 50% 以上

---

### UI-119 | 可交互对象无禁用状态渲染

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`PixelToolbar.cpp` 和 `PixelInventoryGrid.cpp` 对空格子或禁用物品没有明显的"禁用"视觉状态，只是灰色可能与空格子混淆。

**任务目的**：
为禁用/空状态添加明确的视觉区分。

**技术要求**：
1. 空格子：白色背景 `#FEFEF8` + 灰色虚线边框
2. 禁用格子：深灰色 `#A9A9A9` 背景 + `×` 图标
3. 在 `PixelUiConfig` 中定义 `EmptySlotStyle` 和 `DisabledSlotStyle`

**交付标准**：
- 空格子有明确的虚线边框
- 禁用格子有斜线图案或 `×` 图标
- 两种状态视觉上可明显区分

---

### UI-120 | 标签栏悬停效果缺失（通用 TabBar 组件）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelInventoryGrid` 中的 Tab 栏只区分 active 状态，没有独立的 hover 状态。

**任务目的**：
为标签栏添加 hover + active 的三层状态区分。

**技术要求**：
1. Normal：背景 `ColorPalette::LightCream`
2. Hover：背景 `ColorPalette::HighlightYellow`
3. Active：背景 `ColorPalette::LightCream` + 底部 3px `ColorPalette::ActiveGreen` 下划线

**交付标准**：
- 标签栏有 Normal / Hover / Active 三层状态
- 三种状态视觉上可区分

---

### UI-121 | 闪烁定时器使用绝对重置

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelGameHud.cpp` 第 292-299 行闪烁定时器在 `>= interval * 2` 时重置为 0，而非使用取模。这在大帧跳跃（如卡顿恢复）时可能导致闪烁状态跳过。

**任务目的**：
使用取模运算确保闪烁周期稳定。

**技术要求**：
```cpp
blink_timer_ = std::fmod(blink_timer_ + delta_seconds, AnimationConfig::BlinkInterval * 2.0f);
```

**交付标准**：
- 无论帧率或帧时间如何变化，闪烁周期保持稳定

---

### UI-122 | 字体缺失时无占位符

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelFontRenderer.cpp` 的 fallback 字体选择逻辑存在，但如果两者都缺少字形，则什么都不渲染。CJK 字符缺失时用户看不到任何文本。

**任务目的**：
为缺失字形添加 Unicode 占位符渲染。

**技术要求**：
1. 当字体缺少字形时，使用 `[?]` 占位符渲染
2. 占位符使用 `ColorPalette::DarkGray`
3. 占位符尺寸与缺失字形的预期宽度相同

**交付标准**：
- 缺失字形显示 `[?]` 占位符
- 占位符与周围文字宽度一致

---

### UI-123 | 设置面板 slot 未清除

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelSettingsPanel.cpp` 第 44-49 行 `SetSlots()` 只填充前 `min(slots.size(), 3)` 个槽位，如果新传入的槽位数量少于 3，则多余的槽位不会被清除。

**任务目的**：
确保 `SetSlots()` 正确清除所有槽位。

**技术要求**：
```cpp
for (std::size_t i = 0; i < 3; ++i) {
    if (i < slots.size()) {
        slots_[i] = slots[i];
    } else {
        slots_[i] = std::nullopt;  // 清除多余槽位
    }
}
```

**交付标准**：
- 槽位数量减少时多余槽位正确清除
- 不会出现残留的旧存档信息

---

### UI-124 | 存档覆盖无确认对话框

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`PixelSettingsPanel` 中选择已有存档的槽位并按 Enter 会直接覆盖存档，无二次确认对话框。

**任务目的**：
添加存档覆盖确认机制。

**技术要求**：
1. 选择已有存档槽位时，显示确认对话框
2. 对话框内容："覆盖存档？\n第 X 年 · 第 Y 天"
3. 选项：[取消] [覆盖]
4. 覆盖确认后才执行保存操作

**交付标准**：
- 覆盖已有存档时弹出确认对话框
- 用户可取消或确认覆盖

---

### UI-125 | 小地图玩家点可溢出边界

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelMinimap.cpp` 第 181-189 行在渲染玩家位置点时，直接计算 `pos - dot_r` 可能在地图坐标接近边界时超出小地图区域。

**任务目的**：
添加边界裁剪，确保玩家点始终在小地图内。

**技术要求**：
1. 在设置玩家点位置前，添加边界检查：
```cpp
float clamped_x = std::clamp(player_map_pos.x, cs + dot_r, minimap_size.x - cs - dot_r);
float clamped_y = std::clamp(player_map_pos.y, cs + dot_r, minimap_size.y - cs - dot_r);
```
2. 使用裁剪后的坐标渲染玩家点

**交付标准**：
- 玩家点始终在小地图边界内
- 边界裁剪不影响点的大小（保持 `dot_r` 半径）

---

### UI-126 | 对话框跳过提示无视觉差异

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelDialogueBox` 在打字状态和等待确认状态都显示同一个 `▼` 闪烁指示器。玩家不清楚 Enter 键在当前状态下的具体行为。

**任务目的**：
区分打字状态和等待确认状态的提示。

**技术要求**：
1. 打字状态：右下角 `▼`（小三角，表示"继续"）
2. 等待确认：右下角 `▶` 或 `[Enter]` 文本（表示"确认"）
3. 颜色：打字状态使用 `ColorPalette::TextBrown`，等待确认使用 `ColorPalette::ActiveGreen`

**交付标准**：
- 打字状态和等待确认状态有不同的视觉提示
- 玩家能区分 Enter 键的两种行为

---

### UI-127 | NPC 社交详情面板缺失

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
点击社交页 NPC 卡片后选中高亮了，但右侧没有显示 NPC 的详细信息面板（称号、生日、喜好等）。

**任务目的**：
实现 NPC 详情面板。

**技术要求**：
1. 当 `selected_social_index_ >= 0` 且社交页激活时，在卡片右侧显示 NPC 详情面板
2. 详情内容：名字（16px）/ 称号 / 生日 / 喜好物品 / 厌恶物品
3. 好感条和心事件进度条
4. 复用 `PixelProgressBar::SetFavorStyle()`

**交付标准**：
- 选中 NPC 卡片后右侧显示详情面板
- 详情包含完整社交信息

---

### UI-128 | 通知横幅堆叠管理缺失

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelNotificationBanner` 未实现多通知堆叠管理。如果多个通知同时触发，会相互覆盖或丢失。

**任务目的**：
实现通知队列和堆叠管理。

**技术要求**：
1. 维护通知队列 `std::deque<NotificationItem>`
2. 同时最多显示 3 条通知
3. 新通知从上方加入，已有通知依次下移
4. 通知间隔 4px 垂直排列

**交付标准**：
- 多通知同时触发时正确堆叠显示
- 通知数量超过 3 条时队列管理正确

---

### UI-129 | UI 面板无法拖动

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
所有 `PixelUiPanel` 面板都固定在配置的位置，无法通过拖动标题栏来调整位置。

**任务目的**：
为可拖动面板添加拖动功能。

**技术要求**：
1. 在 `PixelUiPanel` 中添加拖动状态成员：`is_dragging_`, `drag_offset_`
2. 鼠标在标题栏按下时开始拖动
3. 拖动时更新面板位置
4. 仅标题栏区域可拖动

**交付标准**：
- 鼠标在标题栏拖动时面板跟随移动
- 拖动范围限制在屏幕内

---

### UI-130 | 背包操作按钮未实现

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`PixelInventoryGrid` 第 579-584 行有操作按钮的占位定义，但点击操作按钮的回调从未实现。

**任务目的**：
实现背包物品的操作按钮回调。

**技术要求**：
1. [使用] 按钮：调用 `inventory.TryUseItem(idx)`
2. [出售] 按钮：调用收购商出售逻辑，显示获得金币
3. [送礼] 按钮：切换到对应 NPC 的送礼模式
4. 背包满/金币不足时禁用对应按钮

**交付标准**：
- 三个操作按钮均可用
- 按钮点击有音效反馈

---

### UI-131 | 任务面板展开/折叠功能缺失

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`PixelQuestMenu.cpp` 中任务条目有 `expanded` 字段，但展开内容从未渲染。展开的任务没有显示详情和奖励信息。

**任务目的**：
实现任务条目展开/折叠详情显示。

**技术要求**：
1. 展开的任务在行下方显示额外内容区域（高度 48-64px）
2. 展开内容：任务描述 / 奖励预览 / 进度要求
3. 展开区域使用浅黄色背景区分
4. 折叠区域使用展开三角箭头指示（`▶` / `▼`）

**交付标准**：
- 点击任务可展开/折叠
- 展开内容正确显示描述和奖励

---

### UI-132 | 右键上下文菜单缺失

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
像素游戏通常支持右键快速操作物品，但当前背包和工具栏没有右键菜单。

**任务目的**：
实现右键上下文菜单。

**技术要求**：
1. 在 `PixelInventoryGrid` 中添加 `context_menu_vertices_` 和 `ContextMenuState`
2. 右键点击物品格时弹出菜单
3. 菜单内容（根据物品类型）：使用 / 丢弃 / 送礼 / 详细信息
4. 菜单外点击或 Esc 关闭

**交付标准**：
- 右键点击物品格弹出上下文菜单
- 菜单内容根据物品类型变化

---

### UI-133 | 存档缩略图生成缺失

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
存档时没有生成游戏画面快照作为缩略图，读档界面显示"无预览"。

**任务目的**：
实现存档缩略图生成。

**技术要求**：
1. 使用 `sf::Texture::update(window)` 截取存档时刻的游戏画面
2. 缩略图尺寸：160×90px（`InventoryGridConfig::SocialCardHeight` 相关）
3. 保存为 `saves/slot_{n}/thumbnail.png`
4. 无缩略图时显示占位图

**交付标准**：
- 存档时生成缩略图文件
- 读档界面正确显示缩略图
- 无缩略图时显示占位图

---

### UI-134 | 像素缩放模式未强制

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
SFML 默认开启抗锯齿，导致像素游戏的字体和图形出现模糊。

**任务目的**：
强制所有 SFML 渲染对象使用整数缩放和无抗锯齿模式。

**技术要求**：
1. 创建 `sf::ContextSettings` 时设置 `antialiasingLevel = 0`
2. 所有 `sf::Text` 调用 `setSmooth(false)`
3. 所有精灵渲染使用整数坐标（`std::round()`）
4. 所有缩放使用整数倍

**交付标准**：
- 所有 UI 渲染无抗锯齿模糊
- 像素风格保持锐利

---

### UI-135 | 世界提示浮动动画不自然

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
世界提示（目标头顶高亮）的上下浮动动画（第 118 行注释提到）当前未实现，提示静止在对象头顶。

**任务目的**：
实现提示的浮动动画。

**技术要求**：
1. 提示 Y 坐标使用 `sin(pulse_timer_ * 3.14159f)` 进行正弦波动
2. 振幅 2px，周期 0.5s
3. 动画在 `TargetHintRuntime` 中管理

**交付标准**：
- 世界提示有轻微上下浮动动画
- 动画周期 0.5s，振幅 2px

---

### UI-136 | UI 组件测试覆盖缺失

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
engine 层的 `Pixel*` UI 组件完全没有单元测试，无法验证渲染逻辑正确性。

**任务目的**：
为 UI 组件添加基础渲染单元测试。

**技术要求**：
1. 在 `tests/engine/` 下创建 `PixelUiComponentTest.cpp`
2. 测试场景：
   - `PixelProgressBar`：不同百分比值的渲染顶点数量正确
   - `PixelToolbar`：12 格布局坐标正确
   - `PixelUiPanel`：淡入淡出动画 alpha 变化正确
   - `PixelArtStyle`：颜色运算正确（Darken/Lighten）
3. 使用 Mock SFML RenderTarget 避免实际渲染

**交付标准**：
- UI 组件至少有 20 个测试用例
- 渲染逻辑有单元测试覆盖

---

### UI-137 | 面板动画时长不可配置

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
所有面板的淡入淡出时长统一为 `AnimationConfig` 中的固定值，无法为不同面板设置不同动画速度。

**任务目的**：
支持面板级别的动画时长配置。

**技术要求**：
1. 在 `PixelUiPanel` 中添加 `animation_duration_` 成员
2. 构造时从 `PixelUiConfig` 读取默认值
3. 提供 `SetAnimationDuration()` 方法允许面板覆盖默认值
4. 继承类可以在构造时设置不同的时长

**交付标准**：
- 对话框可设置更快的动画时长
- 面板动画时长可在构造时配置

---

### UI-138 | 对话框选项键盘导航缺失

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
对话框有选项时（WaitingChoice 状态），玩家只能使用鼠标选择，无法使用键盘上下箭头导航。

**任务目的**：
实现对话框选项的键盘导航。

**技术要求**：
1. 在 `PixelDialogueBox::HandleKeyPressed()` 中处理 Up/Down 箭头
2. Up 箭头：`hovered_choice_ = (hovered_choice_ - 1 + choices_.size()) % choices_.size()`
3. Down 箭头：`hovered_choice_ = (hovered_choice_ + 1) % choices_.size()`
4. Enter 确认当前悬停选项

**交付标准**：
- 对话框选项支持键盘上下箭头导航
- Enter 键确认当前悬停选项

---

### UI-139 | 无障碍触控热区不达标

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
多个交互元素尺寸小于 WCAG 2.1 AAA 的最小触控热区标准（44×44px）。工具栏格子 48×48px 勉强达标，但任务行高度仅 32px。

**任务目的**：
确保所有交互元素触控热区 ≥ 44×44px。

**技术要求**：
1. 任务面板行高从 32px 提升至 36px（触控热区达标）
2. Tab 标签栏高度从 36px 提升至 40px
3. 选项条目最小高度 36px
4. 在 `PixelUiConfig.hpp` 中添加 `MinTouchTargetSize = 44.0f` 常量

**交付标准**：
- 所有交互元素热区 ≥ 44×44px
- 常量已在 `PixelUiConfig` 中定义

---

## 第二部分：后端补充任务

> 共 **35 个补充任务**，按四层架构分类

---

### BE-101 | Domain 层 SFML 依赖违规（Critical）

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`src/domain/PickupDrop.hpp`（第 24-25 行）和 `src/domain/Interactable.hpp`（第 24-25 行）包含 `#include <SFML/Graphics/Rect.hpp>` 和 `#include <SFML/Graphics/RectangleShape.hpp>`，`sf::RectangleShape` 成员变量存在于 domain 层。

**任务目的**：
将 SFML 渲染对象从 domain 层完全移除，使 domain 层可无 GUI 单元测试。

**技术要求**：
1. `PickupDrop`：将 `sf::RectangleShape shape_` 替换为纯数据 `sf::FloatRect bounds_`
2. `Interactable`：同上，将渲染对象移到 engine 层
3. 在 `engine/rendering/` 下创建 `PickupVisualComponent.hpp` 和 `InteractableVisualComponent.hpp`
4. `FarmingRuntime.cpp` 持有 VisualComponent，负责同步 domain 数据和渲染对象
5. domain 层头文件禁止包含 `SFML/Graphics.hpp`

**交付标准**：
- `domain/` 目录下所有文件不含 `#include <SFML/Graphics.hpp>`
- domain 层代码可在无 GUI 环境下编译和测试

---

### BE-102 | MainPlotSystem 命名空间违规（Critical）

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`src/domain/MainPlotSystem.cpp` 第 10 行使用 `namespace CloudSeamanor::engine`，但文件位于 `src/domain/` 目录。应使用 `namespace CloudSeamanor::domain`。

**任务目的**：
修正命名空间违规。

**技术要求**：
1. 将 `namespace CloudSeamanor::engine` 改为 `namespace CloudSeamanor::domain`
2. 更新对应的头文件命名空间
3. 更新所有调用点（`GameRuntime.cpp` 中的 include 和命名空间）
4. 编译验证无命名空间冲突

**交付标准**：
- `MainPlotSystem` 位于 `CloudSeamanor::domain` 命名空间
- 所有引用点编译通过

---

### BE-103 | 茶园地块数据未存档（Critical）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/GameRuntime.cpp` 第 173-177 行创建的 `tea_garden_plots_` 从未出现在 `SaveGameState` 或 `LoadGameState` 函数中。玩家每次读取存档后茶园状态丢失。

**任务目的**：
将茶园地块完整纳入存档系统。

**技术要求**：
1. 在 `SaveGameState()` 中添加茶园地块数据序列化：
   ```cpp
   if (save_key == "tea_garden_plots") {
       lines.push_back("teagarden|" + std::to_string(plots.size()));
       for (const auto& p : plots) {
           lines.push_back("tp|" + p.crop_name + "|" + ...);
       }
   }
   ```
2. 在 `LoadGameState()` 中添加对应的反序列化
3. 在存档版本号 v4 中添加茶园数据字段
4. 添加存档兼容性：旧存档茶园默认为空

**交付标准**：
- 存档包含完整的 tea_garden_plots 数据
- 读取存档后茶园地块状态完全恢复
- 旧存档向前兼容

---

### BE-104 | 云海日期推进双重调用导致状态损坏（Critical）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`GameRuntime.cpp` 第 629-635 行的 `SleepToNextMorning()` 调用 `cloud_system.AdvanceToNextDay()`，而 `OnDayChanged()` 在 `Update()` 中也调用了相同的逻辑。云海推进在每次睡眠时发生两次，导致灵气积累、天气状态计算错误。

**任务目的**：
消除云海推进的双重调用。

**技术要求**：
1. `SleepToNextMorning()` 不直接调用 `cloud.AdvanceToNextDay()`
2. 睡眠后次日的云海推进由 `OnDayChanged()` 自然触发
3. 确保 `AdvanceToNextDay()` 只在每日第一次 `Update()` 时触发一次

**交付标准**：
- 云海推进在每次日变更时仅触发一次
- 灵气积累数值正确

---

### BE-105 | NpcDialogueManager 候选列表空时越界访问（Critical）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/NpcDialogueManager.cpp` 第 804-805 行，当候选列表中所有候选项评分都为 0 时，`best_indices` 可能为空，但随后构造 `uniform_int_distribution(0, -1)` 触发未定义行为。

**任务目的**：
修复随机选择的空列表保护。

**技术要求**：
```cpp
if (best_indices.empty()) {
    return candidates[dist(rng_)];  // 退化为均匀分布
}
// 正常：加权随机
std::uniform_int_distribution<std::size_t> dist(0, best_indices.size() - 1);
return candidates[best_indices[dist(rng_)]];
```

**交付标准**：
- 候选列表为空时不会崩溃
- 所有评分相同时使用均匀随机选择

---

### BE-106 | 茶园永久品质 BUFF（Critical）

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`GameRuntime.cpp` 第 1163-1165 行，在大潮期间每帧都将茶园地块的 `quality` 设置为 `Spirit`，但 `OnDayChanged()` 中茶园地块不参与每日重置，导致大潮结束后品质仍为 `Spirit`。

**任务目的**：
茶园品质仅在收获时计算一次，不做永久 BUFF。

**技术要求**：
1. 移除第 1163-1165 行的永久品质赋值
2. 改为在收获时（`HarvestTeaGarden()`）计算并应用大潮品质加成
3. 品质仅影响本次收获，不修改 `plot.quality` 字段

**交付标准**：
- 大潮结束后茶园品质恢复正常结算
- 大潮期间收获的茶园作物品质为 Spirit

---

### BE-107 | ResourceManager::GetFont 返回错误字体（Critical）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/infrastructure/ResourceManager.cpp` 第 159-162 行，当字体加载失败时，在 `if (it == fonts_.end())` 分支内创建了 `default_font_`，但随后返回的是未初始化的 `static sf::Font dummy` 而非 `*default_font_`。

**任务目的**：
修复字体加载失败时的返回逻辑。

**技术要求**：
```cpp
if (it == fonts_.end()) {
    if (!default_font_) {
        default_font_ = CreateDefaultFont();
    }
    return *default_font_;  // 返回创建的默认字体
}
```

**交付标准**：
- 字体加载失败时返回默认字体而非空字体
- 默认字体在所有平台上有字形显示

---

### BE-108 | BattleField 语法错误（Critical）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/BattleField.cpp` 第 264 行存在 `== false` 的多余比较：
```cpp
if (!partners_.back().cooldown_remaining.empty() == false)
```

**任务目的**：
清理多余比较，提高代码可读性。

**技术要求**：
```cpp
if (partners_.back().cooldown_remaining.empty()) {
    // cooldown 为空，不需要等待
```

**交付标准**：
- 代码可读性改善
- 逻辑语义不变

---

### BE-109 | CloudSystem 未设置随机数种子（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/domain/CloudSystem.cpp` 第 189, 195, 201 行多次调用 `std::rand()` 而未先调用 `std::srand()`。Windows/MSVC 上 C runtime 自动以启动时间为种子，但 Linux/GCC 可能以 1 为种子，导致天气在某些环境下完全可预测。

**任务目的**：
添加随机数种子设置。

**技术要求**：
1. 在 `CloudSystem::Initialize()` 中调用 `std::srand(std::random_device{}())`
2. 或使用 `<random>` 的 `std::mt19937` 和 `std::uniform_real_distribution<float>` 替代 `std::rand()`

**交付标准**：
- 天气随机性不依赖于平台
- 多次启动的天气序列不同

---

### BE-110 | MainPlotSystem NPC 列表重复 4 次（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/domain/MainPlotSystem.cpp` 中 NPC ID 数组 `{"acha", "xiaoman", "wanxing", "lin", ...}` 重复出现 4 次（第 1077, 1127, 1162, 1284 行），新增 NPC 需要在所有 4 处添加，极易遗漏。

**任务目的**：
消除 NPC 列表重复，改为常量或配置。

**技术要求**：
1. 在 `MainPlotSystem.hpp` 中定义静态常量数组：
   ```cpp
   static constexpr const char* kNpcIds[] = {"acha", "xiaoman", "wanxing", "lin", ...};
   ```
2. 所有使用处引用 `kNpcIds`
3. 未来可通过配置文件动态生成 NPC 列表

**交付标准**：
- NPC 列表只在一处定义
- 新增 NPC 只需修改一处

---

### BE-111 | MainPlotSystem LoadState 不清空旧状态（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/domain/MainPlotSystem.cpp` 第 1449-1456 行，`LoadState()` 直接向 `completed_chapters_` 和 `completed_plots_` 插入数据，不先清空。如果读取已损坏的存档文件，旧状态会与新状态混合。

**任务目的**：
在加载前清空所有状态。

**技术要求**：
```cpp
void MainPlotSystem::LoadState(const std::vector<std::string>& all_lines) {
    current_chapter_ = 0;
    current_plot_ = 0;
    completed_chapters_.clear();  // 清空！
    completed_plots_.clear();    // 清空！
    current_route_ = PlotRoute::None;
    // 然后解析 all_lines...
}
```

**交付标准**：
- 存档读取前清空所有剧情进度状态
- 多次读取同一存档结果一致

---

### BE-112 | QuestManager 自动接取逻辑竞态（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/GameRuntime.cpp` 第 393-399 行，每日 6:00 触发 `OnDayChanged()` 时，第一个 `NotTaken` 任务被自动转为 `InProgress`，玩家无法选择是否接取。自动任务接取与任务"手动接取"的设计存在矛盾。

**任务目的**：
统一任务接取逻辑。

**技术要求**：
1. 如果任务是委托类（NPC 委托），自动转为 `InProgress`
2. 如果是契约类（云海之约），保持 `NotTaken`，等待玩家主动接取
3. 在 `RuntimeQuest` 结构中添加 `quest.auto_accept` 字段

**交付标准**：
- 委托类任务自动接取
- 契约类任务需玩家主动接取

---

### BE-113 | FarmingSystem 忽略 AddItem/RemoveItem 失败（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/FarmingSystem.cpp` 第 88 行调用 `RemoveItem()` 忽略返回值；第 130 行调用 `AddItem()` 忽略返回值。而 `InteractionSystem.cpp` 正确使用了 `TryAddItem()` 和 `TryRemoveItem()`。

**任务目的**：
统一使用 Result 返回值的库存操作。

**技术要求**：
1. 将 `RemoveItem()` 替换为 `TryRemoveItem()`
2. 将 `AddItem()` 替换为 `TryAddItem()`
3. 失败时记录警告日志并拒绝继续操作

**交付标准**：
- 库存操作失败时正确处理
- 背包满时收获/种植被正确拒绝

---

### BE-114 | 作物收获阈值硬编码（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/FarmingSystem.cpp` 第 177 行硬编码 `if (plot.growth >= 80.0f)` 作为收获条件，但不同作物的 `growth_time` 不同（萝卜 4 天 vs 茶树 160 秒）。`growth_time` 存在 `CropData` 中，阈值应按作物动态计算。

**任务目的**：
使用动态收获阈值。

**技术要求**：
```cpp
if (plot.growth >= plot.growth_time * CropGrowthSystem::kHarvestThreshold) {
    // 收获
}
```
其中 `kHarvestThreshold = 1.0f`（作物数据中可配置）

**交付标准**：
- 所有作物在生长到 `growth_time` 后可收获
- 短周期和长周期作物收获阈值正确

---

### BE-115 | CloudGuardianContract LoadState 是空操作（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/domain/CloudGuardianContract.cpp` 第 347-349 行，`LoadState()` 包含 TODO 注释后直接调用 `Initialize()`，忽略所有存档数据。契约完成进度每次读档后丢失。

**任务目的**：
实现云海契约的完整存档。

**技术要求**：
1. 实现与 `SaveState()` 对应的 `LoadState()` 解析逻辑
2. 解析 `contract_progress` 字段恢复各卷完成状态
3. 解析 `spirit_energy` 和 `current_phase` 恢复灵气状态

**交付标准**：
- 契约进度正确存档和读取
- 读档后契约状态与存档时一致

---

### BE-116 | InputManager::LoadFromFile 是空实现（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/InputManager.cpp` 第 178-180 行的 `LoadFromFile()` 永远返回 `false`，按键绑定修改无法持久化。

**任务目的**：
实现按键绑定的持久化。

**技术要求**：
1. 从 `configs/keybindings.json` 读取按键映射
2. JSON 格式：`{ "MoveUp": "W", "Interact": "E" }`
3. 加载失败时使用默认按键映射并记录警告

**交付标准**：
- 按键绑定可从配置文件加载
- 修改的按键绑定可持久化到配置文件

---

### BE-117 | DialogEngine 和 NpcDialogueManager 代码重复（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/DialogueEngine.cpp`（第 20-172 行）和 `src/engine/NpcDialogueManager.cpp`（第 143-285 行）有几乎相同的 JSON 对话解析逻辑。bug 修复或功能增强需要同步修改两处。

**任务目的**：
将 JSON 解析逻辑合并到 infrastructure 层。

**技术要求**：
1. 在 `infrastructure/` 下创建 `DialogueJsonParser.cpp`
2. 提取共享的解析逻辑：`ParseDialogueNodes()`、`ParseChoices()`
3. `DialogueEngine` 和 `NpcDialogueManager` 共用同一个解析器
4. 消除代码重复

**交付标准**：
- JSON 解析逻辑只在一处定义
- 两处对话引擎使用相同的解析代码

---

### BE-118 | NpcDialogueManager 候选 ID 提取脆弱（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/NpcDialogueManager.cpp` 第 215 行通过取 JSON 对象的第 0 个字符串字段作为 choice ID，但 JSON 对象字段顺序不确定，可能取到错误的字段。

**任务目的**：
使用显式 key 查找。

**技术要求**：
```cpp
ch.id = extract_string(choice_obj, find_key(choice_obj, "id"));
```

**交付标准**：
- choice ID 通过显式 `"id"` key 查找
- 不依赖 JSON 字段顺序

---

### BE-119 | 双层 State 类导致状态重复（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`game_state::RuntimeState`（`game_state/RuntimeState.hpp` 第 35-56 行）与 `engine::GameWorldState` 存在大量字段重复（`interaction_state`、`tea_plots` 等）。两套状态需要同步更新，极易产生不一致。

**任务目的**：
合并双层 State 类，消除状态重复。

**技术要求**：
1. `RuntimeState` 作为 `GameWorldState` 的成员，而非并行结构
2. 消除重复字段（`interaction_state` 只存在于 `GameWorldState`）
3. `RuntimeState` 仅包含 UI 特有的运行时状态（如动画定时器）

**交付标准**：
- 无并行状态类
- 所有游戏状态在 `GameWorldState` 中统一管理

---

### BE-120 | 茶园地块别名导致状态混乱（High）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/GameRuntime.cpp` 第 173 行：
```cpp
world_state_.GetTeaGardenPlots() = farming_.Plots();
```
将 `farming_.Plots()` 别名赋给 `tea_garden_plots_`，导致三个地块集合指向同一份数据。修改一个影响另一个，但各自的增长率和品质规则不同。

**任务目的**：
茶园使用独立的地块集合。

**技术要求**：
1. 移除别名赋值：`tea_garden_plots_` 应为独立的 `std::vector<TeaPlot>`
2. 茶园地块在 `FarmingSystem::BuildDefaultPlots()` 之后单独创建
3. 三个地块集合独立存储，独立更新

**交付标准**：
- 茶园地块是独立的数据集合
- 普通田和茶园田互不影响

---

### BE-121 | 存档版本魔法数字（Medium）

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`src/engine/GameAppSave.cpp` 第 628-656 行使用硬编码魔法数字 `15` 和 `19` 判断存档字段数量：
```cpp
if (fields.size() >= 15) { /* v3 字段 */ }
if (fields.size() >= 19) { /* v4 字段 */ }
```

**任务目的**：
使用命名字段而非字段数量判断版本。

**技术要求**：
1. 在存档开头写入字段映射表（如 `fields_map:count=23`）
2. 解析时先读取字段映射表，再按名称查找字段
3. 移除 `fields.size() >= N` 的版本判断

**交付标准**：
- 存档使用字段名而非字段数量判断版本
- 增加字段不影响旧版本存档的兼容性

---

### BE-122 | 跨层依赖违规（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
- `src/domain/RecipeData.cpp` 第 5 行 include `"CloudSeamanor/GameAppText.hpp"`（engine 层）
- `src/domain/CloudGuardianContract.cpp` 第 5 行同样 include `"CloudSeamanor/GameAppText.hpp"`

**任务目的**：
消除 domain 层的跨层依赖。

**技术要求**：
1. `GameAppText` 改为纯文本 ID（字符串枚举）而非渲染对象
2. domain 层只存储 `ItemDisplayName` 的字符串 key
3. UI 层根据 key 从 `GameAppText` 获取渲染文本

**交付标准**：
- domain 层不含 engine 层依赖
- 文本显示完全由 UI 层处理

---

### BE-123 | 存档测试覆盖缺失（Medium）

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
存档系统（`SaveGameService.cpp` 和 `GameAppSave.cpp`）完全没有单元测试。存档是玩家数据的唯一保障，损坏可能导致游戏进度永久丢失。

**任务目的**：
为存档系统添加完整测试。

**技术要求**：
1. 创建 `tests/infrastructure/SaveGameServiceTest.cpp`
2. 测试用例：
   - 基本存档和读取
   - 存档损坏检测（checksum 错误）
   - 版本迁移（v3 → v4）
   - 字段缺失的默认值填充
   - 茶园数据存档和读取
   - NPC 进度存档和读取

**交付标准**：
- 存档系统至少 30 个测试用例
- CI 测试通过

---

### BE-124 | ResourceManager 字体搜索路径不完整（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/infrastructure/ResourceManager.cpp` 第 370-372 行只搜索 Windows 路径，macOS (`/System/Library/Fonts/`) 和 Linux (`/usr/share/fonts/`) 路径未实现。

**任务目的**：
实现跨平台字体搜索。

**技术要求**：
1. Windows: `C:/Windows/Fonts/` + 用户指定路径
2. macOS: `/System/Library/Fonts/` + `~/Library/Fonts/` + 用户指定路径
3. Linux: `/usr/share/fonts/` + `~/.fonts/` + 用户指定路径
4. 使用 `#ifdef _WIN32` / `#ifdef __APPLE__` / `#ifdef __linux__` 条件编译

**交付标准**：
- 三种平台均可找到可用字体
- 缺失字体时优雅降级到默认字体

---

### BE-125 | 每日两次日常重置（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`GameRuntime.cpp` 第 335 行 `OnDayChanged()` 和第 646 行 `SleepToNextMorning()` 都调用了 `ResetDailyInteractionState()` 和 `farming_.ResetDailyState()`。`SleepToNextMorning()` 在游戏内执行睡眠时调用，而 `OnDayChanged()` 在自然日变更时调用（每日凌晨 6:00）。两者都会在同一天触发。

**任务目的**：
消除双重日常重置。

**技术要求**：
1. `SleepToNextMorning()` 只做睡眠到次日，不重置日常状态
2. 日变更触发重置（`OnDayChanged()`）
3. 添加 `last_reset_day_` 字段确保重置只在每日第一次触发

**交付标准**：
- 日常状态每日只重置一次
- 睡眠和自然日变更不冲突

---

### BE-126 | NPC 心事件检测逻辑重复（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/domain/MainPlotSystem.cpp` 的 `GetTriggerablePlots_()`（第 1075-1086 行）、`CanTriggerPlot_()`（第 1126-1137 行）、`CanEnterPlot_()`（第 1160-1171 行）中都有一段相同的 NPC 好感度获取代码（重复 3 次）。

**任务目的**：
提取公共 NPC 好感度查询方法。

**技术要求**：
1. 在 `MainPlotSystem` 中添加私有方法 `GetNpcHeartLevels()`
2. 返回 `std::unordered_map<std::string, int>` 的 NPC 心等级映射
3. 所有三处使用同一方法

**交付标准**：
- NPC 心等级查询只在一处定义
- 新增 NPC 只需修改一处

---

### BE-127 | SpiritBeastWateredToday 类型不一致（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/GameAppSave.cpp` 第 156 行存档时使用 `bool&` 引用：
```cpp
bool& spirit_beast_watered_today,
```
但 `src/engine/GameWorldState.cpp` 第 219 行声明为：
```cpp
bool& GetSpiritBeastWateredToday() { return spirit_beast_watered_today_; }
```
存档函数参数类型与实际类型存在潜在不匹配。

**任务目的**：
统一 `spirit_beast_watered_today` 的类型声明。

**技术要求**：
1. 检查所有存档相关函数的参数类型签名
2. 确保 `bool` 参数与存档格式匹配
3. 添加存档解析时的类型转换保护

**交付标准**：
- 所有存档函数类型一致
- 存档解析无类型警告

---

### BE-128 | 农作物数据 CSV 校验工具完善（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`tools/validate_crop_table.py` 已存在但可能未覆盖所有校验场景。

**任务目的**：
完善作物表校验工具。

**技术要求**：
1. 校验规则：
   - `growth_time > 0`
   - `stages >= 1`
   - `base_harvest >= 1`
   - `seed_item_id` 和 `harvest_item_id` 非空
   - `season` 字段合法
   - 缺少季节字段时报警告
2. 输出格式：`[ERROR] line N: <field> 字段无效: <value>`
3. 校验通过时输出 `All checks passed`

**交付标准**：
- 工具可检测所有常见数据错误
- 错误信息包含行号和原因

---

### BE-129 | GameRuntime::HandleGiftInteraction 重复代码（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/GameRuntime.cpp` 第 948-1035 行 `HandleGiftInteraction()` 与第 1040-1127 行 `HandlePrimaryInteraction()` 几乎完全相同（88 行重复）。

**任务目的**：
消除交互处理函数的重复代码。

**技术要求**：
1. 提取公共的交互上下文构建逻辑
2. 将 `HandleGiftInteraction()` 和 `HandlePrimaryInteraction()` 合并为一个模板方法
3. 通过参数区分送礼和主交互

**交付标准**：
- 交互处理代码无重复
- 单一职责，逻辑清晰

---

### BE-130 | 对话选项回调静默失败（Medium）

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
`src/engine/GameApp.cpp` 第 352-355 行，选项 ID 不匹配时不执行任何操作也不报错：
```cpp
if (choices[i].id == choice_id) {
    engine.SelectChoice(i);
    return;
}
// 循环结束静默返回
```

**任务目的**：
添加选项回调失败的警告日志。

**技术要求**：
```cpp
if (choices[i].id == choice_id) {
    engine.SelectChoice(i);
    return;
}
// 循环结束
if (!choices.empty()) {
    Logger::Warn("选项 ID '{}' 未找到，回退到第一个选项", choice_id);
    engine.SelectChoice(0);
}
```

**交付标准**：
- 未找到选项 ID 时回退到第一个选项
- 记录警告日志便于调试

---

### BE-131 | 每日购买量/出售量未存档（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/GameRuntime.cpp` 第 438-441 行的 `weekly_buy_count` 和 `weekly_sell_count` 在每次存档时被保存，但存档加载时（第 1179-1182 行）这些数据未被读取恢复。

**任务目的**：
将周购买/出售统计纳入存档。

**技术要求**：
1. 在存档格式中添加 `weekly_buy` 和 `weekly_sell` 字段
2. 在 `LoadGameState()` 中解析并恢复这些值
3. 确保跨日重置正确（存档加载后次日才清零）

**交付标准**：
- 周购买/出售统计正确存档和读取
- 价格波动系统基于存档数据正确运行

---

### BE-132 | 邮件/订单系统数据未存档（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/engine/GameRuntime.cpp` 中邮件队列（`mail_orders_`, `pending_mail_`）的数据在任何地方都没有序列化。

**任务目的**：
将邮件/订单系统纳入存档。

**技术要求**：
1. 在 `SaveGameState()` 中序列化邮件队列
2. 在 `LoadGameState()` 中反序列化邮件队列
3. 格式：`mail|<item_id>|<delivery_day>|<count>`

**交付标准**：
- 邮件订单正确存档和读取
- 邮购物品跨存档持久化

---

### BE-133 | 建筑升级状态未存档（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
主屋升级进度、工坊等级等建筑状态在存档序列化中未体现。

**任务目的**：
将建筑升级状态纳入存档。

**技术要求**：
1. 在存档中添加建筑数据：
   ```
   building|<id>|<level>|<upgrade_progress>
   ```
2. 在加载时重建建筑状态

**交付标准**：
- 建筑等级正确存档和读取
- 升级进度正确恢复

---

### BE-134 | GameConfig 加载错误处理不完整（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/infrastructure/GameConfig.cpp` 的加载函数在遇到无效值时使用默认值但可能不记录警告。

**任务目的**：
完善 GameConfig 的错误日志。

**技术要求**：
1. 所有使用默认值的地方记录 `Logger::Warn()`
2. 日志格式：`[Config] <key> 无效 (value={}), 使用默认值 {default}`
3. 统计加载过程中使用的默认值数量

**交付标准**：
- 配置错误有详细日志
- 默认值使用有统计

---

### BE-135 | 单元测试覆盖率提升（High）

**进度标识**：🚧 开发中（2026-04-22）

**不足描述**：
当前测试覆盖率约 25%，engine 层（58+ 文件）和 infrastructure 层（9 文件）几乎无测试。`CMakeLists.txt` 第 320-329 行只注册了 8 个测试源，而 `tests/` 目录下实际有 20 个测试文件。

**任务目的**：
补全测试注册并提升覆盖率。

**技术要求**：
1. 更新 `CMakeLists.txt` 的 `TEST_SOURCES` 列表，注册所有 20 个测试文件
2. 补充缺失测试：
   - `InteractionSystemTest.cpp`：15+ 用例（交互结果测试）
   - `CloudSystemTest.cpp`：10+ 用例（天气状态测试）
   - `SaveGameServiceTest.cpp`：20+ 用例
   - `DialogueEngineTest.cpp`：10+ 用例
   - `FarmingSystemTest.cpp`：10+ 用例
3. 覆盖率目标：engine 层 ≥ 30%，infrastructure 层 ≥ 60%

**交付标准**：
- CMakeLists.txt 注册所有 20 个测试文件
- CI 中所有测试通过
- engine 层测试覆盖率 ≥ 30%

---

### BE-136 | TMX 地图加载健壮性增强（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
`src/infrastructure/TmxMapLoader.cpp` 在遇到无效图层或对象时可能崩溃。

**任务目的**：
增强 TMX 地图加载的容错能力。

**技术要求**：
1. 地图文件不存在时使用默认地图
2. 解析错误时跳过错误元素并记录日志
3. 缺少必需图层时使用占位符填充
4. 添加 `Result<TmxMap>` 返回值模式

**交付标准**：
- 地图加载失败不导致游戏崩溃
- 错误信息详细且可调试

---

### BE-137 | 缺失资源文件占位符（Medium）

**进度标识**：✅ 已完成（2026-04-22）

**不足描述**：
多个源文件引用了不存在的资源文件：
- `ResourceManager.cpp`：`assets/fonts/simhei.ttf`, `msyh.ttc`, `arial.ttf`
- `DayCycleRuntime.cpp`：`assets/audio/bgm/spring_theme.ogg` 等 4 个 BGM
- `SpriteAssetManager.cpp`：`npc_villagers.atlas.json`, `tiles_farm.atlas.json`
- `GameRuntime.cpp`：`save_slot_placeholder.png`

**任务目的**：
为缺失资源创建占位符，防止游戏崩溃。

**技术要求**：
1. 字体：创建最小化的占位字体文件或使用内置回退
2. 音频：使用 SFML 的静音处理，不崩溃
3. 精灵：使用纯色占位纹理
4. 缩略图：创建 1×1 灰色占位图

**交付标准**：
- 所有缺失资源有合理的占位符/回退处理
- 游戏在资源缺失时优雅降级而非崩溃

---

## 第三部分：补充任务总览

### 前端 UI 补充任务（UI-101 ~ UI-139）

| ID | 不足描述 | 严重度 | 分类 |
|----|---------|--------|------|
| UI-101 | Tooltip 渲染链路完全缺失 | Critical | 交互 |
| UI-102 | 空状态界面完全缺失 | Critical | 缺失状态 |
| UI-103 | 全局键盘导航系统缺失 | Critical | 交互 |
| UI-104 | 对话框选项悬停状态缺失 | Critical | 交互 |
| UI-105 | 进度条标签文本从不渲染 | Critical | 缺失状态 |
| UI-106 | 设置面板修改不持久化 | Critical | 交互 |
| UI-107 | 窗口大小改变未处理 | Critical | 缺陷 |
| UI-108 | 像素直线算法实现错误 | Critical | 缺陷 |
| UI-109 | 金币显示使用 Emoji | Critical | 视觉 |
| UI-110 | NPC 头像无边界裁剪 | Critical | 视觉 |
| UI-111 | Tab 标签栏无悬停状态 | Critical | 交互 |
| UI-112 | 已选物品无视觉反馈 | Critical | 交互 |
| UI-113 | 任务菜单悬停检测忽略 X 坐标 | Critical | 交互 |
| UI-114 | 面板可见性检查不一致 | High | 缺陷 |
| UI-115 | 魔法数字遍布代码 | High | 规范 |
| UI-116 | 社交页 NPC 卡片无悬停反馈 | High | 交互 |
| UI-117 | 无加载状态显示 | High | 缺失状态 |
| UI-118 | 进度条低值闪烁性能浪费 | High | 性能 |
| UI-119 | 可交互对象无禁用状态渲染 | High | 缺失状态 |
| UI-120 | 标签栏悬停效果缺失 | High | 交互 |
| UI-121 | 闪烁定时器使用绝对重置 | Medium | 缺陷 |
| UI-122 | 字体缺失时无占位符 | Medium | 缺陷 |
| UI-123 | 设置面板 slot 未清除 | Medium | 缺陷 |
| UI-124 | 存档覆盖无确认对话框 | Medium | 交互 |
| UI-125 | 小地图玩家点可溢出边界 | Medium | 缺陷 |
| UI-126 | 对话框跳过提示无视觉差异 | Medium | 交互 |
| UI-127 | NPC 社交详情面板缺失 | Medium | 缺失状态 |
| UI-128 | 通知横幅堆叠管理缺失 | Medium | 缺陷 |
| UI-129 | UI 面板无法拖动 | Medium | 交互 |
| UI-130 | 背包操作按钮未实现 | Medium | 缺陷 |
| UI-131 | 任务面板展开/折叠功能缺失 | Medium | 缺陷 |
| UI-132 | 右键上下文菜单缺失 | Medium | 交互 |
| UI-133 | 存档缩略图生成缺失 | Medium | 缺陷 |
| UI-134 | 像素缩放模式未强制 | Medium | 规范 |
| UI-135 | 世界提示浮动动画不自然 | Medium | 视觉 |
| UI-136 | UI 组件测试覆盖缺失 | Medium | 测试 |
| UI-137 | 面板动画时长不可配置 | Low | 规范 |
| UI-138 | 对话框选项键盘导航缺失 | Low | 交互 |
| UI-139 | 无障碍触控热区不达标 | Low | 规范 |

### 后端补充任务（BE-101 ~ BE-137）

| ID | 不足描述 | 严重度 | 层级 |
|----|---------|--------|------|
| BE-101 | Domain 层 SFML 依赖违规 | Critical | domain |
| BE-102 | MainPlotSystem 命名空间违规 | Critical | domain |
| BE-103 | 茶园地块数据未存档 | Critical | engine |
| BE-104 | 云海日期推进双重调用导致状态损坏 | Critical | engine |
| BE-105 | NpcDialogueManager 候选列表空时越界访问 | Critical | engine |
| BE-106 | 茶园永久品质 BUFF | Critical | engine |
| BE-107 | ResourceManager::GetFont 返回错误字体 | Critical | infrastructure |
| BE-108 | BattleField 语法错误 | Critical | engine |
| BE-109 | CloudSystem 未设置随机数种子 | High | domain |
| BE-110 | MainPlotSystem NPC 列表重复 4 次 | High | domain |
| BE-111 | MainPlotSystem LoadState 不清空旧状态 | High | domain |
| BE-112 | QuestManager 自动接取逻辑竞态 | High | engine |
| BE-113 | FarmingSystem 忽略 AddItem/RemoveItem 失败 | High | engine |
| BE-114 | 作物收获阈值硬编码 | High | engine |
| BE-115 | CloudGuardianContract LoadState 是空操作 | High | domain |
| BE-116 | InputManager::LoadFromFile 是空实现 | High | engine |
| BE-117 | DialogEngine 和 NpcDialogueManager 代码重复 | High | engine |
| BE-118 | NpcDialogueManager 候选 ID 提取脆弱 | High | engine |
| BE-119 | 双层 State 类导致状态重复 | High | engine |
| BE-120 | 茶园地块别名导致状态混乱 | High | engine |
| BE-121 | 存档版本魔法数字 | Medium | infrastructure |
| BE-122 | 跨层依赖违规 | Medium | domain |
| BE-123 | 存档测试覆盖缺失 | Medium | infrastructure |
| BE-124 | ResourceManager 字体搜索路径不完整 | Medium | infrastructure |
| BE-125 | 每日两次日常重置 | Medium | engine |
| BE-126 | NPC 心事件检测逻辑重复 | Medium | domain |
| BE-127 | SpiritBeastWateredToday 类型不一致 | Medium | engine |
| BE-128 | 农作物数据 CSV 校验工具完善 | Medium | infrastructure |
| BE-129 | GameRuntime::HandleGiftInteraction 重复代码 | Medium | engine |
| BE-130 | 对话选项回调静默失败 | Medium | engine |
| BE-131 | 每日购买量/出售量未存档 | Medium | engine |
| BE-132 | 邮件/订单系统数据未存档 | Medium | engine |
| BE-133 | 建筑升级状态未存档 | Medium | engine |
| BE-134 | GameConfig 加载错误处理不完整 | Medium | infrastructure |
| BE-135 | 单元测试覆盖率提升 | High | app |
| BE-136 | TMX 地图加载健壮性增强 | Medium | infrastructure |
| BE-137 | 缺失资源文件占位符 | Medium | infrastructure |

---

## 附录：优先修复路线图

### 第一优先级（必须立即修复）

这些是阻塞性问题，直接影响游戏正常运行或数据安全：

```
Critical 前端: UI-101 → UI-103 → UI-105 → UI-106 → UI-107 → UI-108 → UI-109 → UI-110 → UI-111 → UI-112 → UI-113
Critical 后端: BE-101 → BE-102 → BE-103 → BE-104 → BE-105 → BE-106 → BE-107 → BE-108
```

### 第二优先级（影响功能正确性）

```
High 前端: UI-114 → UI-115 → UI-116 → UI-117 → UI-118 → UI-119 → UI-120
High 后端: BE-109 → BE-110 → BE-111 → BE-112 → BE-113 → BE-114 → BE-115 → BE-116 → BE-117 → BE-118 → BE-119 → BE-120 → BE-135
```

### 第三优先级（影响可维护性）

```
Medium 前端: UI-121 → UI-122 → UI-123 → UI-124 → UI-125 → UI-126 → UI-127 → UI-128 → UI-129 → UI-130 → UI-131 → UI-132 → UI-133 → UI-134 → UI-135 → UI-136 → UI-137 → UI-138 → UI-139
Medium 后端: BE-121 → BE-122 → BE-123 → BE-124 → BE-125 → BE-126 → BE-127 → BE-128 → BE-129 → BE-130 → BE-131 → BE-132 → BE-133 → BE-134 → BE-136 → BE-137
```

---

*文档版本 1.0 | 2026-04-22 | 项目不足检查与补充任务清单，与《云海山庄》前端 UI 任务清单和后端任务清单配套使用*
