# 美术资源管理 · 美术师操作手册

**版本**: 1.0
**日期**: 2026-04-21
**适用**: 美术团队、程序团队

---

## 目录

1. [资源目录结构](#1-资源目录结构)
2. [图集制作规范](#2-图集制作规范)
3. [图集元数据 (.atlas.json)](#3-图集元数据-atlasjson)
4. [页面布局配置 (.ui.json)](#4-页面布局配置-uijson)
5. [美术师工作流程](#5-美术师工作流程)
6. [命名规范](#6-命名规范)
7. [程序集成接口](#7-程序集成接口)

---

## 1. 资源目录结构

```
assets/
├── sprites/
│   ├── characters/          # 角色精灵
│   │   ├── player_main.png   # 主角（含所有方向动画）
│   │   └── player_main.atlas.json
│   ├── npc/                 # NPC 精灵
│   │   ├── acha.png
│   │   ├── lin.png
│   │   └── ...
│   ├── items/               # 物品精灵
│   │   ├── items_crop.png   # 作物/材料
│   │   └── items_crop.atlas.json
│   ├── tiles/              # 地形瓦片
│   │   ├── tiles_farm.png
│   │   └── tiles_farm.atlas.json
│   ├── ui/                 # UI 图集
│   │   ├── ui_main.png      # UI 主图集
│   │   ├── ui_main.atlas.json
│   │   ├── ui_icons.png    # 小图标
│   │   └── ui_icons.atlas.json
│   └── effects/             # 特效精灵
│
└── ui/
    └── pages/               # 页面布局配置
        ├── inventory.ui.json  # 背包
        ├── workshop.ui.json  # 工坊
        ├── contract.ui.json  # 契约系统
        └── ...
```

---

## 2. 图集制作规范

### 2.1 PNG 图集规格

| 类型 | 推荐尺寸 | 最大尺寸 | 格式 |
|------|----------|----------|------|
| 角色精灵 | 256×512 | 1024×1024 | PNG 32-bit |
| NPC 精灵 | 256×256 | 512×512 | PNG 32-bit |
| 物品精灵 | 256×128 | 512×512 | PNG 32-bit |
| UI 图集 | 512×256 | 1024×1024 | PNG 32-bit (透明) |
| 地形瓦片 | 256×256 | 512×512 | PNG 32-bit |
| 特效 | 256×128 | 512×512 | PNG 32-bit (透明) |

### 2.2 网格布局

所有图集使用**固定网格**排列，便于程序自动解析。

- **角色精灵**: 32×32px 每帧，8 列（8 方向 × 1 帧）
- **物品精灵**: 32×32px 每格，8 列
- **UI 图标**: 24×24px 每格，16 列
- **地形瓦片**: 16×16px 或 32×32px，16 列

### 2.3 角色精灵排列规范

```
图集: 256×512 (8列×16行)
每帧: 32×32px

行 0-3:   待机 (idle_down, idle_right, idle_up, idle_left)
行 4-7:   行走 (walk_down, walk_right, walk_up, walk_left)
行 8-11:  工具使用 (tool_hoe, tool_water, tool_axe, tool_pick)
行 12-15: 特殊动作 (harvest, sleep, swim, ride)
```

### 2.4 导出要求

- **像素游戏**: 导出为 **PNG**，**不要**抗锯齿
- **分辨率**: 32-bit RGBA（支持透明）
- **命名**: 全部 **小写 + 下划线**（如 `player_main.png`）
- **空白边距**: 每个精灵周围留 1px 透明边距（防止渲染粘连）

---

## 3. 图集元数据 (.atlas.json)

每张 PNG 图集必须配套一个同名的 `.atlas.json` 文件。

### 3.1 完整示例

```json
{
  "id": "player_main",
  "texture": "assets/sprites/characters/player_main.png",
  "width": 256,
  "height": 512,
  "grid": {
    "frame_width": 32,
    "frame_height": 32,
    "columns": 8,
    "rows": 16
  },
  "frames": [
    {
      "id": "idle_down_0",
      "x": 0, "y": 0, "w": 32, "h": 32,
      "pivotX": 0.5, "pivotY": 1.0,
      "duration": 100
    }
  ],
  "animations": [
    {
      "id": "idle_down",
      "name": "待机·下",
      "loop": true,
      "speed": 1.0,
      "frames": [
        "idle_down_0", "idle_down_1", "idle_down_2", "idle_down_3"
      ]
    }
  ]
}
```

### 3.2 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `id` | string | 是 | 图集 ID，英文小写 + 下划线 |
| `texture` | string | 是 | PNG 相对路径 |
| `width` | int | 是 | PNG 宽度（像素） |
| `height` | int | 是 | PNG 高度（像素） |
| `grid` | object | 否 | 网格布局（用于自动解析） |
| `frames` | array | 是 | 所有帧定义 |
| `frames[].id` | string | 是 | 帧 ID（不含图集前缀） |
| `frames[].x` | int | 是 | 左上角 X |
| `frames[].y` | int | 是 | 左上角 Y |
| `frames[].w` | int | 是 | 帧宽度 |
| `frames[].h` | int | 是 | 帧高度 |
| `frames[].pivotX` | float | 否 | 旋转中心 X（0-1，默认 0.5） |
| `frames[].pivotY` | float | 否 | 旋转中心 Y（0-1，默认 1.0=底部） |
| `frames[].duration` | int | 否 | 帧持续时间（ms，默认 100） |
| `animations` | array | 否 | 动画定义 |
| `animations[].id` | string | 是 | 动画 ID |
| `animations[].name` | string | 是 | 显示名（中文） |
| `animations[].loop` | bool | 否 | 是否循环（默认 true） |
| `animations[].speed` | float | 否 | 速度倍率（默认 1.0） |
| `animations[].frames` | array | 是 | 帧 ID 序列 |

### 3.3 自动生成工具

运行 AssetBridge 可自动扫描目录并生成 `.atlas.json`：

```cpp
// 程序中调用
AssetBridge bridge(config);
auto results = bridge.ScanDirectory("assets/sprites/");
// 自动为每个 PNG 生成 .atlas.json
```

或通过命令行工具：
```bash
# 将来可扩展为命令行工具
./CloudSeaManor --scan-sprites assets/sprites/
```

---

## 4. 页面布局配置 (.ui.json)

每个 UI 页面由一个 `.ui.json` 配置文件定义。

### 4.1 完整示例

```json
{
  "page_id": "inventory",
  "title": "背包",
  "width": 640,
  "height": 480,
  "modal": false,
  "pause_game": false,
  "background": {
    "sprite_id": "ui_main.panel_fill",
    "tint": "#FBF4E0",
    "alpha": 0.94
  },
  "elements": [
    {
      "id": "tab_bar",
      "type": "TabBar",
      "x": 0, "y": 40,
      "width": 640, "height": 40,
      "tabs": [
        {"id": "tab_items", "text": "物品"},
        {"id": "tab_tools", "text": "工具"}
      ],
      "active_tab": 0
    },
    {
      "id": "grid",
      "type": "Grid",
      "x": 16, "y": 88,
      "width": 408, "height": 376,
      "grid_columns": 8,
      "spacing_x": 4,
      "spacing_y": 4
    },
    {
      "id": "btn_sell",
      "type": "Button",
      "x": 0, "y": 288,
      "width": 80, "height": 32,
      "text": "出售",
      "sprite_id": "ui_main.btn_default",
      "enabled": false,
      "on_click": "inventory.sell_item"
    }
  ]
}
```

### 4.2 元素类型

| type | 说明 | 特殊属性 |
|------|------|----------|
| `Empty` | 占位空白 | - |
| `Image` | 静态图片 | `sprite_id` |
| `Image9Patch` | 9 宫格图片 | `sprite_id`, `9patch_border` |
| `AnimatedSprite` | 动画精灵 | `atlas_id`, `anim_id` |
| `Text` | 文本 | `text`, `text_style` |
| `Button` | 按钮 | `text`, `sprite_id`, `on_click` |
| `ProgressBar` | 进度条 | `progress_value`, `progress_max` |
| `IconSlot` | 图标槽 | `item_id`, `item_count` |
| `ScrollPanel` | 滚动面板 | `children` |
| `TabBar` | 标签栏 | `tabs`, `active_tab` |
| `HeartBar` | 好感心条 | `heart_count`, `heart_max` |
| `Container` | 容器 | `children`, `layout` |

### 4.3 布局规则

| layout | 说明 |
|--------|------|
| `Absolute` | 绝对像素坐标 |
| `Anchor` | 锚点布局（相对父容器） |
| `Flow` | 流式布局（横向/纵向排列） |
| `Grid` | 网格布局 |
| `Stack` | 堆叠布局（同位叠加） |

---

## 5. 美术师工作流程

### 5.1 新增角色精灵

1. 在 `assets/sprites/characters/` 创建 PNG（如 `new_character.png`）
2. 按网格规范排列帧（32×32px）
3. 创建 `new_character.atlas.json`，填写帧坐标和动画
4. 程序运行 `AssetBridge::ScanDirectory()` 自动注册
5. 在代码中使用：
   ```cpp
   SpriteAnimator animator(&sprite_manager);
   animator.BindSprite(&player_sprite);
   animator.Play("new_character", "idle_down");
   ```

### 5.2 新增 UI 元素

1. 在 `assets/sprites/ui/ui_main.png` 添加新图标
2. 更新 `ui_main.atlas.json`，添加帧定义
3. 在 `assets/ui/pages/` 创建或编辑 `.ui.json`
4. 引用：`"sprite_id": "ui_main.icon_new_feature"`

### 5.3 新增 UI 页面

1. 复制 `assets/ui/pages/_template.ui.json` 为新文件
2. 修改 `page_id` 和 `title`
3. 添加/删除 `elements`
4. 每个元素的 `sprite_id` 引用图集中的帧 ID

---

## 6. 命名规范

### 6.1 图集命名

```
{主题}_{子分类}.png
```

| 示例 | 说明 |
|------|------|
| `player_main.png` | 主角主精灵 |
| `npc_acha.png` | 阿茶 NPC |
| `items_crop.png` | 作物物品 |
| `tiles_farm.png` | 农场地形 |
| `ui_main.png` | UI 主图集 |
| `effects_magic.png` | 魔法特效 |

### 6.2 帧命名

```
{动作}_{方向}_{序号}
```

| 示例 | 说明 |
|------|------|
| `idle_down_0` | 待机·下·第0帧 |
| `walk_right_1` | 行走·右·第1帧 |
| `tool_hoe_down_0` | 使用锄头·下·第0帧 |
| `harvest_2` | 收获·第2帧（无方向） |

### 6.3 动画命名

```
{动作}_{方向}
```

| 示例 | 说明 |
|------|------|
| `idle_down` | 待机·下 |
| `walk_right` | 行走·右 |
| `tool_down` | 使用工具·下 |

### 6.4 UI 元素命名

```
{类型}_{功能}_{序号}
```

| 示例 | 说明 |
|------|------|
| `btn_sell` | 出售按钮 |
| `tab_items` | 物品标签 |
| `slot_0` | 物品格 0 |
| `icon_heart_full` | 满心图标 |

---

## 7. 程序集成接口

### 7.1 初始化

```cpp
// GameApp.cpp 或初始化代码中

// 1. 创建资产管理器
auto sprite_mgr = std::make_unique<SpriteAssetManager>(
    resources.get());  // 传入 ResourceManager

// 2. 加载关键图集
sprite_mgr->LoadAtlases({
    {"assets/sprites/characters/player_main.atlas.json"},
    {"assets/sprites/npc/npc_villagers.atlas.json"},
    {"assets/sprites/items/items_crop.atlas.json"},
    {"assets/sprites/ui/ui_main.atlas.json"},
});

// 3. 创建页面管理器
UiPageManager page_mgr;
page_mgr.SetAssetManager(sprite_mgr.get());
page_mgr.SetFontRenderer(pixel_hud_->GetFontRenderer());

// 4. 加载所有页面配置
page_mgr.LoadPagesFromDirectory("assets/ui/pages/");
```

### 7.2 在游戏循环中使用

```cpp
// 游戏主循环

// 更新
page_mgr.Update(delta_seconds);

// 事件处理
page_mgr.HandleMouseMove(mx, my);
page_mgr.HandleMouseClick(mx, my);
page_mgr.HandleKeyPressed(key);

// 渲染（在世界渲染之后）
world_renderer.Render(window, world_state);
page_mgr.Render(window);  // UI 页面渲染
```

### 7.3 打开/关闭页面

```cpp
// 打开背包
page_mgr.OpenPage("inventory");

// 关闭背包
page_mgr.ClosePage("inventory");

// 切换
page_mgr.TogglePage("workshop");

// 关闭所有
page_mgr.CloseAllPages();
```

### 7.4 绑定游戏数据

```cpp
// 获取页面
UiPage* page = page_mgr.GetPage("inventory");

// 绑定数据
UiElement* grid = page->GetElement("grid");
grid->BindData("items", game_inventory.GetItems());

// 刷新 UI
page->RefreshBindings();
```

### 7.5 注册回调

```cpp
UiPage* page = page_mgr.GetPage("inventory");
UiElement* btn = page->GetElement("btn_sell");
btn->on_click = [](const std::string& id) {
    game_inventory.SellSelectedItem();
};
```

---

## 附录: 快速参考卡片

### 图集尺寸速查

| 图集类型 | 尺寸 | 帧数 (32×32) |
|----------|------|--------------|
| 角色 (角色精灵) | 256×512 | 128 帧 |
| NPC (单角色精灵) | 256×256 | 64 帧 |
| 物品 (作物/材料) | 256×128 | 32 帧 |
| UI (主图集) | 512×256 | 128 帧 |
| 地形 (瓦片集) | 256×256 | 64 帧 |

### pivotY 快速参考

| pivotY | 用途 |
|--------|------|
| 1.0 | 底部对齐（角色站立点） |
| 0.5 | 中心对齐（图标/特效） |
| 0.0 | 顶部对齐 |

### 云海山庄专用图标清单

| 图标 ID | 说明 |
|---------|------|
| `ui_main.icon_stamina` | 体力图标 |
| `ui_main.icon_coin` | 金币图标 |
| `ui_main.icon_spirit` | 灵气图标 |
| `ui_main.icon_cloud_clear` | 晴 |
| `ui_main.icon_cloud_mist` | 薄雾 |
| `ui_main.icon_cloud_dense` | 浓云海 |
| `ui_main.icon_cloud_tide` | 大潮 |
| `ui_main.icon_heart_full` | 满心 |
| `ui_main.icon_heart_half` | 半心 |
| `ui_main.icon_heart_empty` | 空心 |
| `ui_main.icon_check` | 对勾 |
| `ui_main.icon_close` | 关闭 |

---

*本手册配合 `SpriteAssetManager.hpp`、`SpriteRenderer.hpp`、`UiLayoutSystem.hpp`、`AssetBridge.hpp` 使用*
