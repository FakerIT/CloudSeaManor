# 《云海山庄 / Cloud Sea Manor》工程规范总纲

> **版本**：3.0（整合版）
> **更新日期**：2026-04-19
> **状态**：生效中
> **定位**：项目最高级别工程治理文件，涵盖代码架构、美术资源、前端引擎、CI/CD、版本管理、Issue 治理六大领域。

## 核心原则

- **边界清晰**：每一行代码、每一个资源文件都有唯一归属。
- **改动便宜**：修改一个系统不需要理解全部系统。
- **依赖可控**：依赖方向单向，禁止循环。
- **可追溯**：每次变更可定位到 Issue 和提交。
- **简洁性**：每次变更减少md 文件的生成，优先合并 当前有的 文件内容。
---

## 1. 仓库结构

```
CloudSeaManor/                        ← 游戏主项目（唯一代码源）
├── CloudSeaManor/                   ← 主工程（CMake 项目根）
│   ├── src/
│   │   ├── app/                    ← 程序入口（main.cpp）
│   │   ├── domain/                 ← 纯玩法规则（Player、Inventory、GameClock……）
│   │   ├── engine/                 ← 运行时编排（HUD 渲染、NPC 运行时、输入……）
│   │   └── infrastructure/          ← IO 与解析（配置、日志、TmxMap、存档）
│   ├── include/CloudSeamanor/      ← 公共头文件
│   ├── assets/                     ← 运行时资源（data/、maps/、fonts/、audio/）
│   ├── configs/                    ← 运行时配置（gameplay.cfg）
│   ├── tests/                      ← 单元测试
│   └── docs/                       ← 本文件 + CHANGELOG
│
├── 项目资料/                         ← 策划与设计文档（GDD、任务卡、世界设定……）
├── scripts/                         ← 工具脚本
├── .github/                        ← CI/CD
├── CHANGELOG.md                    ← 版本变更日志
├── README.md                        ← 项目总入口（精简导航版）
└── SFML-3.0.2/                     ← SFML 本地依赖
```

**禁止入库**：第三方库二进制 / IDE 工具链 / 早期原型 / 美术源文件 / 压缩包。

---

## 2. 四层架构

```
app → engine → domain
            ↘ infrastructure
```

| 层 | 职责 | 依赖 |
|---|------|------|
| app | 程序入口、应用装配 | engine |
| engine | 主循环、输入、渲染、系统协调 | domain、infrastructure |
| domain | 纯玩法规则、稳定业务状态 | 无（仅标准库） |
| infrastructure | 配置、日志、IO、解析 | 无（仅标准库） |

**强制规则**：`infrastructure` 禁止依赖 engine；`domain` 禁止依赖 app 或 infrastructure；禁止任何循环依赖。

---

## 3. 代码落点筛选器

### 四问法

```
Step 1: 它属于哪一层？
  纯玩法规则/稳定状态 → domain/
  渲染/输入/运行时编排 → engine/
  文件IO/日志/解析 → infrastructure/
  程序入口/装配 → app/

Step 2: 它属于 engine/ 哪个子系统？
  初始化/输入/性能/消息 → engine/core/
  绘制/视觉效果 → engine/rendering/
  游戏逻辑运行时 → engine/systems/
  精灵动画 → engine/animation/
  音频播放 → engine/audio/

Step 3: 是否需要新文件？
  是 → 创建 {ClassName}.hpp + {ClassName}.cpp，命名空间 CloudSeamanor::{layer}[:{subsystem}]，更新 CMakeLists.txt
  否 → 放入已有合适文件。禁止放入 GameApp.cpp / Utils.cpp / Helper.cpp

Step 4: 自检
  □ 文件名体现单一职责  □ 头文件自包含  □ 命名空间正确
  □ 无循环依赖  □ CMakeLists.txt 已更新  □ 公共接口有注释
  □ 硬编码值已提取为配置或常量
```

### 具体落点对照（重点）

| 代码内容 | 正确落点 | 禁止落点 |
|---------|---------|---------|
| 作物品质计算公式 | `domain/farming/` | `GameApp.cpp` |
| HUD 文本刷新 | `engine/rendering/HudRenderer.cpp` | `GameAppHud.cpp` |
| JSON 配置解析 | `infrastructure/data/` | `domain/` |
| 键盘按键映射 | `engine/core/InputManager.cpp` | `GameApp.cpp` |
| 存档序列化 | `infrastructure/SaveGameService.cpp` | `GameAppSave.cpp` |
| 纹理加载缓存 | `infrastructure/ResourceManager.cpp` | `GameAppScene.cpp` |
| NPC 好感度模型 | `domain/social/` | `GameAppNpc.cpp` |

### GameApp.cpp 瘦身规则

| 禁止 | 正确落点 |
|------|---------|
| 茶田成长逻辑 | `engine/systems/FarmingRuntime.cpp` |
| NPC 日程逻辑 | `engine/systems/NpcRuntime.cpp` |
| HUD 渲染逻辑 | `engine/rendering/HudRenderer.cpp` |
| 存档读写逻辑 | `infrastructure/SaveGameService.cpp` |
| 输入映射逻辑 | `engine/core/InputManager.cpp` |

---

## 4. 命名规范

| 类型 | 规则 | 示例 |
|------|------|------|
| Class / Struct | PascalCase，名词 | `Player`、`CloudSystem` |
| Function | PascalCase，动词短语 | `Update()`、`GetHealth()` |
| Variable | snake_case | `player_count`、`movement_speed` |
| Member variable | snake_case，可选 `_` 后缀 | `health_`、`max_health` |
| Constant | UPPER_SNAKE_CASE | `MAX_HEALTH` |
| Enum class 值 | PascalCase | `CloudState::Clear` |
| Namespace | snake_case | `domain`、`engine` |
| 文件名 | PascalCase | `Player.hpp` |

**禁止文件名**：`Utils.*` `Helper.*` `Common.*` `Temp.*` `Misc.*`

---

## 5. 头文件规则

- Public headers 在 `include/CloudSeamanor/`
- **必须自包含**
- 优先前向声明而非 `#include`
- **禁止**在头文件中使用 `using namespace`
- **禁止**在头文件中放置大量实现
- 内部辅助用 `.cpp` 匿名命名空间

**包含顺序**：1. 对应头文件 → 2. 项目头文件（domain→engine→infra）→ 3. 第三方 → 4. 标准库（字母序）

---

## 6. 注释规范

### 文件头（必须）

```cpp
// ============================================================================
// 【FileName】Brief description
// ============================================================================
// Responsibilities:
// - Responsibility 1
// ============================================================================
```

### 函数文档（公共函数必须）

```cpp
/**
 * @brief Brief function description
 * @param name Parameter description
 * @return Return value description
 * @note Important notes
 */
```

### 行内注释 — 解释**为什么**，不解释**是什么**

```cpp
// Good
if (health <= 0) {
    // Player died, trigger respawn flow
    Respawn();
}

// Bad
// If health is less than or equal to zero
if (health <= 0) {
    Respawn();
}
```

---

## 7. Git 提交规范

```
<type>(<scope>): <subject>
type: feat | fix | perf | refactor | docs | test | chore | assets
```

| 类型 | 用途 | 示例 |
|------|------|------|
| `feat` | 新功能 | `feat(npc): add NPC schedule system` |
| `fix` | Bug 修复 | `fix(render): fix tile rendering glitch` |
| `perf` | 性能优化 | `perf(hud): cache HUD text updates` |
| `refactor` | 重构 | `refactor(inventory): simplify item lookup` |
| `docs` | 文档更新 | `docs(arch): update standards` |
| `test` | 测试更新 | `test(player): add movement tests` |
| `chore` | 构建/工具 | `chore(cmake): update CMake config` |
| `assets` | 美术/音频 | `assets(art): add player spritesheet` |

**分支策略**：`main`（稳定版）← `develop`（集成分支）← `feat/`、`fix/`（从 develop 拉出）

---

## 8. 美术资源规范

### 目录结构

```
assets/
├── textures/               # 纹理（characters/、tiles/、items/、ui/、effects/）
├── audio/                  # 音频（bgm/.ogg、sfx/.wav、ambient/.ogg）
├── fonts/                 # 字体（TTF/OTF）
├── data/                  # 游戏数据（CSV/JSON）
├── maps/                  # TMX 地图
├── shaders/               # 着色器
└── animations/            # 动画定义 JSON
```

### 格式标准

| 资源类型 | 格式 | 透明通道 | 压缩 |
|---------|------|---------|------|
| 角色/瓦片精灵图 | PNG | 必需 | 无损 |
| 物品图标 | PNG | 必需 | 无损 |
| UI面板 | PNG (9-slice) | 必需 | 无损 |
| 大背景图 | JPG | 不需要 | 有损85% |
| BGM | OGG Vorbis | N/A | q=5 |
| SFX | WAV | N/A | 无损 |

**禁止**：BMP 格式（无压缩）、MP3（专利风险，用 OGG）

---

## 9. 新增子系统设计规范

### ResourceManager（强制，infrastructure 层）

```cpp
namespace CloudSeamanor::infrastructure {
class ResourceManager {
public:
    static ResourceManager& Instance();
    void Initialize(const std::filesystem::path& assets_root);
    void Shutdown();

    const sf::Texture& GetTexture(const std::string& asset_id);
    const sf::Font& GetFont(const std::string& asset_id);
    const sf::SoundBuffer& GetSoundBuffer(const std::string& asset_id);

    void PreloadGroup(const std::string& group_name);
    void UnloadGroup(const std::string& group_name);
    void UnloadUnused();

    [[nodiscard]] size_t TextureMemoryUsage() const noexcept;

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts_;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> sounds_;
};
}
```

**强制规则**：所有资源必须通过 ResourceManager 加载（禁止直接 `loadFromFile`）；重复加载返回缓存；缺失资源返回默认占位而非崩溃；**禁止**在 Update/Render 中调用 Load。

### InputManager（强制，engine 层）

```cpp
namespace CloudSeamanor::engine {
enum class Action {
    MoveUp, MoveDown, MoveLeft, MoveRight,
    Interact, OpenInventory, Cancel, Confirm,
    Sprint, UseTool, OpenMenu,
    DebugToggle, QuickSave, QuickLoad,
    Count
};

class InputManager {
public:
    bool IsActionPressed(Action action) const;
    bool IsActionJustPressed(Action action) const;  // 本帧刚按下
    sf::Vector2f GetMovementVector() const;        // 归一化

    void BeginNewFrame();  // 每帧必须调用
    void HandleEvent(const sf::Event& event);
};
}
```

**强制规则**：WASD + 方向键同时可用；`IsJustPressed` 仅返回 true 一帧；移动向量必须归一化；可通过配置文件覆盖默认键位。

### UISystem（engine 层）

- 命名空间：`CloudSeamanor::engine`
- 职责：初始化所有 UI 面板、更新 UI 文本和进度条、渲染所有 UI 层
- **禁止**：实现游戏逻辑、直接修改领域对象
- 强制使用**脏标记**避免无变化时刷新

### EventBus（engine 层）

```cpp
namespace CloudSeamanor::engine {
struct Event {
    std::string type;
    std::unordered_map<std::string, std::string> data;
};
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;
    void Subscribe(const std::string& event_type, Handler handler);
    void Emit(const Event& event);
    void Clear();
};
}
```

用途：解耦 PlayerInteractRuntimeContext 的 20+ 参数传递。

---

## 10. 状态管理规范

### GameWorldState 拆分规则

1. **新字段必须归入已有子结构体**，禁止直接挂在 GameWorldState 上
2. **新子结构体职责单一**（WorldConfig / InteractionState / TutorialState 分开）
3. **SFML 渲染对象使用 `std::unique_ptr`**
4. 存档字段与运行时临时字段用注释明确分隔

### GameAppState 迁移规则

- 新代码**必须**使用 GameWorldState，**禁止**使用 GameAppState
- GameAppState 仅作为过渡期兼容层，**禁止**添加新字段

### 状态访问器规则

```cpp
// ✅ 正确：通过访问器
[[nodiscard]] Player& GetPlayer() { return player_; }

// ❌ 错误：公开成员
public: Player player;  // 禁止
```

---

## 11. 场景生命周期规范

| 分类 | 特征 | 示例 |
|------|------|------|
| 全屏场景 | 替换栈顶，停止更新 | MainMenu、GameWorld |
| 暂停覆盖层 | 压入栈顶，暂停游戏 | Inventory、Settings |
| 透明覆盖层 | 压入栈顶，游戏继续 | Dialogue、Notification |

```
场景创建 → OnEnter() → [活跃] → OnExit() → 场景销毁
                          ↑↓
                    OnPause() / OnResume()
```

---

## 12. 数据驱动规范

| 数据特征 | 格式 | 原因 |
|---------|------|------|
| 表格型（作物、配方、日程） | CSV | 简单、Excel 可编辑 |
| 嵌套型（对话脚本、礼物偏好） | JSON | 支持层级 |
| 配置型（键值对） | gameplay.cfg | 当前已使用 |

**规则**：数据加载必须在初始化阶段完成（禁止在 Update/Render 中加载）；加载失败必须有默认值兜底。

### 12.1 统一表注册规范

- 所有新表必须通过 `infrastructure::DataRegistry` 注册，不允许每个系统各写一套 CSV 解析循环。
- `ResourceManager` 负责解析数据根目录与文本资源读取；`DataRegistry` 负责 CSV/JSON 解析、字段转换、校验与 typed table 装配。
- `domain` 只消费结构化数据，不直接依赖文件路径或 `ifstream`。
- 每张表至少定义：
  - 唯一主键列 `Id`（或兼容旧表的显式唯一字段）
  - 必填字段检查
  - 重复 ID 检查
  - 外键/引用字段检查
- 新表推荐目录：
  - `assets/data/npc/`
  - `assets/data/pet/`
  - `assets/data/tea/`
  - `assets/data/skills/`
  - `assets/data/weapons/`
  - `assets/data/festival/`
  - `assets/data/diary/`
  - `assets/data/tool/`

### 12.2 字段命名与编码

- 列名统一使用 `PascalCase`
- 文本字段允许直接保存中文
- 一对多字段优先用 JSON 数组字符串，例如 `["tea_a","tea_b"]`
- 资源/实体引用统一使用全局唯一字符串 ID，不使用显示名做关联

### 12.3 兼容策略

- 旧系统迁移期间允许保留 built-in fallback，但 fallback 仅作为开发兜底，不得继续扩展为主数据源。
- 任何新功能优先补表，不允许继续往 `GameConstants.hpp`、大 `switch` 或静态数组里堆内容配置。

---

## 13. 测试规范

```
tests/
├── TestRunner.cpp
├── TestFramework.hpp
├── domain/     ← 80% 覆盖率目标
│   ├── CropDataTest.cpp
│   ├── GameClockTest.cpp
│   ├── InventoryTest.cpp
│   └── StaminaTest.cpp
├── engine/     ← 30% 覆盖率
└── infrastructure/  ← 60% 覆盖率
```

**最低测试要求（P0）**：Player×5、Inventory×4、GameClock×3、Stamina×4、CropData×3

---

## 14. 性能规范

| 指标 | 目标值 |
|------|--------|
| FPS | ≥ 60（中端 PC） |
| 帧时间 | ≤ 16.67ms |
| 纹理内存 | < 300MB |
| 启动时间 | < 5 秒 |

**禁止**在 `Update()`/`Render()` 中：分配内存 / 在大 map 中查找。热路径函数必须添加 `PROFILE_SCOPE` 宏。

---

## 15. CI/CD

| Job | 触发条件 | 执行内容 |
|-----|---------|---------|
| `build-windows` | push/PR 到 main/develop | CMake 配置 + Debug 构建 |
| `format-check` | push/PR 到 main/develop | clang-format 格式检查 |

**提交前自检**：

```bash
clang-format -i src/**/*.hpp src/**/*.cpp
cmake --preset vs2022-x64 && cmake --build --preset build-debug
ctest --test-dir build/vs2022-x64 --output-on-failure
```

---

## 16. Issue 标签体系

| 标签 | 含义 |
|------|------|
| `P0-critical` | 阻塞性问题，立即修复 |
| `P1-high` | 本迭代必须完成 |
| `P2-medium` | 计划内但可延后 |
| `P3-low` | 有空再做 |
| `layer:domain` | 领域层 |
| `layer:engine` | 引擎层 |
| `layer:infra` | 基础设施层 |
| `breaking` | 破坏性变更 |
| `tech-debt` | 技术债务 |

---

## 17. 版本管理

```
MAJOR.MINOR.PATCH
MAJOR: 不兼容的 API 变更
MINOR: 向后兼容的功能新增
PATCH: 向后兼容的问题修复
```

| 版本 | 里程碑 | 状态 |
|------|--------|------|
| 0.1.0 | 基础原型 | ✅ 已完成 |
| 0.2.0 | MVP（GameApp 瘦身 + ResourceManager） | 进行中 |
| 0.3.0 | 美术集成（精灵图/瓦片集/AudioManager） | 规划 |
| 0.5.0 | Demo 可玩演示 | 规划 |
| 1.0.0 | Early Access 发布 | 规划 |

---

## 18. 四阶段路线图

| 阶段 | 时间 | 目标 |
|------|------|------|
| Phase 1: 工程治理 | 1-2 周 | 仓库清理 + CI/CD + 规范落地 |
| Phase 2: 架构瘦身 | 2-3 周 | GameApp 拆分 + InputManager + ResourceManager + EventBus |
| Phase 3: 美术集成 | 4-6 周 | SpriteAnimator + 瓦片集 + UI 素材 + AudioManager |
| Phase 4: 内容填充 | 6-8 周 | 完整 NPC、节日、灵兽、工坊、存档系统 |

---

## 19. 禁止事项清单

| 编号 | 禁止事项 |
|------|---------|
| A1 | 向 GameAppState 添加新字段（已废弃） |
| A2 | 在 GameApp.cpp 中添加业务逻辑 |
| A3 | 在 Update/Render 中加载资源 |
| A4 | 跨层直接依赖（domain → engine） |
| A5 | 新增 Utils/Helper/Common 文件 |
| A6 | 在头文件中使用 `using namespace` |
| R1 | 提交大于 10MB 的二进制文件 |
| R2 | 使用 BMP 格式 |
| R3 | 使用 MP3 格式（专利风险） |
| R4 | 硬编码资源路径 |
| R5 | 重复加载同一资源 |

---

## 20. 新增模块 Checklist

```
□ 1. 公共接口还是内部实现？
□ 2. 属于 app / engine / domain / infrastructure 哪一层？
□ 3. engine/ 下属于 core / rendering / systems / animation / audio 哪个子系统？
□ 4. 是否有更准确的目录可复用？
□ 5. 名字是否足够具体？（禁止 Utils/Helper/Common/Temp/Misc）
□ 6. 能否避免新增跨层依赖？
□ 7. 头文件是否自包含？
□ 8. 是否加入 CMakeLists.txt？
□ 9. 公共接口是否有注释？
□ 10. 硬编码值是否提取为配置或常量？
□ 11. 是否需要编写测试？
□ 12. CHANGELOG.md 是否更新？
□ 13. 是否有重复的 MD 文档需要同步更新？
```

---

## 1. 架构与分层（强制）

依赖方向固定为：

`app -> engine -> domain`  
`engine -> infrastructure`

强制规则：

- `domain`：仅允许标准库，不允许依赖 SFML、`engine`、`infrastructure`、`app`
- `infrastructure`：仅允许标准库与第三方基础库，不允许依赖 `engine`
- `engine`：负责运行时编排，可依赖 `domain` 与 `infrastructure`
- `app`：只做入口装配与生命周期管理，不承载业务规则
- 禁止循环依赖

落点判定四问：

1. 这是玩法规则还是运行时流程？
2. 是否涉及 IO/配置/资源加载？
3. 是否只是入口装配？
4. 是否会引入反向依赖？

## 2. 命名规范（统一风格）

- 类/结构体：`PascalCase`
- 函数：`PascalCase`
- 命名空间：`snake_case`
- 成员变量：`snake_case`（可选 `_` 后缀，如 `health_`）
- 普通变量：`snake_case`
- 常量：`UPPER_SNAKE_CASE`
- 文件名：`PascalCase.hpp/.cpp`

禁止命名：

- `Utils.*`
- `Helper.*`
- `Common.*`
- `Temp.*`
- `Misc.*`

## 3. 资源与内存管理（强制）

- 所有纹理/字体/音频统一由 `infrastructure::ResourceManager` 管理
- 禁止在 `Update()` / `Render()` 中调用加载接口
- 资源缺失必须有占位兜底与日志，不允许直接崩溃
- 所有拥有关系优先 `std::unique_ptr` 或值语义，禁止裸 `new/delete`
- 需要缓存回收时，必须有可验证的引用语义（`Acquire/Release` 或等价机制）

## 4. SFML 3.0.2 使用规范

- 事件循环使用 `while (const auto ev = window.pollEvent())`
- 事件分发优先 `event.getIf<...>()` / `event.is<...>()`
- 字体加载使用 `sf::Font::openFromFile`
- 窗口状态、视图、坐标变换必须与 SFML 3 API 对齐，不混用旧版调用方式
- SFML 渲染对象（`sf::Shape`/`sf::Text`/`sf::Sprite`）不进入 `domain`

## 5. 可重构性要求

- `GameApp` 保持瘦身：仅保留装配、主循环、场景切换
- UI 视图数据组装下沉到 `engine/systems` 或 `engine/rendering`
- 避免“大函数 + 大分支”；单函数优先单职责
- 硬编码路径、硬编码魔法数应抽取为配置或常量

## 6. 注释与异常处理

- 每个公共头文件需具备职责说明
- 公共函数需具备 `@brief/@param/@return` 注释
- 行内注释解释“为什么”，不解释“是什么”
- 任何外部输入解析（配置、CSV、JSON）必须可失败且可恢复：
  - 记录日志
  - 提供默认值
  - 不因单条坏数据导致主流程退出

## 7. 提交前检查清单（开发自检）

- [ ] 新增/修改代码层级符合 `app/engine/domain/infrastructure`
- [ ] `domain` 未引入 SFML 或跨层依赖
- [ ] 无 `Update/Render` 内资源加载
- [ ] 命名符合统一规则
- [ ] 公共接口有注释，异常路径有处理
- [ ] 关键改动有对应测试或手工验证记录

**最后更新**：2026-04-22
