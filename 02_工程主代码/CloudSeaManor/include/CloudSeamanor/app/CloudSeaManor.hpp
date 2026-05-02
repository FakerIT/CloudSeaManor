#pragma once

// ============================================================================
// 【CloudSeaManor】对外聚合主头文件（Master Header）
// ============================================================================
// 用途：
// - 提供给外部工具/集成方的一键引入入口。
// - 适合 Demo、脚本或第三方接入时快速包含常用公共接口。
//
// 注意：
// - 项目内部开发默认“按需 include”，避免引入无关依赖导致编译膨胀。
// - 只有稳定公共 API 才应被纳入本聚合头。
// ============================================================================

#include "CloudSeamanor/app/GameApp.hpp"
#include "CloudSeamanor/app/GameAppState.hpp"
#include "CloudSeamanor/engine/GameRuntime.hpp"
#include "CloudSeamanor/engine/GameWorldState.hpp"
#include "CloudSeamanor/engine/GameWorldSystems.hpp"
#include "CloudSeamanor/engine/FarmingSystem.hpp"
#include "CloudSeamanor/engine/PickupSystem.hpp"
