# 美术资源指南

## 当前状态

项目使用 **Kenney 像素艺术资源包**，所有资源遵循 **CC0 许可证**（可免费用于个人、教育和商业项目）。

## 需要下载的资源包

### 1. Tiny Town (必需 - 基础场景)

**下载地址**: https://kenney.nl/assets/tiny-town

**内容**: 16x16 像素瓷砖，用于建造和装饰

**下载后放置位置**:
```
assets/textures/third_party/kenney_tiny-town/
├── PNG/
│   ├── tilemap.png          (主图集)
│   └── tilemap_packed.png   (打包版本)
├── Tilesheet.txt
├── License.txt
└── (其他文件)
```

---

### 2. Tiny Dungeon (必需 - UI 面板)

**下载地址**: https://kenney.nl/assets/tiny-dungeon

**内容**: 16x16 像素地牢主题资源，可用于 UI 面板

**下载后放置位置**:
```
assets/textures/third_party/kenney_tiny-dungeon/
├── PNG/
│   └── tilemap.png
└── License.txt
```

---

### 3. Input Prompts Pixel (必需 - 键盘/手柄图标)

**下载地址**: https://kenney.nl/assets/input-prompts-pixel

**内容**: 键盘、手柄按钮图标

**下载后放置位置**:
```
assets/textures/third_party/kenney_input-prompts-pixel/
├── PNG/
│   ├── Keyboard/
│   │   └── *.png
│   ├── Gamepad/
│   │   └── *.png
│   └── tilemap.png
└── License.txt
```

---

### 4. Pixel Platformer - Farm Expansion (可选 - 农作物)

**下载地址**: https://kenney.nl/assets/pixel-platformer-farm-expansion

**内容**: 农场主题像素资源，包括作物、工具

**下载后放置位置**:
```
assets/textures/third_party/kenney_pixel-platformer-farm-expansion/
├── PNG/
│   └── *.png
└── License.txt
```

---

### 5. Pixel Platformer - Food Expansion (可选 - 食物/料理)

**下载地址**: https://kenney.nl/assets/pixel-platformer-food-expansion

**内容**: 食物像素资源

---

## 图集文件配置参考

### 已更新的纹理路径

| Atlas 文件 | 新纹理路径 |
|-----------|------------|
| `ui_main.atlas.json` | `assets/textures/third_party/kenney_tiny-dungeon/PNG/tilemap.png` |
| `player_main.atlas.json` | `assets/textures/third_party/kenney_tiny-town/PNG/tilemap.png` |
| `items_crop.atlas.json` | `assets/textures/third_party/kenney_pixel-platformer-farm-expansion/PNG/Tiles/tiles_packed.png` |
| `ui_icons.atlas.json` | `assets/textures/third_party/kenney_input-prompts-pixel/PNG/Keyboard/tiles.png` |
| `ui_borders.atlas.json` | `assets/textures/third_party/kenney_tiny-dungeon/PNG/tilemap.png` |
| `npc_villagers.atlas.json` | `assets/textures/third_party/kenney_tiny-town/PNG/tilemap.png` |
| `tiles_farm.atlas.json` | `assets/textures/third_party/kenney_pixel-platformer-farm-expansion/PNG/Tiles/tiles_packed.png` |

---

### 完整目录结构 (下载后)

```
assets/
├── textures/
│   └── third_party/
│       ├── kenney_tiny-town/
│       │   └── PNG/
│       │       └── tilemap.png          ← player_main, npc_villagers 使用
│       ├── kenney_tiny-dungeon/
│       │   └── PNG/
│       │       └── tilemap.png          ← ui_main, ui_borders 使用
│       ├── kenney_input-prompts-pixel/
│       │   └── PNG/
│       │       ├── Keyboard/
│       │       │   └── tiles.png        ← ui_icons 使用
│       │       └── tilemap.png
│       └── kenney_pixel-platformer-farm-expansion/
│           └── PNG/
│               └── Tiles/
│                   └── tiles_packed.png  ← items_crop, tiles_farm 使用
└── sprites/
    ├── ui/
    │   ├── ui_main.atlas.json       ✅ 已创建
    │   ├── ui_icons.atlas.json       ✅ 已创建
    │   └── ui_borders.atlas.json    ✅ 已创建
    ├── characters/
    │   └── player_main.atlas.json   ✅ 已更新路径
    ├── items/
    │   └── items_crop.atlas.json    ✅ 已更新路径
    ├── npc_villagers.atlas.json     ✅ 已创建
    └── tiles_farm.atlas.json        ✅ 已创建
```

---

## 快速开始

### Windows PowerShell 命令下载

```powershell
# 创建目录结构
mkdir -Force "assets/textures/third_party/kenney_tiny-town/PNG"
mkdir -Force "assets/textures/third_party/kenney_tiny-dungeon/PNG"
mkdir -Force "assets/textures/third_party/kenney_input-prompts-pixel/PNG"

# 访问 https://kenney.nl/assets/tiny-town 下载 ZIP
# 解压后将 PNG 文件夹内容放入上述目录
```

### 或者使用命令行下载 (需要 curl)

```bash
# Tiny Town
curl -L -o tiny-town.zip "https://kenney.nl/assets/tiny-town?filename=KenneyTinytown.zip"
unzip tiny-town.zip -d temp-tiny-town
# 移动 PNG 文件到正确位置

# Tiny Dungeon
curl -L -o tiny-dungeon.zip "https://kenney.nl/assets/tiny-dungeon"
unzip tiny-dungeon.zip -d temp-tiny-dungeon
```

---

## 资源尺寸参考

| 资源包 | 瓷砖尺寸 | 推荐用途 |
|--------|---------|---------|
| Tiny Town | 16x16 | 建筑、装饰物 |
| Tiny Dungeon | 16x16 | UI 面板、边框 |
| Input Prompts Pixel | 16x16 / 32x32 | 按钮图标 |
| Farm Expansion | 16x16 / 32x32 | 农作物 |

---

## 许可证归属

所有 Kenney 资源包含以下归属（可选）:

```
Created/distributed by Kenney (www.kenney.nl)
```

根据 CC0 许可证，这不是强制要求，但建议在游戏的 credits 中提及。

---

## 替代资源源

如果 Kenney.nl 不可访问，可以使用免费的替代资源：

1. **OpenGameArt.org** - 免费像素艺术资源
2. **itch.io** - 免费/付费像素艺术
3. **GameDev Market** - 付费像素艺术包

---

## 下一步

1. 下载上述资源包
2. 将 PNG 文件放置到正确目录
3. 更新 atlas JSON 文件中的 `texture` 路径
4. 运行游戏测试资源加载
