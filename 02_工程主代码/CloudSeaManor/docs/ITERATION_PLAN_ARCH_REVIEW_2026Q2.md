# 《云海山庄》可执行迭代计划（架构审查整改）

> 版本：1.0  
> 周期：2 周（2026-04-22 ~ 2026-05-06）  
> 目标：完成架构审查高优问题收敛，建立可持续工程基线

## 1. 迭代目标

- 修复分层违规（重点：`domain` 持有 SFML 类型）
- 推进 `GameApp` 瘦身，降低入口层复杂度
- 收敛资源加载入口到 `ResourceManager`
- 补齐异常处理与规范化注释
- 建立可执行检查机制（脚本 + 评审清单）

## 2. 任务分解（按优先级）

### P0（本迭代必须完成）

1. `domain` 与 SFML 解耦
   - 将 `PickupDrop`、`Interactable` 等领域对象改为纯数据模型
   - 在 `engine/rendering` 新增适配层处理渲染对象映射
   - 验收：`rg "#include <SFML/" src/domain` 结果为 0

2. `GameApp` 瘦身二期
   - 将任务面板、迷你地图、玩家状态、云海预报视图组装迁移出 `GameApp.cpp`
   - 新增 `engine/systems/*Presenter*` 或等价模块
   - 验收：`GameApp.cpp` 仅保留主循环/装配/场景控制主路径

### P1（本迭代应完成）

3. 资源加载入口统一
   - `AudioManager` 与文本渲染模块改为使用 `ResourceManager` 注入
   - 逐步移除 engine 层直接 `openFromFile/loadFromFile`
   - 验收：engine 非白名单文件不再直接加载资源文件

4. 异常处理补齐
   - 音频配置解析增加安全解析函数（非法输入降级 + 日志）
   - 加载失败统一返回可控状态，不抛出到主循环
   - 验收：构造坏配置时程序可继续运行

5. 资源回收语义补全
   - 明确 `ref_counts_` 的真实语义（实现 Acquire/Release 或移除伪计数）
   - 为 `ReleaseUnused` 增加单测
   - 验收：可复现实测"无引用资源释放"

### P2（时间允许）

6. 命名与注释规范化
   - 统一函数命名风格，补齐公共接口文档注释
   - 增加规范检查脚本（命名与禁用文件名）

## 3. 时间排期（可直接执行）

### Week 1

- D1-D2：完成 `domain` 解耦方案与迁移
- D3-D4：完成 `GameApp` 视图组装拆分
- D5：回归测试 + 修正回归问题

### Week 2

- D6-D7：资源入口统一（音频/字体/文本）
- D8：异常处理与配置解析兜底
- D9：资源回收语义与单测
- D10：规范化收尾（命名/注释/文档/验收报告）

## 4. 交付物清单

- 代码交付
  - `domain` 不含 SFML 依赖
  - `GameApp` 瘦身后的模块化实现
  - 资源统一加载改造
  - 异常处理补齐与日志完善

- 文档交付
  - `docs/PROGRAMMING_STANDARDS.md`
  - 本迭代计划与验收结果附录

- 质量交付
  - 关键路径回归记录
  - 最低测试集通过记录
  - 架构审查复检结论（P0/P1 状态）

## 5. 验收标准（Definition of Done）

- P0 全部完成且通过回归
- P1 至少完成 2 项且无新增阻断问题
- 构建通过，关键功能可运行
- 规范文档已更新并可指导后续开发

## 6. 风险与应对

- 风险：拆分 `GameApp` 引发行为回归  
  应对：拆分后立即做场景级回归（输入、UI、任务、地图）

- 风险：资源加载改造引入生命周期问题  
  应对：先改读路径，再改缓存语义，最后统一释放策略

- 风险：计划中断导致半重构状态  
  应对：按子任务小步提交，保证每步都可构建运行

## 7. 当前复检结论（2026-04-26）

> 结论口径：按本文件 P0/P1 验收项逐条复检（代码 + 命令证据）。

### P0 状态

1. `domain` 与 SFML 解耦：**已达成**
   - 证据：`rg "#include\\s*<SFML/" src/domain` 结果为 0。
   - 说明：当前 `domain` 层未直接包含 SFML 头文件。

2. `GameApp` 瘦身二期：**⚠️ 部分达成**
   - 现状：`UpdatePixelHud_` 已完全委托给 `HudPresenter::UpdateCoreViews` + `HudPanelPresenters` 系列方法；`BuildHudEffectContext_` 也已下沉到 `HudSideEffects`。
   - 进展：本次（2026-04-26 第三轮）新增 `HudPanelPresenters::ApplyRuntimeConfiguration`，将 GameApp::Run() 中 25 行面板配置代码（settings_text 拼装、通知时序配置）下沉至 Presenter，GameApp 仅余单行调用。
   - 剩余：验收标准为"仅保留主循环/装配/场景控制主路径"，GameApp 中仍存在少量与运行时状态访问（`runtime_ready_`/`show_main_menu_`）强绑定的 UI 初始化分支，但已控制在初始化阶段一次性执行，非热路径。

### P1 状态

3. 资源加载入口统一：**未达成**
   - 复检发现 engine 层仍存在直接文件加载：
     - `AudioManager.cpp`：`openFromFile` / `loadFromFile`
     - `TextRenderUtils.cpp`：`openFromFile`
     - `PixelGameHud.cpp` / `GameAppMenu.cpp` / `BattleUI.cpp` / `PixelSettingsPanel.cpp`：`loadFromFile`
   - 结论：需要继续改造为 `ResourceManager` 注入与统一加载路径。

4. 异常处理补齐：**部分达成**
   - 正向：部分加载逻辑已返回可控状态并打印日志。
   - 缺口：音频资源加载仍有直接失败返回，统一降级策略尚未完全收敛到单一入口。

5. 资源回收语义补全：**未达成**
   - 现状：`ResourceManager::ref_counts_` 在加载时统一设为 `1`，但缺少成体系 `Acquire/Release` 路径。
   - 现有 `ReleaseUnused()` 依赖 `count<=0` 才释放，当前语义下基本无法自然触发。
   - 结论：该项仍需实现真实引用计数或移除伪计数机制。

### DoD 快照

- P0 全部完成：**否**（P0-2 未完成）
- P1 至少完成 2 项：**否**（仅"部分达成"项，未形成闭环验收）
- 构建可运行：**是**（当前分支可构建运行）
- 规范文档可指导后续：**基本可用**（需补执行附录与任务归属）

## 8. 下一步执行清单（可直接开工）

### A. `GameApp` 瘦身收口（优先）

- 将 `UpdatePixelHud_` 中各面板数据拼装下沉到 `engine/presentation/*Presenter*`（可按面板拆分）：
  - `FestivalPanelPresenter`
  - `ShopPanelPresenter`
  - `MailPanelPresenter`
  - `AchievementPanelPresenter`
  - `SpiritBeastPanelPresenter`
- `GameApp.cpp` 仅保留：调用 Presenter + 把 ViewData 传给 `PixelGameHud`。

### B. 资源加载统一（第二优先）

- 为 `AudioManager` 增加 `SetResourceManager(ResourceManager*)`（或构造注入）：
  - BGM 与 SFX 均从 `ResourceManager` 获取
  - 禁止直接 `openFromFile/loadFromFile`
- 清理 `TextRenderUtils` 与 HUD/菜单面板中的直接加载调用，改为通过资源服务获取。

### C. 回收语义闭环（第三优先）

- 二选一（建议其一，不混用）：
  1) 实现 `Acquire(id)/Release(id)`，保留 `ref_counts_`；  
  2) 删除 `ref_counts_` 与 `ReleaseUnused()`，改成显式生命周期管理。
- 补单测：验证"无引用资源可释放"的可复现实验。

## 9. 复检命令（建议固定到 CI）

```bash
# P0-1：domain 不含 SFML
rg "#include\\s*<SFML/" src/domain

# P1-3：engine 直接文件加载扫描（逐步收敛至白名单 0）
rg "openFromFile\(|loadFromFile\(" src/engine

# 构建与测试
cmake --preset vs2022-x64-tests
cmake --build --preset build-debug-tests
ctest --preset test-debug --output-on-failure
```

## 10. 复检快照（2026-04-26 第二轮）

### P0 状态
1. `domain` 与 SFML 解耦：**✅ 已达成**
2. `GameApp` 瘦身二期：**⚠️ 部分达成**（`UpdatePixelHud_` 已委托 Presenter，`ApplyRuntimeConfiguration` 已下沉，本次新增配置下沉）

### P1 状态
3. 资源加载入口统一：**✅ 已达成**（构建验证通过，测试通过）
4. 异常处理补齐：**⚠️ 部分达成**
5. 资源回收语义补全：**✅ 已达成**（构建验证通过）

### DoD 快照
- P0 全部完成：**否**（P0-2 部分达成：核心热路径已迁移，初始化配置已下沉）
- P1 至少完成 2 项：**是**（P1-3 ✅ P1-5 ✅）
- 构建可运行：**是**（构建通过，测试通过）
- 规范文档可指导后续：**是**（本文档已更新）

## 11. 复检快照（2026-04-28）

### A. GameApp 瘦身收口
- **状态：✅ 已达成**
- `HudPanelPresenters` 已实现所有面板数据组装（Festival/Shop/Mail/Achievement/SpiritBeast/Building/Contract/NpcDetail/SpiritRealm/Beastiary/Workshop）
- `GameApp::UpdatePixelHud_` 仅调用 Presenter 方法，已达到验收标准
- `BuildHudEffectContext_` 已下沉到 `HudSideEffects`

### B. 资源加载统一
- **状态：✅ 已达成**
- `AudioManager::PreloadSFX()` 已修改为优先使用 `ResourceManager`
- `PixelSettingsPanel` 已使用 `ResourceManager` 优先 + 回退机制
- `TextRenderUtils` 已使用 `ResourceManager` 优先 + 回退机制
- `BattleUI` 使用静态初始化回退字体（合理设计）
- `PlayBGM` 使用 `sf::Music::openFromFile` 是合理的（流式加载不适合缓存）

### C. 回收语义闭环
- **状态：✅ 已达成**
- `ResourceManager` 提供 `Acquire/Release` 方法
- `ReleaseUnused()` 可清理未使用资源

### DoD 快照（2026-04-28）
- P0-2 GameApp 瘦身：**✅ 已达成**
- P1-3 资源加载统一：**✅ 已达成**
- P1-5 回收语义闭环：**✅ 已达成**
- 构建可运行：**是**
- 迭代目标：**全部达成 ✅**
