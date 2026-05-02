# 贡献指南

> 欢迎参与《云海山庄》的开发！本指南将帮助你快速上手。

## 目录

- [开发环境准备](#开发环境准备)
- [代码规范](#代码规范)
- [Git 工作流](#git-工作流)
- [提交规范](#提交规范)
- [PR 创建流程](#pr-创建流程)
- [测试指南](#测试指南)
- [常见问题](#常见问题)

---

## 开发环境准备

### 前置条件

- **操作系统**：Windows 10/11（推荐）、Linux、macOS
- **编译器**：MSVC 2022 / GCC 12+ / Clang 15+
- **构建工具**：CMake >= 3.23
- **图形库**：SFML 3.0.2（项目已附带）

### 快速开始

```bash
# 1. 克隆仓库
git clone https://github.com/your-repo/CloudSeaManor.git
cd CloudSeaManor

# 2. 配置 CMake（推荐方式）
cmake --preset vs2022-x64

# 3. 构建 Debug 版本
cmake --build --preset build-debug

# 4. 运行游戏
./02_工程主代码/CloudSeaManor/build/vs2022-x64/Debug/CloudSeamanor_game.exe

# 5. 运行测试
ctest --test-dir 02_工程主代码/CloudSeaManor/build/vs2022-x64 --output-on-failure
```

### IDE 配置

**VS Code**
```json
// .vscode/settings.json
{
    "cmake.configureOnOpen": true,
    "cmake.buildDirectory": "${workspaceFolder}/02_工程主代码/CloudSeaManor/build",
    "files.associations": {
        "*.hpp": "cpp",
        "*.cpp": "cpp"
    }
}
```

**CLion**
- 直接打开 `02_工程主代码/CloudSeaManor/` 目录
- CMake 会自动识别 `CMakePresets.json`

---

## 代码规范

### 必读文档

1. [CLAUDE.md](../../CLAUDE.md) - AI 开发规范
2. [ENGINEERING_STANDARDS.md](../docs/ENGINEERING_STANDARDS.md) - 工程规范总纲

### 四层架构

```
app → engine → domain
            ↘ infrastructure
```

| 层级 | 职责 | 依赖 |
|------|------|------|
| `app` | 程序入口、应用装配 | engine |
| `engine` | 主循环、输入、渲染、系统协调 | domain、infrastructure |
| `domain` | 纯玩法规则、稳定业务状态 | 无（仅标准库） |
| `infrastructure` | 配置、日志、IO、解析 | 无（仅标准库） |

### 命名规范

| 类型 | 规则 | 示例 |
|------|------|------|
| 类/结构体 | PascalCase | `Player`、`CloudSystem` |
| 函数 | PascalCase | `Update()`、`GetHealth()` |
| 命名空间 | snake_case | `domain`、`engine` |
| 成员变量 | snake_case | `health_`、`max_health` |
| 普通变量 | snake_case | `player_count` |
| 常量 | UPPER_SNAKE_CASE | `MAX_HEALTH` |
| 文件名 | PascalCase | `Player.hpp` |

### 禁止事项

- ❌ 向 `GameApp.cpp` 添加业务逻辑
- ❌ 在 `Update/Render` 中加载资源
- ❌ `domain` 层使用 SFML
- ❌ 创建 `Utils.*`、`Helper.*`、`Common.*` 文件
- ❌ 跨层直接依赖

---

## Git 工作流

### 分支策略

```
main        ← 稳定发布分支
  ↑
develop     ← 开发主分支
  ↑
feature/*   ← 功能分支（从 develop 拉出）
  ↑
fix/*       ← 修复分支
  ↑
hotfix/*    ← 热修复分支（从 main 拉出）
```

### 分支命名

```bash
# 功能分支
git checkout -b feature/npc-dialogue-expansion

# Bug 修复
git checkout -b fix/crop-growth-calculator

# 热修复
git checkout -b hotfix/steam-launch-crash
```

---

## 提交规范

### 格式

```
<type>(<scope>): <subject>

[optional body]

[optional footer]
```

### Type 类型

| 类型 | 说明 | 示例 |
|------|------|------|
| `feat` | 新功能 | `feat(npc): add NPC dialogue expansion` |
| `fix` | Bug 修复 | `fix(crop): fix growth calculation overflow` |
| `perf` | 性能优化 | `perf(hud): cache text updates` |
| `refactor` | 重构 | `refactor(inventory): simplify item lookup` |
| `docs` | 文档更新 | `docs(arch): update standards` |
| `test` | 测试更新 | `test(player): add movement tests` |
| `chore` | 构建/工具 | `chore(cmake): update config` |
| `assets` | 美术/音频 | `assets(art): add player spritesheet` |

### 示例

```bash
# 好的提交
git commit -m "feat(farming): add crop quality modifier from cloud density

- Add CloudDensityQualityBonus to CropData
- Update growth calculation to include cloud factor
- Add unit tests for quality calculation

Closes #123"

# 不好的提交
git commit -m "fix stuff"
git commit -m "update"
git commit -m "WIP"
```

---

## PR 创建流程

### 1. 确保代码质量

```bash
# 格式化代码
clang-format -i src/**/*.hpp src/**/*.cpp

# 本地构建
cmake --preset vs2022-x64
cmake --build --preset build-debug

# 运行测试
ctest --test-dir 02_工程主代码/CloudSeaManor/build/vs2022-x64 --output-on-failure
```

### 2. 创建 PR

1. Push 分支到远程
2. 在 GitHub 创建 Pull Request
3. 填写 PR 描述（使用模板）
4. 关联相关 Issue
5. 等待 Code Review

### 3. PR 检查清单

- [ ] 代码符合四层架构
- [ ] `domain` 层无 SFML 依赖
- [ ] 无 `Update/Render` 内资源加载
- [ ] 命名符合规范
- [ ] 公共接口有注释
- [ ] 有对应的测试
- [ ] CHANGELOG.md 已更新

---

## 测试指南

### 测试结构

```
tests/
├── TestRunner.cpp
├── TestFramework.hpp
├── domain/     ← 80% 覆盖率目标
│   ├── CropDataTest.cpp
│   ├── GameClockTest.cpp
│   └── ...
├── engine/     ← 30% 覆盖率
└── infrastructure/  ← 60% 覆盖率
```

### 运行测试

```bash
# 运行所有测试
ctest --test-dir 02_工程主代码/CloudSeaManor/build/vs2022-x64 --output-on-failure

# 运行特定测试
./build/vs2022-x64/Debug/CloudSeamanor_tests.exe "[CropData]"

# 运行带覆盖率的测试
cmake --preset vs2022-x64-tests
cmake --build --preset build-debug-tests
ctest --test-dir 02_工程主代码/CloudSeaManor/build/vs2022-x64 --output-on-failure
```

### 编写测试

```cpp
#include <catch2/catch.hpp>
#include "CloudSeamanor/domain/farming/CropData.hpp"

TEST_CASE("CropData quality calculation", "[CropData]") {
    CropData crop;
    crop.quality_ = CropQuality::Fine;
    crop.base_value_ = 100;

    auto result = CalculateFinalPrice(crop, CloudDensity::Dense);

    REQUIRE(result > crop.base_value_);  // 浓云应该提升价格
}
```

### 最低测试要求（P0）

| 模块 | 测试数 |
|------|--------|
| Player | 5 |
| Inventory | 4 |
| GameClock | 3 |
| Stamina | 4 |
| CropData | 3 |

---

## 常见问题

### Q: SFML_ROOT 环境变量如何设置？

```powershell
# Windows PowerShell
$env:SFML_ROOT = "D:\CloudSeaManor\05_参考与第三方资源\01_SFML引擎\SFML-3.0.2"
```

### Q: 构建失败怎么办？

1. 清理构建目录：`rm -rf build/`
2. 重新配置：`cmake --preset vs2022-x64`
3. 检查 SFML 路径是否正确

### Q: 测试找不到？

确保构建了测试目标：
```bash
cmake --build --preset build-debug --target CloudSeamanor_tests
```

### Q: 如何报告 Bug？

请使用 [Bug Report 模板](../../.github/ISSUE_TEMPLATE/bug_report.md) 创建 Issue。

---

## 联系方式

- **Issue**: https://github.com/your-repo/CloudSeaManor/issues
- **讨论**: https://github.com/your-repo/CloudSeaManor/discussions

---

> 感谢你的贡献！每一个 PR 都让《云海山庄》变得更好。
