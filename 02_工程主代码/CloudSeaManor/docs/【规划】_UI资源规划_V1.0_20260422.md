# 【规划】UI资源规划 V1.0

## 目标范围

- 对应任务：`UI-044` ~ `UI-048`
- 目标：在不阻塞程序开发的前提下，统一图集尺寸、命名、占位符策略与导出规范

## UI-044 图集规划

- `assets/sprites/ui/ui_main.png`：`1024x1024`
  - 边框切片：`corner_8x8_*`，`edge_8x1_*`
  - 按钮：`btn_default_*` / `btn_hover_*` / `btn_pressed_*`
  - 对话框、进度条、工具栏基础样式块
- `assets/sprites/ui/ui_icons.png`：`512x256`
  - 天气：晴/薄雾/浓云/大潮
  - 品质：普通/优质/稀有/传说
  - 状态：心/星/锁/箭头/确认/关闭
- 输出：`ui_main.atlas.json`、`ui_icons.atlas.json`

## UI-045 NPC头像规划

- 图块尺寸：`64x64`
- 排列：`13 NPC x 4 表情 = 52` 帧
- 占位策略：每个 NPC 使用稳定主色 + 4 档亮度作为表情差异

## UI-046 物品图标规划

- 尺寸：`16x16`（小图标）与 `32x32`（主图标）
- 图集分拆：
  - `items_crop.png`
  - `items_tool.png`
  - `items_material.png`
  - `items_product.png`
- 占位策略：按品类编码颜色（作物绿、工具棕、材料灰、产物金）

## UI-047 瓦片图集规划

- 尺寸：`32x32`
- 图集分拆：
  - `tiles_farm.png`
  - `tiles_spirit.png`
  - `tiles_building.png`
  - `tiles_decoration.png`
- 占位策略：纯色+边线区分可走/阻挡/装饰层

## UI-048 粒子图集规划

- 尺寸：`8x8` 与 `16x16`
- 图集分拆：
  - `particles_basic.png`
  - `particles_nature.png`
  - `particles_magic.png`
- 首批占位符（>=4）：心形、星点、水滴、灵气环

## 命名与落库约定

- 纹理命名：小写下划线
- 帧命名：`{category}_{name}_{index}`
- 所有规划文档后续版本沿用同命名格式升级
