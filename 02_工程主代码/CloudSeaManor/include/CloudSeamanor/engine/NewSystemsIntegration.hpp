#pragma once

// ============================================================================
// 【NewSystemsIntegration】新功能系统集成
// ============================================================================
//
// 本文件负责将以下新系统接入游戏主循环：
// 1. 钓鱼系统 (FishingSystem + FishingUI)
// 2. 观云镜系统 (MysticMirrorSystem + MysticMirrorUI)
// 3. 茶灵契约系统 (ContractSystem + ContractUI)
//
// 集成方式：
// 1. 在 GameRuntime.hpp 中添加系统头文件
// 2. 在 GameRuntime.cpp 的 Initialize() 中初始化系统
// 3. 在 GameApp.cpp 的 ProcessEvents() 中处理输入
// 4. 在 GameApp.cpp 的 Render() 中渲染UI
//
// ============================================================================

#include "CloudSeamanor/FishingSystem.hpp"
#include "CloudSeamanor/engine/FishingMiniGame.hpp"
#include "CloudSeamanor/MysticMirrorSystem.hpp"
#include "CloudSeamanor/engine/PixelMysticMirrorPanel.hpp"
#include "CloudSeamanor/ContractSystem.hpp"
#include "CloudSeamanor/engine/PixelContractPanel.hpp"

namespace CloudSeamanor::engine {

// ============================================================================
// 【新系统UI管理器声明】
// ============================================================================

// 钓鱼UI
inline FishingUI& GetFishingUI() {
    return FishingUI::Instance();
}

// 观云镜UI
inline MysticMirrorUI& GetMysticMirrorUI() {
    return MysticMirrorUI::Instance();
}

// 契约UI
inline ContractUI& GetContractUI() {
    return ContractUI::Instance();
}

// ============================================================================
// 【新系统领域层声明】
// ============================================================================

// 钓鱼系统
inline domain::FishingSystem& GetFishingSystem() {
    return domain::FishingSystem::Instance();
}

// 观云镜系统
inline domain::MysticMirrorSystem& GetMysticMirrorSystem() {
    return domain::MysticMirrorSystem::Instance();
}

// 契约系统
inline domain::ContractSystem& GetContractSystem() {
    return domain::ContractSystem::Instance();
}

// ============================================================================
// 【快捷宏】
// ============================================================================

// 检查是否有UI处于活跃状态
#define IS_ANY_NEW_UI_ACTIVE() ( \
    GetFishingUI().IsFishing() || \
    GetMysticMirrorUI().IsVisible() || \
    GetContractUI().IsVisible() \
)

// 检查是否可以触发交互
#define CAN_TRIGGER_INTERACTION() (!IS_ANY_NEW_UI_ACTIVE())

}  // namespace CloudSeamanor::engine
