# 云海山庄 / Cloud Sea Manor

一款以云海灵气为特色的治愈系生活模拟游戏。

- 类型：生活模拟 + 轻度奇幻冒险 + 农场经营
- 平台：PC（Steam 优先）/ Nintendo Switch
- 技术栈：C++20 / SFML 3.0.2 / CMake
- 版本：0.1.0（MVP 原型阶段）

你将继承一座沉寂已久的云海山庄，种植灵茶、养育灵兽、与居民建立羁绊、守护人界与灵界的界桥——在四季流转中发现自己的归宿。

游戏循环：

`种茶 → 加工 → 探索 → 社交 → 战斗 → 存档`

## 快速开始

### 前置条件

- 编译器支持 C++20（MSVC 2022 / GCC 12+ / Clang 15+）
- CMake >= 3.23
- SFML 3.0.2（本项目已附带，位于 `05_参考与第三方资源/01_SFML引擎/SFML-3.0.2/`）

### 方式 1：CMake Presets（推荐）

```bash
cmake --preset vs2022-x64
cmake --build --preset build-debug
```

产物位置：
`02_工程主代码/CloudSeaManor/build/vs2022-x64/Debug/CloudSeamanor_game.exe`

### 方式 2：IDE 直接打开

使用 Cursor / VS Code / CLion 打开 `02_工程主代码/CloudSeaManor/`，IDE 会自动识别 `CMakePresets.json` 进行配置。

### 方式 3：手动构建

```bash
cmake -S 02_工程主代码/CloudSeaManor -B 02_工程主代码/CloudSeaManor/build
cmake --build 02_工程主代码/CloudSeaManor/build --config Debug
```

### SFML 依赖查找顺序

1. 环境变量 `SFML_ROOT`
2. 项目同级目录 `05_参考与第三方资源/01_SFML引擎/SFML-3.0.2/`
3. 若均未找到则报错

## 快捷键

| 快捷键 | 功能 |
|--------|------|
| `W/A/S/D` | 移动 |
| `E` | 交互 |
| `F3` | 调试信息面板 |
| `F5` | 切换云海状态 |
| `F6` | 手动保存 |
| `F7` | 强制大潮 |
| `F9` | 读取存档 |

## 项目结构

```
云海山庄/
├── 01_项目核心文档/          # 策划、需求、设计文档（GDD）
├── 02_工程主代码/
│   └── CloudSeaManor/       # 全部 C++ 源码、头文件、资产、测试
│       ├── src/             # 源代码（四层：app / engine / domain / infrastructure）
│       ├── include/         # 公共头文件
│       ├── assets/          # 运行时资源（data/、maps/、fonts/、audio/）
│       ├── configs/         # 运行时配置
│       ├── tests/           # 单元测试
│       └── docs/           # 工程规范与任务文档
│           ├── TASKS.md      # 综合任务清单（合并版）
│           ├── PROJECT_ROADMAP.md
│           ├── ENGINEERING_STANDARDS.md
│           ├── ITERATION_PLAN_*.md
│           ├── CLOUD_SEA_MANOR_UI_DESIGN.md
│           └── design/      # 子系统设计（战斗、灵兽等）
├── 03_参考与第三方资源/     # SFML 引擎、开发脚本
├── 04_发布与构建包/         # 版本发布包
├── 06_测试与验收/          # 测试用例、验收报告
├── 07_归档与废弃/          # 旧文档、废弃方案、早期原型
├── README.md               # 本文件
├── CLAUDE.md               # AI 开发规范
└── CHANGELOG.md           # 版本变更日志
```

## 工程规范

规范文档位于：
`02_工程主代码/CloudSeaManor/docs/ENGINEERING_STANDARDS.md`

核心规范：

- 四层架构：`app → engine → domain`，`engine → infrastructure`
- 代码落点筛选器（四问法）
- 统一命名规范（PascalCase / snake_case / UPPER_SNAKE_CASE）
- 资源必须通过 `ResourceManager` 加载
- Git 提交规范：`<type>(<scope>): <subject>`

冲突解决：`ENGINEERING_STANDARDS.md` > 各子文档

## 文档导航

>- `docs/TASKS.md` — **综合开发任务清单**（优先级、状态、路线图）
>- `docs/PROJECT_ROADMAP.md` — 项目全面提升路线图（按系统分类，含工时估算）
>- `docs/ENGINEERING_STANDARDS.md` — **工程规范总纲**（架构、命名、Git 规范）
>- `docs/ITERATION_PLAN_ARCH_REVIEW_2026Q2.md` — 当前迭代计划（2 周架构整改）
>- `docs/CLOUD_SEA_MANOR_UI_DESIGN.md` — 像素 UI 设计规范 + 资源规划
>- `docs/ART_PIPELINE_GUIDE.md` — 美术师操作手册

## 当前完成状态

- 基础移动与碰撞 ✅
- 地图加载与渲染 ✅
- 可拾取物品系统 ✅
- 背包与体力系统 ✅
- 游戏时钟与四季 ✅
- 云海天气系统 ✅
- 农业种植系统 ✅（部分）
- 技能升级系统 ✅
- NPC 日程与对话 ✅（基础）
- 灵兽跟随系统 ✅
- 节日系统 ✅（框架）
- 战斗系统 🔧 骨架完整，待接入主循环
- 存档系统 🔧 单槽位，待多槽位
- 音效系统 🔧 路由完整，待音频文件
- 像素 UI 🔧 面板框架完整，待美术资源
- 告白/婚礼系统 ❌ 未实现
- 邮件系统 ❌ 未实现

## CI / CD

GitHub Actions（`.github/workflows/`）自动执行：

- 构建测试：Windows + MSVC / Debug
- 格式检查：`clang-format --dry-run --Werror`
- 触发条件：`main` 和 `develop` 分支的 push / PR

## 贡献

请先阅读：
1. `CLAUDE.md` — AI 开发规范
2. `02_工程主代码/CloudSeaManor/docs/ENGINEERING_STANDARDS.md` — 工程规范

提交前自检：
```bash
cmake --preset vs2022-x64 && cmake --build --preset build-debug
ctest --test-dir build/vs2022-x64 --output-on-failure
```
