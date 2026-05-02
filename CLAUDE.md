# 云海山庄 — CLAUDE.md

> AI 助手项目规范。任何人用 AI 辅助开发前必须先读此文件。

## 项目基本信息

- **名称**: 云海山庄 / Cloud Sea Manor
- **类型**: 治愈系生活模拟 + 轻度奇幻冒险 + 农场经营
- **平台**: PC（Steam 优先）/ Nintendo Switch
- **技术栈**: C++20 / SFML 3.0.2 / CMake
- **版本**: 0.1.0（MVP 原型阶段）
- **架构**: 四层分离（app / engine / domain / infrastructure）
- **构建**: MSVC 2022 + CMakePresets（推荐）/ 手动 CMake

## 目录结构（企业级规范）

```
云海山庄 /
│  # --- 标准文件夹（7个，永不混放）---
│
├─ 01_项目核心文档/          # 所有策划、需求、设计、GDD
│   └─ 项目资料/              # 人物卡、世界观、系统设计、数值骨架、对话脚本等
│   └─ 01_GDD游戏总纲/
│   └─ 02_世界观与剧情/
│   └─ 03_系统设计文档/
│   └─ 04_开发规划与排期/
│   └─ 05_版本与BUG管理/
│
├─ 02_工程主代码/            # 唯一代码区
│   └─ CloudSeaManor/         # 全部 C++ 源码、头文件、资产、测试
│
├─ 03_美术与资源源文件/       # PSD、原画、特效源文件（待填）
│
├─ 04_发布与构建包/
│   └─ 01_MVP原型版本/
│   └─ 02_开发构建包/
│   └─ 03_历史版本归档/       # SFML 压缩包等
│
├─ 05_参考与第三方资源/
│   └─ 01_SFML引擎/
│   └─ 02_浏览器自动化工具/
│   └─ 03_开发脚本/
│   └─ 04_参考案例/
│
├─ 06_测试与验收/
│   └─ 测试用例/
│   └─ 验收报告/
│   └─ 性能报告/
│
├─ 07_归档与废弃/
│   └─ 01_旧文档/
│   └─ 02_废弃方案/
│   └─ 03_早期原型/
│
│  # --- 根目录文件（仅允许以下4个）---
├─ .gitattributes             # Git 配置
├─ .gitignore                # Git 配置
├─ README.md                 # 项目入口文档
└─ CLAUDE.md                 # 本文件
```

**核心原则**: 代码只在一个地方（`02_工程主代码/`），所有策划文档只在一个地方（`01_项目核心文档/`），任何新文件必须进入对应目录，不许放根目录。

## 工程代码结构

```
CloudSeaManor/
├── src/
│   ├── app/                  # 程序入口（main.cpp）
│   ├── domain/               # 纯玩法规则（Player、Inventory、GameClock……）
│   ├── engine/               # 运行时编排（HUD 渲染、NPC 运行时……）
│   └── infrastructure/        # IO 与解析（配置、日志、TmxMap）
├── include/CloudSeamanor/    # 公共头文件
├── assets/                   # 运行时资源（data/、maps/、fonts/、audio/）
│   └── data/
│       ├── battle/           # 战斗系统数据
│       ├── plot/             # 剧情对话数据
│       ├── battle/           # 战斗配置
│       └── *.csv / *.json    # 各种游戏数据表
├── configs/                  # 运行时配置（gameplay.cfg）
├── tests/                    # 单元测试
├── docs/                     # 工程文档
└── CMakeLists.txt
```

## 命名规范

### 文档命名
`【类型】_模块_版本_日期.md`
- `【GDD】_云海山庄_总纲_V2.1_20260421.md`
- `【需求】_灵茶种植系统_V1.0_20260421.md`

### 版本包命名
`【版本】_云海山庄_平台_版本号_日期.zip`
- `【发布】_云海山庄_PC_MVP_V0.1.0_20260421.zip`

### 资源文件命名
`【类型】_名称_用途_版本.png`
- `【UI】_背包面板_V1.png`
- `【作物】_灵茶_生长阶段3.png`

### 代码命名（C++）
- 类名: `PascalCase`（如 `Player`, `InventorySystem`）
- 函数名: `PascalCase`（如 `UpdatePosition`）
- 成员变量: `m_memberName` 或 `memberName_`
- 常量: `kConstantName` 或 `CONSTANT_NAME`
- 头文件: `PascalCase.hpp`（如 `Player.hpp`）

## Git 分支策略

- `main`: 稳定发布分支
- `develop`: 开发主分支
- `feature/<功能名>`: 功能分支
- `hotfix/<问题名>`: 热修复分支

**提交规范**:
```
feat: 新功能
fix: 修复 bug
refactor: 重构
docs: 文档更新
test: 测试
chore: 构建/工具
```

## AI 开发规范

1. **任何 AI 修改代码前**: 先读 `02_工程主代码/CloudSeaManor/docs/ENGINEERING_STANDARDS.md`
2. **任何新增文件**: 必须进入对应标准目录，不放根目录
3. **任何新增 .cpp/.hpp**: 检查四层落点（app/engine/domain/infrastructure），严格分类
4. **任何新增数据文件 (.csv/.json)**: 放入 `assets/data/` 对应子目录
5. **任何文档变更**: 同步更新 `CHANGELOG.md`

## SFML 依赖说明

CMake 按以下顺序查找 SFML:
1. 环境变量 `SFML_ROOT`
2. 项目同级目录 `05_参考与第三方资源/01_SFML引擎/SFML-3.0.2/`
3. 若均未找到则报错

## 快速开始

```bash
# 推荐方式：CMake Presets
cmake --preset vs2022-x64
cmake --build --preset build-debug

# 产物位置：CloudSeaManor/build/vs2022-x64/Debug/CloudSeamanor_game.exe
```

## 调试快捷键

| 快捷键 | 功能 |
|--------|------|
| W/A/S/D | 移动 |
| E | 交互 |
| F3 | 调试面板开关 |
| F5 | 切换云海调试状态 |
| F6 | 手动保存 |
| F7 | 强制大潮 |
| F9 | 读取存档 |

## 冲突解决

`docs/ENGINEERING_STANDARDS.md` > 各子文档。当规范冲突时以此文件为准。
