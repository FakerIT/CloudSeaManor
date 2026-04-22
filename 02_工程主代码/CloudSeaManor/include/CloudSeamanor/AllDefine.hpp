#pragma once

// ============================================================================
// 【AllDefine.hpp】全项目公共头文件
// ============================================================================
// 用途：
// - 所有 .cpp 文件只需 #include "CloudSeamanor/AllDefine.hpp" 即可引入全部依赖
// - 解决跨模块依赖顺序问题，所有包含顺序按拓扑排序组织
//
// 包含顺序（必须严格遵守，共11层）：
//   1. 标准库 + SFML 基础
//   2. Domain 层叶节点（无跨模块依赖）
//   3. Domain 层组合对象
//   4. Domain 层聚合系统
//   5. Domain 层跨域类型（CloudGuardianContract 等）
//   6. 基础设施层
//   7. 运行时类型（GameAppRuntimeTypes）
//   8. Engine 层领域对象
//   9. Engine 层子系统
//   10. Engine 层应用函数
//   11. 主入口
// ============================================================================

// ── 第1层：标准库 + SFML ──────────────────────────────────────────────────

// SFML 图形
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Export.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>

// SFML 窗口
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <SFML/Window/WindowHandle.hpp>

// SFML 系统
#include <SFML/System/Vector2.hpp>

// 标准库
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// ── 第2层：Domain 叶节点（无跨模块依赖）─────────────────────────────────

#include "CloudSeamanor/GameClock.hpp"            // 无跨模块依赖
#include "CloudSeamanor/RecipeData.hpp"           // 无跨模块依赖
#include "CloudSeamanor/Stamina.hpp"              // 无跨模块依赖

// ── 第3层：Domain 层组合对象 ────────────────────────────────────────────────

#include "CloudSeamanor/CropData.hpp"             // 依赖 CloudSystem（domain 内部）
#include "CloudSeamanor/Inventory.hpp"            // 依赖 CropData
#include "CloudSeamanor/PickupDrop.hpp"           // 依赖 SFML
#include "CloudSeamanor/Interactable.hpp"         // 依赖 SFML, Inventory
#include "CloudSeamanor/Player.hpp"               // 依赖 SFML

// ── 第4层：Domain 层聚合系统 ───────────────────────────────────────────────

#include "CloudSeamanor/CloudSystem.hpp"           // 无跨模块依赖
#include "CloudSeamanor/DynamicLifeSystem.hpp"     // 依赖 GameClock, CloudSystem, FestivalSystem
#include "CloudSeamanor/FestivalSystem.hpp"        // 依赖 GameClock, CloudSystem
#include "CloudSeamanor/SkillSystem.hpp"           // 依赖 CloudSystem, SkillSystem 内部类型
#include "CloudSeamanor/WorkshopSystem.hpp"       // 依赖 Inventory, RecipeData
#include "CloudSeamanor/AtmosphereState.hpp"        // 依赖 CloudSystem, GameClock

// ── 第5层：Domain 层跨域类型 ───────────────────────────────────────────────

#include "CloudSeamanor/CloudGuardianContract.hpp" // 依赖 CloudSystem（类型已就绪）

// ── 第6层：基础设施层 ────────────────────────────────────────────────────

#include "CloudSeamanor/GameConfig.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/ResourceManager.hpp"
#include "CloudSeamanor/TmxMap.hpp"
#include "CloudSeamanor/infrastructure/AtmosphereDataLoader.hpp"

// ── 第7层：运行时类型 ────────────────────────────────────────────────────

#include "CloudSeamanor/GameAppRuntimeTypes.hpp"   // 依赖领域层几乎所有类型，必须在 Engine 层之前

// ── 第8层：Engine 层领域对象 ─────────────────────────────────────────────

#include "CloudSeamanor/FarmingSystem.hpp"         // 依赖 GameAppRuntimeTypes, Inventory, SkillSystem
#include "CloudSeamanor/PickupSystem.hpp"          // 依赖 PickupDrop, Player, Inventory
#include "CloudSeamanor/LevelUpSystem.hpp"          // 依赖 SkillSystem
#include "CloudSeamanor/InteractionSystem.hpp"       // 依赖多个领域类型
#include "CloudSeamanor/TargetHintRuntime.hpp"      // 依赖 GameAppRuntimeTypes, Interactable, Inventory, WorkshopSystem
#include "CloudSeamanor/InputManager.hpp"            // 无跨模块依赖
#include "CloudSeamanor/DialogueEngine.hpp"           // 无跨模块依赖

// ── 第9层：Engine 层子系统 ───────────────────────────────────────────────

#include "CloudSeamanor/DayCycleRuntime.hpp"       // 依赖领域层多个系统
#include "CloudSeamanor/PlayerInteractRuntime.hpp"  // 依赖领域层多个类型
#include "CloudSeamanor/GameWorldState.hpp"        // 依赖领域层几乎所有类型
#include "CloudSeamanor/GameWorldSystems.hpp"      // 依赖领域层系统
#include "CloudSeamanor/GameRuntime.hpp"           // 依赖多个 Engine 子系统
#include "CloudSeamanor/engine/rendering/AtmosphereRenderer.hpp"  // 依赖 AtmosphereState

// ── 第9层后补：需要依赖 layer 8 的 domain 系统 ───────────────────────────

#include "CloudSeamanor/MainPlotSystem.hpp"        // 依赖 DialogueEngine（layer 8）

// ── 第10层：Engine 层应用函数 ─────────────────────────────────────────────

#include "CloudSeamanor/GameAppFarming.hpp"         // 依赖 CropData, GameAppRuntimeTypes, CloudSystem, SkillSystem
#include "CloudSeamanor/GameAppNpc.hpp"            // 依赖 GameAppRuntimeTypes
#include "CloudSeamanor/GameAppSpiritBeast.hpp"    // 依赖 GameAppRuntimeTypes, GameClock, Stamina
#include "CloudSeamanor/GameAppText.hpp"           // 依赖 GameAppRuntimeTypes, CloudSystem, Inventory
#include "CloudSeamanor/GameAppWeather.hpp"        // 依赖 CloudSystem, GameClock
#include "CloudSeamanor/GameAppSave.hpp"           // 依赖多个领域类型
#include "CloudSeamanor/GameAppHud.hpp"            // 依赖多个领域和引擎类型
#include "CloudSeamanor/GameAppScene.hpp"          // 依赖 GameAppRuntimeTypes, 前向声明
#include "CloudSeamanor/UISystem.hpp"              // 依赖 GameAppRuntimeTypes

// ── 第11层：主入口 ───────────────────────────────────────────────────────

#include "CloudSeamanor/GameAppState.hpp"           // 依赖几乎所有领域+引擎类型
#include "CloudSeamanor/GameApp.hpp"                // 依赖 GameAppState
#include "CloudSeamanor/CloudSeaManor.hpp"          // 聚合主头文件
